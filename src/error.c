/* 
 * error.c: part of fivetwoplus libraries
 *
 * 1.  error(fmt, [level], [...])
 * 2.  assert(bool, [fmt], [...])
 *       returns all args if bool is true
 * 3.  check(obj, arg#, [fmt], [...])
 *       returns only obj
 * 4.  istype(obj, types...)
 *       returns first matching type, or false
 *       types can be standard "nil" and so on,
 *       or "callable", "indexable", "iterable", "iterator",
 *       or "string!", "number!", "integer!", "positive[!]", "negative[!]", "natural[!]"
 *       or a typeobject (metatable or __type)
 * 5.  arenil(...)
 *       true if all args are nil
 * 6.  checktype(obj, arg#, [expected idx], types...)
 *       returns first matching type, or raises
 *       complains that it expected the first/last type, if none match
 * 7.  checkopt(obj, arg#, type, [default])
 *       asserts that obj is of type or is nil
 *       returns object, or if it's nil, default
 * 8.  checkany(obj, arg#)
 *       raises if nil, else returns obj
 * 9.  checkrange(num, arg#, [[-]max] or [min, [-]max])
 *       returns abs(num) if it's in range
 * 10. bad(arg#, "extra")
 * 11. badtype(obj, arg#, "expected")
 *
 */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>

#define LUA_FIVETWO_PLUS
#include "fivetwo.h"


/****** these functions derived from lauxlib.c *********/

static int prefix_error (lua_State *L, int lvl) {
    /* stack: ..., errmsg */
    luaL_where(L, lvl);
    lua_insert(L, -2);
    lua_concat(L, 2);
    return lua_error(L);
}


static int prefix_argerror (lua_State *L, int narg, const char *extramsg, int lvl) {
    lua_Debug ar;
    if (!lua_getstack(L, lvl-1, &ar)) {  /* no stack frame? */
        lua_pushfstring(L, "bad argument #%d (%s)", narg, extramsg);
        return prefix_error(L, lvl);
    }
    lua_getinfo(L, "n", &ar);
    if (strcmp(ar.namewhat, "method") == 0) {
        narg--;  /* do not count 'self' */
        if (narg == 0) { /* error is in the self argument itself? */
            lua_pushfstring(L, "calling " LUA_QS " on bad self (%s)", ar.name, extramsg);
            return prefix_error(L, lvl);
        }
    }
    if (ar.name == NULL)
        ar.name = "?";
    lua_pushfstring(L, "bad argument #%d to " LUA_QS " (%s)", narg, ar.name, extramsg);
    return prefix_error(L, lvl);
}



static int error2 (lua_State *L) {
    /* upvalue #1: string.format */
    int lvl = luaL_optint(L, 2, 1); /* defaults to lvl=1 */
    if (lua_isstring(L, 1)) {
        if (lua_gettop(L) > 1)
            lua_remove(L, 2);
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, 1);
        /* now stack is: formatted msg */
        if (lvl > 0)  /* add extra information? */
            return prefix_error(L, lvl);
    } else
        lua_settop(L, 1);
    return lua_error(L);
}


static int assert2 (lua_State *L) {
    /* upvalue #1: string.format */
    if (lua_toboolean(L, 1))
        return lua_gettop(L);
    /* else arg[1] was false or nil */
    int nostr = lua_isnoneornil(L, 2);
    if (nostr) {
        lua_settop(L, 0);
        lua_pushstring(L, "assertion failed!");
    }
    else {
        luaL_checktype(L, 2, LUA_TSTRING);
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_replace(L, 1);
        /* now stack has: string.format, fmt string, ... */
        lua_call(L, lua_gettop(L) - 1, 1);
    }
    /* now stack has: formatted string */
    return prefix_error(L, 1); /* asserts without argnums are lvl=1 */
}


static int badarg(lua_State *L) {
    int arg = lua_tointeger(L, 1);
    luaL_argcheck(L, 0 < arg, 1, "bad argument number");
    return prefix_argerror(L, arg, luaL_checkstring(L, 2), 2); /* lvl=2 for argerrors */ 
}


static int badtype(lua_State *L) {
    int arg = lua_tointeger(L, 2);
    luaL_argcheck(L, 0 < arg, 2, "bad argument number");
    const char *msg = lua_pushfstring(L, "%s expected, got %s", luaL_checkstring(L, 3), luaL_typename(L, 1));
    return prefix_argerror(L, arg, msg, 2); /* lvl=2 for argerrors */ 
}


