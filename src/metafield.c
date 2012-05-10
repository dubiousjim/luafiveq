/* 
 * metafield.c: part of fiveqplus libraries
 *
 * 1. debug.getmetafield(table or userdata, fieldname): ignores __metatable
 *
 */

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define LUA_FIVEQ_PLUS
#include "fiveq.h"


static const char *const fieldnames[] = { "__len","__reversed","__copy","__eq","__index","__tostring",NULL };

static int db_getmetafield (lua_State *L) {
    int tp = lua_type(L, 1);
    if (tp != LUA_TUSERDATA && tp != LUA_TTABLE)
        return luaL_error(L, "attempt to get metafield of a %s value", lua_typename(L, tp));
    /* we only retrieve string keys */
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "attempt to get non-string metafield");
    luaL_checkoption(L, 2, NULL, fieldnames);
    if (lua_getmetatable(L, 1)) {
        /* currently we retrieve metafields even if there's a blocking __metatable, like debug.getmetatable does */
        if (0 && luaL_getmetafield(L, 1, "__metatable")) {
            /* currently we fail for any __metatable, even if they're tables */
            if (1 || lua_type(L, -1) != LUA_TTABLE) {
                /* non-table meta.__metatable? */
                lua_pushnil(L);
                return 1;
            }
        }
        /* stack = obj, fieldname, ... metatable or meta.__metatable */
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static const luaL_Reg mlib[] =
{
        { "getmetafield",  db_getmetafield },
        { NULL,		NULL	}
};

extern int luaopen_fiveq_metafield (lua_State *L) {
    luaQ_checklib(L, LUA_DBLIBNAME);
    luaL_setfuncs(L, mlib, 0);
    return 0;
}
