// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>  // For gettimeofday
#include <stdio.h>     // For fprintf, stderr
#include "script.h"
#include "http_parser.h"
#include "stats.h"
#include "zmalloc.h"

typedef struct {
    char *name;
    int   type;
    void *value;
} table_field;

// Function declarations
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

// Lua library definitions
static const struct luaL_Reg addrlib[] = {
    { "__tostring", script_addr_tostring },
    { "__gc",       script_addr_gc       },
    { NULL,         NULL                 }
};

static const struct luaL_Reg statslib[] = {
    { "__index", script_stats_get },
    { "__len",   script_stats_len },
    { NULL,      NULL             }
};

static const struct luaL_Reg threadlib[] = {
    { "__index",    script_thread_index    },
    { "__newindex", script_thread_newindex },
    { NULL,         NULL                   }
};

// Implementation of script_create
lua_State *script_create(const char *file, const char *url, char **headers) {
    lua_State *L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "Error: Unable to create Lua state\n");
        exit(1);
    }

    luaL_openlibs(L);

    // Load WRK Lua bindings
    if (luaL_dostring(L, "wrk = require 'wrk'")) {
        const char *cause = lua_tostring(L, -1);
        fprintf(stderr, "Error loading WRK bindings: %s\n", cause);
        lua_close(L);
        exit(1);
    }

    luaL_newmetatable(L, "wrk.addr");
    luaL_setfuncs(L, addrlib, 0);
    luaL_newmetatable(L, "wrk.stats");
    luaL_setfuncs(L, statslib, 0);
    luaL_newmetatable(L, "wrk.thread");
    luaL_setfuncs(L, threadlib, 0);

    // Parse the provided URL
    struct http_parser_url parts = {};
    if (!script_parse_url(url, &parts)) {
        fprintf(stderr, "Error parsing URL: %s\n", url);
        lua_close(L);
        exit(1);
    }

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

    if (file && luaL_dofile(L, file)) {
        const char *cause = lua_tostring(L, -1);
        fprintf(stderr, "Error loading Lua script '%s': %s\n", file, cause);
        lua_close(L);
        exit(1);
    }

    return L;
}

// Implement the other script functions as provided...

// Helper functions
static void set_field(lua_State *L, int index, char *field, int type) {
    (void)type;
    lua_setfield(L, index, field);
}

static void set_fields(lua_State *L, int index, const table_field *fields) {
    for (int i = 0; fields[i].name; i++) {
        table_field f = fields[i];
        switch (f.value == NULL ? LUA_TNIL : f.type) {
            case LUA_TFUNCTION:
                lua_pushcfunction(L, (lua_CFunction) f.value);
                break;
            case LUA_TNUMBER:
                lua_pushinteger(L, *((lua_Integer *) f.value));
                break;
            case LUA_TSTRING:
                lua_pushstring(L, (const char *) f.value);
                break;
            case LUA_TNIL:
                lua_pushnil(L);
                break;
        }
        lua_setfield(L, index, f.name);
    }
}

static int push_url_part(lua_State *L, char *url, struct http_parser_url *parts, enum http_parser_url_fields field) {
    int type = parts->field_set & (1 << field) ? LUA_TSTRING : LUA_TNIL;
    uint16_t off, len;
    switch (type) {
        case LUA_TSTRING:
            off = parts->field_data[field].off;
            len = parts->field_data[field].len;
            lua_pushlstring(L, &url[off], len);
            break;
        case LUA_TNIL:
            lua_pushnil(L);
    }
    return type;
}
