/*
 * module.c for fiveqplus
 * adapted from Lua sources, and http://lua-users.org/wiki/ModuleDefinition
 */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include "fiveq.h"


/*
 * ll_require behaves exactly like Lua 5.2's require, which is mostly the same
 * as Lua 5.1.4, except that 5.1.4 leaves package.loaded[name] alone if
 * the library wrote anything there (even nil), and detects cyclic requires.
 * Whereas 5.2.0 only leaves it alone if the library wrote non-nil there,
 * and no longer detects cyclic requires. We allow for the difference in
 * signatures between 5.1.4 loaders/libraries (loaders return a single
 * function, which is called with the library name) and 5.2.0 (loaders
 * return a single function plus an arbitrary value, which is supplied as
 * a second argument to the library).
 *
 * Following the native design, we use the global "package" table as upvalue(1)
 * (in 5.1.4, it's the function's env). But other designs are also reasonable:
 * package.loaded is already accessible as REG._LOADED, and we only need
 * additional access to package.searchers: why not make that table directly be
 * the upvalue/env?
 */

static int ll_require_base (lua_State *L, int loaded) {
  int i;
  const char *name = luaL_checkstring(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, -1, name);
  if (lua_toboolean(L, -1)) {  /* is it there? */
    return 1;  /* package is already loaded */
  }
  /* else must load package; iterate over available searchers */
  lua_pop(L, 1);  /* remove 'getfield' result */
  /* lua_getfield(L, LUA_ENVIRONINDEX, "loaders"); */
#if LUA_VERSION_NUM == 501
  lua_getfield(L, lua_upvalueindex(1), "loaders");  /* will be at index 3 */
  if (!lua_istable(L, -1))
    luaL_error(L, LUA_QL("package.loaders") " must be a table");
#else
  lua_getfield(L, lua_upvalueindex(1), "searchers");  /* will be at index 3 */
  if (!lua_istable(L, -1))
    luaL_error(L, LUA_QL("package.searchers") " must be a table");
#endif
  lua_pushliteral(L, "");  /* error message accumulator */
  /*  iterate over available seachers to find a loader */
  for (i=1; ; i++) {
    lua_rawgeti(L, -2, i);  /* get a searcher */
    if (lua_isnil(L, -1)) {
      /* no more searchers */
      luaL_error(L, "module " LUA_QS " not found:%s",
                    name, lua_tostring(L, -2));
    }
    lua_pushstring(L, name);
#if LUA_VERSION_NUM == 501
    lua_call(L, 1, 1);  /* call it */
#else
    lua_call(L, 1, 2);  /* call it */
#endif
    if (lua_isfunction(L, 3+loaded))  /* did it find module? */
      break;  /* module loaded successfully */
    else if (lua_isstring(L, 3+loaded)) {  /* loader returned error message? */
      lua_settop(L, 3+loaded);  /* remove any extra return values */
      lua_concat(L, 2);  /* accumulate it */
    }
    else
      lua_settop(L, 2+loaded);
  }
  lua_pushstring(L, name);  /* pass name as argument to module */
  lua_insert(L, 4+loaded);  /* name is 1st argument (before search data) */
#if LUA_VERSION_NUM == 501
    lua_call(L, 1, 1);  /* run loaded module */
#else
    lua_call(L, 2, 1);  /* run loaded module */
#endif
  if (!lua_isnil(L, -1))  /* non-nil return? */
    lua_setfield(L, loaded, name);  /* _LOADED[name] = returned value */
  lua_getfield(L, loaded, name);
  if (lua_isnil(L, -1)) {   /* module did not set a value? */
    lua_pushboolean(L, 1);  /* use true as result */
    lua_pushvalue(L, -1);  /* extra copy to be returned */
    lua_setfield(L, loaded, name);  /* _LOADED[name] = true */
  }
  return 1;
}

#if 0
static int ll_require (lua_State *L) {
  lua_settop(L, 1); /* _LOADED table will be at index 2 */
  return ll_require_base(L, 2);
}
#endif

#ifdef LUA_FIVEQ_PLUS
/*
 * This wrapper provides the enhancements to `require`.
 * It needs package as upvalue(1).
 */
