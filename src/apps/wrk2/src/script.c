// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "script.h"
#include "http_parser.h"
#include "stats.h"
#include "zmalloc.h"

typedef struct {
    char *name;
    int   type;
    void *value;
} table_field;

static int script_addr_tostring(lua_State *);
static int script_addr_gc(lua_State *);
static int script_stats_len(lua_State *);
static int script_stats_get(lua_State *);
static int script_thread_index(lua_State *);
static int script_thread_newindex(lua_State *);
static int script_wrk_lookup(lua_State *);
static int script_wrk_connect(lua_State *);
static int script_wrk_time_us(lua_State *);

static void set_fields(lua_State *, int, const table_field *);
static void set_field(lua_State *, int, char *, int);
static int push_url_part(lua_State *, char *, struct http_parser_url *, enum http_parser_url_fields);

static const struct luaL_reg addrlib[] = {
    { "__tostring", script_addr_tostring   },
    { "__gc"    ,   script_addr_gc         },
    { NULL,         NULL                   }
};

static const struct luaL_reg statslib[] = {
    { "__index",    script_stats_get       },
    { "__len",      script_stats_len       },
    { NULL,         NULL                   }
};

static const struct luaL_reg threadlib[] = {
    { "__index",    script_thread_index    },
    { "__newindex", script_thread_newindex },
    { NULL,         NULL                   }
};

lua_State *script_create(char *file, char *url, char **headers) {
    printf("[DEBUG] Initializing Lua state\n");
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if (luaL_dostring(L, "wrk = require \"wrk\"")) {
        fprintf(stderr, "[ERROR] Failed to load 'wrk' module: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return NULL;
    }

    luaL_newmetatable(L, "wrk.addr");
    luaL_register(L, NULL, addrlib);
    luaL_newmetatable(L, "wrk.stats");
    luaL_register(L, NULL, statslib);
    luaL_newmetatable(L, "wrk.thread");
    luaL_register(L, NULL, threadlib);

    struct http_parser_url parts = {};
    script_parse_url(url, &parts);
    char *path = "/";

    if (parts.field_set & (1 << UF_PATH)) {
        path = &url[parts.field_data[UF_PATH].off];
    }

    const table_field fields[] = {
        { "lookup",  LUA_TFUNCTION, script_wrk_lookup  },
        { "connect", LUA_TFUNCTION, script_wrk_connect },
        { "time_us", LUA_TFUNCTION, script_wrk_time_us },
        { "path",    LUA_TSTRING,   path               },
        { NULL,      0,             NULL               },
    };

    lua_getglobal(L, "wrk");

    set_field(L, 4, "scheme", push_url_part(L, url, &parts, UF_SCHEMA));
    set_field(L, 4, "host",   push_url_part(L, url, &parts, UF_HOST));
    set_field(L, 4, "port",   push_url_part(L, url, &parts, UF_PORT));
    set_fields(L, 4, fields);

    lua_getfield(L, 4, "headers");
    for (char **h = headers; *h; h++) {
        char *p = strchr(*h, ':');
        if (p && p[1] == ' ') {
            lua_pushlstring(L, *h, p - *h);
            lua_pushstring(L, p + 2);
            lua_settable(L, 5);
        }
    }
    lua_pop(L, 5);

    if (file) {
        printf("[DEBUG] Loading Lua script file: %s\n", file);
        if (luaL_dofile(L, file)) {
            const char *cause = lua_tostring(L, -1);
            fprintf(stderr, "[ERROR] Lua script error: %s\n", cause);
            lua_pop(L, 1);
        }
    }

    return L;
}

bool script_resolve(lua_State *L, char *host, char *service) {
    printf("[DEBUG] Resolving host: %s, service: %s\n", host, service);
    lua_getglobal(L, "wrk");

    lua_getfield(L, -1, "resolve");
    lua_pushstring(L, host);
    lua_pushstring(L, service);
    lua_call(L, 2, 0);

    lua_getfield(L, -1, "addrs");
    size_t count = lua_objlen(L, -1);
    lua_pop(L, 2);
    printf("[DEBUG] Resolved %zu addresses\n", count);
    return count > 0;
}

void script_init(lua_State *L, thread *t, int argc, char **argv) {
    printf("[DEBUG] Initializing thread in Lua\n");
    lua_getglobal(t->L, "wrk");

    script_push_thread(t->L, t);
    lua_setfield(t->L, -2, "thread");

    lua_getglobal(L, "wrk");
    lua_getfield(L, -1, "setup");
    script_push_thread(L, t);
    lua_call(L, 1, 0);
    lua_pop(L, 1);

    lua_getfield(t->L, -1, "init");
    lua_newtable(t->L);
    for (int i = 0; i < argc; i++) {
        lua_pushstring(t->L, argv[i]);
        lua_rawseti(t->L, -2, i);
    }
    lua_call(t->L, 1, 0);
    lua_pop(t->L, 1);
}

void script_request(lua_State *L, char **buf, size_t *len) {
    printf("[DEBUG] Generating request from Lua\n");
    int pop = 1;
    lua_getglobal(L, "request");
    if (!lua_isfunction(L, -1)) {
        lua_getglobal(L, "wrk");
        lua_getfield(L, -1, "request");
        pop += 2;
    }
    lua_call(L, 0, 1);
    const char *str = lua_tolstring(L, -1, len);
    if (!str) {
        fprintf(stderr, "[ERROR] Failed to retrieve request from Lua\n");
        *buf = NULL;
        *len = 0;
    } else {
        *buf = realloc(*buf, *len);
        memcpy(*buf, str, *len);
    }
    lua_pop(L, pop);
}

void script_response(lua_State *L, int status, buffer *headers, buffer *body) {
    printf("[DEBUG] Processing response in Lua\n");
    lua_getglobal(L, "response");
    lua_pushinteger(L, status);
    lua_newtable(L);

    for (char *c = headers->buffer; c < headers->cursor; ) {
        c = buffer_pushlstring(L, c);
        c = buffer_pushlstring(L, c);
        lua_rawset(L, -3);
    }

    lua_pushlstring(L, body->buffer, body->cursor - body->buffer);
    lua_call(L, 3, 0);

    buffer_reset(headers);
    buffer_reset(body);
}