static int check (lua_State *L) {
    /* upvalue #1: string.format */
    if (lua_toboolean(L, 1)) {
        lua_settop(L, 1);
        return 1;
    }
    /* arg[2] must be number>0 */
    int arg = lua_tointeger(L, 2);
    luaL_argcheck(L, 0 < arg, 2, "bad argument number");
    int nostr = lua_isnoneornil(L, 3);
    if (nostr) {
        lua_settop(L, 0);
        lua_pushstring(L, "assertion failed!");
    }
    else {
        lua_remove(L, 2);
        luaL_checktype(L, 2, LUA_TSTRING);
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_replace(L, 1);
        /* now stack has: string.format, fmt string, ... */
        lua_call(L, lua_gettop(L) - 1, 1);
    }
    /* now stack has: formatted string */
    return prefix_argerror(L, arg, lua_tostring(L, -1), 2); /* lvl=2 for argerrors */
}


static int gettype(lua_State *L, int idx) {
    if (lua_getmetatable(L, idx)) {
        lua_pushliteral(L, "__type");
        lua_rawget(L, -2);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
        }
        else {
            /* replace metatable with __type */
            lua_replace(L, -2);
        }
        /* metatable or __type is now at stack[+1] */
        return 1;
    } else
        /* obj has no metatable */
        return 0;
}


static int typechecker(lua_State *L, int idx, int fromidx) {
    static const char *const typenames[] = { "nil","boolean","userdata","number!","string!","table","function","userdata","thread","nil","nil","indexable","callable","iterable","iterator","string", "number", "integer!", "positive!", "negative!", "natural!", "integer", "positive", "negative", "natural", NULL };
    int top = lua_gettop(L);
    int n = fromidx;
    int res = 0;
    while (n <= top) {

/*
        [* replace nil@pos(n) with default index *]
        if (lua_isnil(L, n)) {
            if (!defaultindex) {
                return luaL_typerror(L, n, "type");
            }
            lua_pushvalue(L, defaultindex);
            lua_replace(L, n);
        }
*/

        /* compare obj@pos(idx) to type@pos(n) */
        if (lua_isstring(L, n)) {
            int tp = luaL_checkoption(L, n, NULL, typenames);
            if (tp == 12) { /* LUA_TCALLABLE */
                if (lua_type(L, idx) == LUA_TFUNCTION) {
                    res = 1;
                }
                else if (lua_getmetatable(L, idx)) {
                    lua_pushliteral(L, "__call");
                    lua_rawget(L, -2);
                    res = (lua_type(L, -1) == LUA_TFUNCTION);
                    lua_pop(L, 2);
                }
            }
            else if (tp == 11) { /* LUA_TINDEXABLE */
                if (lua_type(L, idx) == LUA_TFUNCTION) {
                    res = 0;
                }
                else {
                    lua_pushvalue(L, idx);
                    if (lua_getmetatable(L, -1)) {
                        lua_pushliteral(L, "__index");
                        lua_rawget(L, -2);
                        // stack[+1]=obj, stack[+2]=meta, stack[+3]=__index
                        if (lua_isnil(L, -1)) {
                            lua_pop(L, 2);
                        }
                        else {
                            lua_replace(L, -3);
                            lua_pop(L, 1);
                        }
                    }
                    // stack[+1]=obj or its non-nil __index
                    res = lua_type(L, -1);
                    res = (res == LUA_TFUNCTION || res == LUA_TTABLE);
                    lua_pop(L, 1);
                }
            }
            else if (tp == 13) { /* LUA_TITERABLE */
                if (lua_type(L, idx) == LUA_TTABLE)
                    res = 1;
                else if (lua_getmetatable(L, idx)) {
                    lua_pushliteral(L, "__iter");
                    lua_rawget(L, -2);
                    // stack[+1]=meta, stack[+2]=__iter
                    if (!lua_isnil(L, -1)) {
                        lua_pop(L, 2);
                        res = 1;
                    }
                    else {
                        lua_pushliteral(L, "__pairs");
                        lua_rawget(L, -3);
                        // stack[+1]=meta, stack[+2]=nil, stack[+3]=__pairs
                        res = !lua_isnil(L, -1);
                        lua_pop(L, 3);
                    }
                }
            }
            else if (tp == 14) { /* LUA_TITERATOR */
                res = lua_type(L, idx) == LUA_TFUNCTION;
            }
            else if (tp == 15) { /* LUA_TSLOPPYSTRING */
                res = lua_isstring(L, idx);
            }
            else if (tp == 16 || tp == 21) { /* LUA_TSLOPPYNUMBER or LUA_TSLOPPYINTEGER */
                res = lua_isnumber(L, idx);
            }
            else if (tp == 17) { /* LUA_TINTEGER */
                lua_Integer m;
                if (lua_type(L, idx) == LUA_TNUMBER && (m = lua_tointeger(L, 1), (lua_Number) m == lua_tonumber(L, 1)))
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == 18) { /* LUA_TPOSITIVE */
                lua_Integer m;
                if (lua_type(L, idx) == LUA_TNUMBER && (m = lua_tointeger(L, 1), (lua_Number) m == lua_tonumber(L, 1)) && m > 0)
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == 19) { /* LUA_TNEGATIVE */
                lua_Integer m;
                if (lua_type(L, idx) == LUA_TNUMBER && (m = lua_tointeger(L, 1), (lua_Number) m == lua_tonumber(L, 1)) && m < 0)
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == 20) { /* LUA_TNATURAL */
                lua_Integer m;
                if (lua_type(L, idx) == LUA_TNUMBER && (m = lua_tointeger(L, 1), (lua_Number) m == lua_tonumber(L, 1)) && m >= 0)
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == 22) { /* LUA_TSLOPPYPOSITIVE */
                lua_Integer m;
                if (lua_isnumber(L, idx) && (m = lua_tointeger(L, 1), m > 0))
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == 23) { /* LUA_TSLOPPYNEGATIVE */
                lua_Integer m;
                if (lua_isnumber(L, idx) && (m = lua_tointeger(L, 1), m < 0))
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == 24) { /* LUA_TSLOPPYNATURAL */
                lua_Integer m;
                if (lua_isnumber(L, idx) && (m = lua_tointeger(L, 1), m >= 0))
                    res = 1;
                else
                    res = 0;
            }
            else if (tp == LUA_TLIGHTUSERDATA) {
                res = lua_type(L, idx);
                res = (res == LUA_TLIGHTUSERDATA || res == LUA_TUSERDATA);
            }
            else {
                res = lua_type(L, idx) == tp;
            }
        }
        else if (gettype(L, idx)) {
            res = lua_rawequal(L, n, -1);
            lua_pop(L, 1);
        }
        if (res) {
            lua_pushvalue(L, n);
            return 1;
        }
        n = n + 1;
    }
    return 0;
}