static int ll_require_plus (lua_State *L) {
    int top = lua_gettop(L);
    // stack[1]=modname, stack[2..top]= [table], keys...
    const char *modname = luaL_checkstring(L, 1);
    lua_pushvalue(L, 1);
    ll_require_base(L, top+2);
    // stack[+1]=_LOADED[name]
    if (top == 1)
        return 1;
    else if (!lua_istable(L, -1))
        luaL_error(L, "module " LUA_QS " doesn't export a table", modname);
    const char *k;
    if (!lua_istable(L, 2))
        luaQ_getfenv(L, 1, "require");
    else if (top > 2) {
        lua_pushvalue(L, 2);
        lua_remove(L, 2);
        top--;
    }
    else {
        /* require("foo",tbl) */
        lua_pushnil(L);  /* first key */
        while (lua_next(L, 3) != 0) {
            // stack[1]=modname, stack[2]=tbl, stack[+1]=_LOADED[name], stack[+2]=key, stack[+3]=value
            if (lua_type(L, 4) == LUA_TSTRING) {
                k = lua_tostring(L, 4);
                if (*k == '_') {
                    /* skip this value */
                    lua_pop(L, 1);
                    continue;
                }
            }
            lua_pushvalue(L, 4);
            lua_insert(L, 5);
            lua_settable(L, 2);
        }
        return 1; /* return stack[+1] */
    }
    /* require("foo", [tbl], ...) */
    const char *bad;
    int i;
    for (i=2; i <= top; i++) {
        // stack[2..top]=keys, stack[+1]=_LOADED[name], stack[+2]=tbl
        k = luaL_checkstring(L, i);
        if (luaQ_getdeepvalue(L, -2, k) != NULL)
            luaL_error(L, "module " LUA_QS " doesn't provide " LUA_QS, modname, k);
        bad = luaQ_setdeepvalue(L, -2, k); 
        if (bad != NULL)
            luaL_error(L, "couldn't set " LUA_QS, bad);
    }
    lua_pop(L, 1);
    return 1;
}
#endif


#ifdef LUA_FIVEQ_PLUS
static const int sentinel_ = 0;
#define sentinel ((const void *)&sentinel_)
#else
extern void luaQ_setfenv (lua_State *L, int level, const char *fname);
extern void luaQ_checklib (lua_State *L, const char *libname);
#endif


/*
 * fiveqplus tweaks the module implementation.
 * Default is to provide a clean environment, and module's public interface
 * is the same as its _ENV.
 * We register module in env of caller of chunk that invokes module, instead of _G.
 */
static int ll_module (lua_State *L) {
  const char *modname = luaL_checkstring(L, 1);
  int lastdecorator = lua_gettop(L);
  /* get/create module table */
#ifndef LUA_FIVEQ_PLUS
  luaL_pushmodule(L, modname, 1);
#else
  luaQ_pushmodule(L, modname, 1, 2, "module");
#endif
  /* does module table already have a _NAME field? */
  lua_getfield(L, -1, "_NAME");
  if (!lua_isnil(L, -1))
    lua_pop(L, 1);
  else {  /* initialize it */
    lua_pop(L, 1);
    /* ---- modinit(L, modname); ---- */
    const char *dot;
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "_M");  /* module._M = module */
    lua_pushstring(L, modname);
    lua_setfield(L, -2, "_NAME");
    dot = strrchr(modname, '.');  /* look for last dot in module name */
    if (dot == NULL) dot = modname;
    else dot++;
    /* set _PACKAGE as package name (full module name minus last part) */
    lua_pushlstring(L, modname, dot - modname);
    lua_setfield(L, -2, "_PACKAGE");
    /* ------------------------------ */
  }
#ifdef LUA_FIVEQ_PLUS
  /* temporarily set module[sentinel]=caller's _ENV for use by seeall decorator */
  luaQ_getfenv(L, 1, "module");
  lua_setfield(L, -2, sentinel);
