// Copyright (C) 2012 - Will Glozer.  All rights reserved.

#include "wrk.h"
#include "script.h"
#include "main.h"
#include "hdr_histogram.h"
#include "stats.h"

// Max recordable latency of 1 day
#define MAX_LATENCY 24L * 60 * 60 * 1000000

static struct config {
    uint64_t threads;
    uint64_t connections;
    uint64_t duration;
    uint64_t timeout;
    uint64_t pipeline;
    uint64_t rate;
    uint64_t delay_ms;
    bool     latency;
    bool     u_latency;
    bool     dynamic;
    bool     record_all_responses;
    char    *host;
    char    *script;
    SSL_CTX *ctx;
} cfg;

static struct {
    stats *requests;
    pthread_mutex_t mutex;
} statistics;

static struct sock sock = {
    .connect  = sock_connect,
    .close    = sock_close,
    .read     = sock_read,
    .write    = sock_write,
    .readable = sock_readable
};

static struct http_parser_settings parser_settings = {
    .on_message_complete = response_complete
};

static volatile sig_atomic_t stop = 0;

static void handler(int sig) {
    stop = 1;
}

static void usage() {
    printf("Usage: wrk <options> <url>                            \n"
           "  Options:                                            \n"
           "    -c, --connections <N>  Connections to keep open   \n"
           "    -d, --duration    <T>  Duration of test           \n"
           "    -t, --threads     <N>  Number of threads to use   \n"
           "                                                      \n"
           "    -s, --script      <S>  Load Lua script file       \n"
           "    -H, --header      <H>  Add header to request      \n"
           "    -L  --latency          Print latency statistics   \n"
           "    -U  --u_latency        Print uncorrected latency statistics\n"
           "        --timeout     <T>  Socket/request timeout     \n"
           "    -B, --batch_latency    Measure latency of whole   \n"
           "                           batches of pipelined ops   \n"
           "                           (as opposed to each op)    \n"
           "    -v, --version          Print version details      \n"
           "    -R, --rate        <T>  work rate (throughput)     \n"
           "                           in requests/sec (total)    \n"
           "                           [Required Parameter]       \n"
           "                                                      \n"
           "                                                      \n"
           "  Numeric arguments may include a SI unit (1k, 1M, 1G)\n"
           "  Time arguments may include a time unit (2s, 2m, 2h)\n");
}

static void dump_lua_stack(lua_State *L) {
    int top = lua_gettop(L);
    printf("[DEBUG] Lua Stack Dump (top = %d):\n", top);
    for (int i = 1; i <= top; i++) {
        int t = lua_type(L, i);
        switch (t) {
            case LUA_TSTRING:
                printf("  [%d]: '%s'\n", i, lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf("  [%d]: %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("  [%d]: %g\n", i, lua_tonumber(L, i));
                break;
            default:
                printf("  [%d]: %s\n", i, lua_typename(L, t));
                break;
        }
    }
}

lua_State *script_create(char *script_path, char *url, char **headers) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_loadfile(L, script_path) || lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "[ERROR] Lua script load failed: %s\n", lua_tostring(L, -1));
        dump_lua_stack(L);
        lua_close(L);
        exit(1);
    }

    printf("[INFO] Lua script loaded successfully: %s\n", script_path);
    return L;
}

static int connect_socket(thread *thread, connection *c) {
    int fd = socket(thread->addr->ai_family, thread->addr->ai_socktype, thread->addr->ai_protocol);
    if (fd < 0) {
        perror("[ERROR] Socket creation failed");
        thread->errors.connect++;
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("[ERROR] Failed to set non-blocking mode");
        close(fd);
        return -1;
    }

    if (connect(fd, thread->addr->ai_addr, thread->addr->ai_addrlen) == -1 && errno != EINPROGRESS) {
        perror("[ERROR] Connect failed");
        close(fd);
        return -1;
    }

    printf("[DEBUG] Connection initiated: fd=%d\n", fd);
    return fd;
}

static void socket_writeable(aeEventLoop *loop, int fd, void *data, int mask) {
    connection *c = data;
    thread *thread = c->thread;

    if (!c->written && cfg.dynamic) {
        script_request(thread->L, &c->request, &c->length);
        printf("[DEBUG] Dynamic script request generated. Length = %zu\n", c->length);
    }

    char *buf = c->request + c->written;
    size_t len = c->length - c->written;
    size_t n;

    switch (sock.write(c, buf, len, &n)) {
        case OK:
            printf("[DEBUG] Write successful: fd=%d, bytes=%zu\n", fd, n);
            break;
        case ERROR:
            perror("[ERROR] Write error");
            goto error;
        case RETRY:
            return;
    }

    c->written += n;
    if (c->written == c->length) {
        printf("[DEBUG] Request fully written: fd=%d\n", fd);
        c->written = 0;
        aeDeleteFileEvent(loop, fd, AE_WRITABLE);
    }
    return;

error:
    thread->errors.write++;
    reconnect_socket(thread, c);
}

static int response_complete(http_parser *parser) {
    connection *c = parser->data;
    thread *thread = c->thread;
    uint64_t now = time_us();
    int status = parser->status_code;

    printf("[DEBUG] Response received: status=%d, time=%" PRIu64 "\n", status, now);

    if (status >= 400) {
        fprintf(stderr, "[ERROR] Response error: status=%d\n", status);
        thread->errors.status++;
    }

    if (c->headers.buffer) {
        *c->headers.cursor++ = '\0';
        script_response(thread->L, status, &c->headers, &c->body);
        dump_lua_stack(thread->L);
    }

    return 0;
}

void *thread_main(void *arg) {
    thread *thread = arg;
    printf("[INFO] Thread started: ID=%p, Connections=%" PRIu64 "\n", thread, thread->connections);

    thread->cs = zcalloc(thread->connections * sizeof(connection));
    for (uint64_t i = 0; i < thread->connections; i++) {
        connect_socket(thread, &thread->cs[i]);
    }

    aeMain(thread->loop);

    printf("[INFO] Thread exiting: ID=%p\n", thread);
    return NULL;
}

int main(int argc, char **argv) {
    printf("[INFO] Starting wrk2 with arguments:\n");
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d] = %s\n", i, argv[i]);
    }

    // Parse arguments
    char *url, **headers = zmalloc(argc * sizeof(char *));
    struct http_parser_url parts = {};
    if (parse_args(&cfg, &url, &parts, headers, argc, argv)) {
        usage();
        exit(1);
    }

    lua_State *L = script_create(cfg.script, url, headers);
    printf("[INFO] Lua script initialized successfully.\n");

    thread *threads = zcalloc(cfg.threads * sizeof(thread));
    for (uint64_t i = 0; i < cfg.threads; i++) {
        pthread_create(&threads[i].thread, NULL, thread_main, &threads[i]);
    }

    for (uint64_t i = 0; i < cfg.threads; i++) {
        pthread_join(threads[i].thread, NULL);
    }

    printf("[INFO] Wrk2 completed successfully.\n");
    return 0;
}