static int istype(lua_State *L) {
    /* stack: obj, ... */
    if (lua_gettop(L) < 2)
        lua_settop(L, 2);
    if (!typechecker(L, 1, 2)) {
        /* didn't find any matching types */
        lua_pushboolean(L, 0);
    }
    /* else matching type was pushed */
    return 1;
}


static int arenil(lua_State *L) {
    int i;
    int n = lua_gettop(L);
    for (i=1; i <= n; i++) {
        if (!lua_isnil(L, i)) {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    lua_pushboolean(L, 1);
    return 1;
}


static int checktype (lua_State *L) {
    // stack: [1]=obj, [2]=arg#, [3]=[expected_idx], [4..top]=types...
    int top = lua_gettop(L);
    if (top < 3) {
        lua_settop(L, 3);
        top = 3;
    }
    int arg = lua_tointeger(L, 2);
    luaL_argcheck(L, 0 < arg, 2, "bad argument number");
    int defidx = 3;
    int defarg = 3;
    if (lua_type(L, 3) == LUA_TNUMBER) {
        defidx = lua_tointeger(L, 3);
        luaL_argcheck(L, 1 <= defidx && defidx <= top - 3, 3, "bad index");
        defidx += 2;
        defarg = defidx + 1;
        lua_remove(L, 3);
        if (--top < 3) {
            lua_settop(L, 3);
            // top = 3;
        }
    }
    if (lua_type(L, defidx) == LUA_TSTRING) {
        lua_pushvalue(L, defidx);
    }
    else if (!luaL_callmeta(L, defidx, "__tostring") ||
            lua_type(L, -1) != LUA_TSTRING) {
        luaL_typerror(L, defarg, "string or named type");
    }
    lua_insert(L, 3);
    // stack: [1]=obj, [2]=arg#, [3]=expected string, [4..top]=types...

    if (typechecker(L, 1, 4))
        return 1; /* found matching type */

    const char *msg = NULL;
    if (gettype(L, 1))
        if (luaL_callmeta(L, -1, "__tostring"))
            msg = lua_tostring(L, -1);
    if (!msg)
        msg = luaL_typename(L, 1);
    msg = lua_pushfstring(L, "%s expected, got %s", lua_tostring(L, 3), msg);
    return prefix_argerror(L, arg, msg, 2);
}


static int checkopt (lua_State *L) {
    // stack: [1]=obj, [2]=arg#, [3]=type, [4]=[default]
    lua_settop(L, 4);
    int arg = lua_tointeger(L, 2);
    luaL_argcheck(L, 0 < arg, 2, "bad argument number");
    if (lua_type(L, 3) == LUA_TSTRING) {
        lua_pushvalue(L, 3);
    }
    else if (!luaL_callmeta(L, 3, "__tostring") ||
            lua_type(L, -1) != LUA_TSTRING) {
        luaL_typerror(L, 3, "string or named type");
    }
    lua_replace(L, 2); /* replace arg# with expected string */
    // stack: [1]=obj, [2]=expected string, [3]=type, [4]=[default]

    if (lua_isnil(L, 1))
        return 1; /* return default or nil */

    lua_settop(L, 3);
    if (typechecker(L, 1, 3)) {
        lua_settop(L, 1);
        return 1; /* matched type */
    }

    const char *msg = NULL;
    if (gettype(L, 1))
        if (luaL_callmeta(L, -1, "__tostring"))
            msg = lua_tostring(L, -1);
    if (!msg)
        msg = luaL_typename(L, 1);
    msg = lua_pushfstring(L, "%s expected, got %s", lua_tostring(L, 2), msg);
    return prefix_argerror(L, arg, msg, 2);
}


static int checkany(lua_State *L) {
    int arg = lua_tointeger(L, 2);
    luaL_argcheck(L, 0 < arg, 2, "bad argument number");
    if (lua_isnil(L, 1))
        return prefix_argerror(L, arg, "value expected", 2);
    lua_settop(L, 1);
    return 1;
}


static int checkrange (lua_State *L) {
    int arg = lua_tointeger(L, 2);
    luaL_argcheck(L, 0 < arg, 2, "bad argument number");
    lua_settop(L, 4);
    if (lua_isnil(L, 4)) {
        lua_pop(L, 1);
        lua_pushinteger(L, 1);
        lua_insert(L, 3);
    }
    int lo = luaL_checkint(L, 3);
    int hi = luaL_checkint(L, 4);
    int negok = 0;
    if (hi < 0) {
        hi = -hi;
        negok = 1;
    }
    int n;
    if (lua_type(L, 1) != LUA_TNUMBER || (n = lua_tointeger(L, 1), (lua_Number) n != lua_tonumber(L, 1)))
        return prefix_argerror(L, arg, "not an integer", 2);
    if (n < 0 && negok)
        n = hi + 1 + n;
    if (n < lo || n > hi)
        return prefix_argerror(L, arg, "out of range", 2);
    lua_pushinteger(L, n);
    return 1;
}


static const luaL_Reg R[] =
{
        { "istype",     istype    },
        { "arenil",     arenil    },
        { "checktype",  checktype },
        { "checkopt",   checkopt  },
        { "checkany",   checkany  },
        { "checkrange", checkrange},
        { "bad",        badarg    },
        { "badtype",    badtype   },
        { NULL,		NULL	  }
};

extern int luaopen_fivetwo_error (lua_State *L) {
    lua52_checklib(L, LUA_STRLIBNAME);
    lua_getfield(L, -1, "format");
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_pushcclosure(L, error2, 1);
    lua_setglobal(L, "error");
    lua_pushcclosure(L, assert2, 1);
    lua_setglobal(L, "assert");
    lua_pushcclosure(L, check, 1);
    lua_setglobal(L, "check");
    lua_pop(L, 1);  /* pop string library */

    /* export functions to _G */
    /* lua_pushglobaltable(L); */
    /* luaL_setfuncs(L, R, 0); */

    luaL_openlib(L, "err", R, 0);
    return 0;
}
