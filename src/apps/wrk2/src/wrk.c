// Copyright (C) 2012 - Will Glozer.  All rights reserved.

#include "wrk.h"
#include "script.h"
#include "main.h"
#include "hdr_histogram.h"
#include "stats.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

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
           "    -v, --version          Print version details      \n"
           "    -R, --rate        <T>  work rate (throughput)     \n"
           "                           in requests/sec (total)    \n"
           "                           [Required Parameter]       \n"
           "                                                      \n"
           "                                                      \n"
           "  Numeric arguments may include a SI unit (1k, 1M, 1G)\n"
           "  Time arguments may include a time unit (2s, 2m, 2h)\n");
}

// Initialize Lua state
static lua_State* initialize_lua(const char *script_path) {
    lua_State *L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "Error: Could not create Lua state\n");
        exit(1);
    }

    luaL_openlibs(L); // Load standard libraries

    // Load wrk-specific Lua bindings if needed
    if (luaL_dofile(L, "wrk.lua")) {
        fprintf(stderr, "Error loading wrk.lua: %s\n", lua_tostring(L, -1));
        exit(1);
    }

    // Load the user script if provided
    if (script_path && luaL_dofile(L, script_path)) {
        fprintf(stderr, "Error loading Lua script %s: %s\n", script_path, lua_tostring(L, -1));
        exit(1);
    }

    return L;
}

// Create Lua script environment
lua_State* script_create(const char *script_path, const char *url, char **headers) {
    lua_State *L = initialize_lua(script_path);

    // Pass arguments like URL and headers to Lua
    lua_newtable(L);
    for (int i = 0; headers[i]; i++) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, headers[i]);
        lua_settable(L, -3);
    }
    lua_setglobal(L, "headers");

    lua_pushstring(L, url);
    lua_setglobal(L, "url");

    return L;
}

int main(int argc, char **argv) {
    char *url, **headers = zmalloc(argc * sizeof(char *));
    struct http_parser_url parts = {};

    if (parse_args(&cfg, &url, &parts, headers, argc, argv)) {
        usage();
        exit(1);
    }

    char *schema  = copy_url_part(url, &parts, UF_SCHEMA);
    char *host    = copy_url_part(url, &parts, UF_HOST);
    char *port    = copy_url_part(url, &parts, UF_PORT);
    char *service = port ? port : schema;

    if (!strncmp("https", schema, 5)) {
        if ((cfg.ctx = ssl_init()) == NULL) {
            fprintf(stderr, "Error: Unable to initialize SSL\n");
            ERR_print_errors_fp(stderr);
            exit(1);
        }
        sock.connect  = ssl_connect;
        sock.close    = ssl_close;
        sock.read     = ssl_read;
        sock.write    = ssl_write;
        sock.readable = ssl_readable;
    }

    cfg.host = host;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  SIG_IGN);

    pthread_mutex_init(&statistics.mutex, NULL);
    statistics.requests = stats_alloc(10);
    thread *threads = zcalloc(cfg.threads * sizeof(thread));

    hdr_init(1, MAX_LATENCY, 3, &(statistics.requests->histogram));

    lua_State *L = script_create(cfg.script, url, headers);

    if (!script_resolve(L, host, service)) {
        fprintf(stderr, "Error: Failed to resolve Lua script for %s:%s\n", host, service);
        lua_close(L);
        exit(1);
    }

    uint64_t connections = cfg.connections / cfg.threads;
    double throughput    = (double)cfg.rate / cfg.threads;
    uint64_t stop_at     = time_us() + (cfg.duration * 1000000);

    for (uint64_t i = 0; i < cfg.threads; i++) {
        thread *t = &threads[i];
        t->loop        = aeCreateEventLoop(10 + cfg.connections * 3);
        t->connections = connections;
        t->throughput  = throughput;
        t->stop_at     = stop_at;

        t->L = script_create(cfg.script, url, headers);
        script_init(t->L, t, argc - optind, &argv[optind]);

        if (i == 0) {
            cfg.pipeline = script_verify_request(t->L);
            cfg.dynamic  = !script_is_static(t->L);
            if (script_want_response(t->L)) {
                parser_settings.on_header_field = header_field;
                parser_settings.on_header_value = header_value;
                parser_settings.on_body         = response_body;
            }
        }

        if (!t->loop || pthread_create(&t->thread, NULL, &thread_main, t)) {
            fprintf(stderr, "Error: Unable to create thread %"PRIu64": %s\n", i, strerror(errno));
            lua_close(t->L);
            exit(2);
        }
    }

    struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags   = 0,
    };
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    printf("Running test @ %s\n", url);
    printf("  %"PRIu64" threads and %"PRIu64" connections\n", cfg.threads, cfg.connections);

    for (uint64_t i = 0; i < cfg.threads; i++) {
        pthread_join(threads[i].thread, NULL);
    }

    lua_close(L); // Clean up Lua state in main
    return 0;
}