#endif
  /* set caller's fenv to module table */
  lua_pushvalue(L, -1);
  luaQ_setfenv(L, 1, "module");
  /* ---- dooptions(L, lastdecorator); ---- */
  int i;
  for (i = 2; i <= lastdecorator; i++) {
    if (lua_isfunction(L, i)) {  /* avoid 'calling' extra info. */
      lua_pushvalue(L, i);  /* get option (a function) */
      lua_pushvalue(L, -2);  /* module */
      lua_call(L, 1, 0);
    }
  }
  /* -------------------------------------- */
#ifdef LUA_FIVEQ_PLUS
  lua_pushnil(L);
  lua_setfield(L, -2, sentinel);
#endif
  return 1; // return module table
}


#ifndef LUA_FIVEQ_PLUS
static int ll_seeall (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  if (!lua_getmetatable(L, 1)) {
    lua_createtable(L, 0, 1); /* create new metatable */
    lua_pushvalue(L, -1);
    lua_setmetatable(L, 1);
  }
  lua_pushglobaltable(L);
  lua_setfield(L, -2, "__index");  /* mt.__index = _G */
  return 0;
}

static const luaL_Reg pkglib[] = {
  {"seeall", ll_seeall},
  {NULL, NULL}
};

#else
static int ll_seeall_index (lua_State *L) {
    // stack[1]=proxy, stack[2]=k, upvalue(1)=_M, upvalue(2)=caller's _ENV
    lua_settop(L, 2);
    lua_pushvalue(L, 2);
    lua_gettable(L, lua_upvalueindex(1));
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_gettable(L, lua_upvalueindex(2));
    }
    return 1;
}

/*
 * Module decorator
 * Expose caller's _ENV via a private _ENV for module
 * Writes to the private enviroment get mirrored to the module's public interface
 */
static int ll_seeall (lua_State *L) {
  // stack[1]=module
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 1);
  lua_newtable(L); /* create proxy */
  lua_createtable(L, 0, 2); /* create its metatable */
  lua_pushvalue(L, 1);
  /* lua_pushvalue(L, LUA_GLOBALSINDEX); */
  lua_getfield(L, 1, sentinel);
  lua_pushcclosure(L, ll_seeall_index, 2);
  lua_setfield(L, 3, "__index");  /* meta.__index = ll_seeall_index */
  lua_pushvalue(L, 1);
  lua_setfield(L, 3, "__newindex"); /* meta.__newindex = module */
  lua_setmetatable(L, 2);
  luaQ_setfenv(L, 2, "seeall"); /* make proxy the caller's _ENV */
  return 0;
}



/*
** retrieves the calling function and its 'what'
*/
static void getcaller (lua_State *L, int level) {
  lua_Debug ar;
  if (lua_getstack(L, level, &ar) == 0 ||
      lua_getinfo(L, "fS", &ar) == 0)  /* get calling function */
    luaL_error(L, "couldn't identify the calling function");
  if (lua_iscfunction(L, -1) || ar.what == NULL)
      lua_pushliteral(L, "C");
  else
      lua_pushstring(L, ar.what); // "Lua", "C", or "main"
}


static int ll_strict_index (lua_State *L) {
    // stack[1]=tbl, stack[2]=k, upvalue(1)=former index, upvalue(2)=declared
    /* new __index with upvalues= former index, declared */
    lua_settop(L, 2);
    lua_pushvalue(L, lua_upvalueindex(1));
    if (lua_type(L, 3) == LUA_TFUNCTION) {
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 2);
        lua_call(L, 2, 1);
    }
    else {
        lua_pushvalue(L, 2);
        lua_gettable(L, 3);
        lua_remove(L, 3);
    }
    // stack[1]=tbl, stack[2]=k, stack[3]=former index[k]
    if (!lua_isnil(L, 3))
        return 1;
    lua_pushvalue(L, 2);
    lua_gettable(L, lua_upvalueindex(2));
    // stack[1]=tbl, stack[2]=k, ... stack[4]=declared[k]
    if (lua_isnil(L, -1)) {
        getcaller(L, 2);
        if (strcmp("C", lua_tostring(L, -1)) != 0)
            luaL_error(L, "variable " LUA_QS " is not declared", lua_tostring(L, 2));
    }
    lua_pushnil(L);
    return 1;
}


static int ll_strict_newindex (lua_State *L) {
    // stack[1]=tbl, stack[2]=k, stack[3]=v, upvalue(1)=former newindex, upvalue(2)=declared, upvalue(3)=toplevel of M, upvalue(4)=M
    lua_settop(L, 3);
    lua_pushvalue(L, 2);
    lua_gettable(L, lua_upvalueindex(2));
    if (lua_isnil(L, -1)) {
        getcaller(L, 2);
        if (!lua_rawequal(L, 5, lua_upvalueindex(3)) && strcmp(lua_tostring(L, -1), "C") != 0)
            luaL_error(L, "assigning to undeclared variable " LUA_QS, lua_tostring(L, 2));
        lua_pushvalue(L, 2);
        lua_pushboolean(L, 1);
        lua_settable(L, lua_upvalueindex(2)); /* declared[k]=true */
    }
    lua_settop(L, 3);
    lua_pushvalue(L, lua_upvalueindex(1));
    switch (lua_type(L, -1)) {
        case LUA_TFUNCTION : {
            lua_insert(L, 1);
            lua_call(L, 3, 0);
            break;
        }
        case LUA_TNIL : {
            lua_pop(L, 1);
            lua_rawset(L, lua_upvalueindex(4)); /* M[k]=v */
            break;
        }
        default : {
            lua_replace(L, 1);
            lua_settable(L, 1);
        }
    }
    return 0;
}


/*
 * Module decorator
 * Catches reads/writes of undeclared globals
 * Can write to undeclared globals only at module's toplevel
 */
static int ll_strict (lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    luaQ_getfenv(L, 2, "package.strict");
    if (!lua_getmetatable(L, 2)) {
        lua_createtable(L, 0, 2); /* create new metatable */
        lua_pushvalue(L, -1);
        lua_setmetatable(L, 2);
    }
    lua_getfield(L, 3, "__index");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_getfield(L, 1, sentinel); /* previous __index defaults to module's ENV */
    }
    lua_newtable(L); /* create `declared` table */
    lua_getfield(L, 3, "__newindex");
    lua_pushvalue(L, -2);
    getcaller(L, 3);
    lua_pop(L, 1); /* discard 'what', leave M's toplevel function */
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, ll_strict_newindex, 4);
    /* new __newindex with upvalues= former newindex, declared, toplevel of M, M */
    lua_setfield(L, 3, "__newindex");
    lua_pushcclosure(L, ll_strict_index, 2);
    /* new __index with upvalues= former index, declared */
    lua_setfield(L, 3, "__index");
    return 0;
}


static const luaL_Reg pkglib[] = {
  {"seeall", ll_seeall},
  {"strict", ll_strict},
  {NULL, NULL}
};
#endif


extern int luaopen_fiveq_module (lua_State *L) {
    /* export to _G */
    lua_register(L, "module", ll_module);

#if defined(LUA_FIVEQ_PLUS) || !defined(LUA_COMPAT_MODULE)
    luaQ_checklib(L, LUA_LOADLIBNAME);
    luaL_setfuncs(L, pkglib, 0);
#endif

#ifdef LUA_FIVEQ_PLUS
    /* make package library upvalue for [* ll_require and *] ll_requireplus */

    /*
    lua_pushvalue(L, -1);
    lua_pushcclosure(L, ll_require, 1);
    lua_pushcclosure(L, ll_require_plus, 2);
    */

    lua_pushcclosure(L, ll_require_plus, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "_FIVEQ");
    lua_pushvalue(L, -2);
    lua_pushinteger(L, 1);
    lua_settable(L, -3); /* _FIVEQ[our register]=1 */
    lua_getglobal(L, "require");
    lua_pushinteger(L, 1);
    lua_settable(L, -3); /* FIVEQ[native register]=1 */
    lua_pop(L, 1); /* pop FIVEQ */

    // stack[1]=name, stack[2]=ll_require_plus
    lua_setglobal(L, "require");
#endif

    return 0;
}

