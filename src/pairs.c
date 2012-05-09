/* 
 * pairs.c: 5.2-like replacements/enhancements for iterator factories
 *
 * 1. ipairs honors __ipairs [formerly it also worked on strings and any __index-able]
 * 2. pairs honors __pairs [formerly it also fell back to ipairs for non-tables (they might have __ipairs or __index)]
 *
 */

#include <lua.h>
#include <lauxlib.h>

#include "fivetwo.h"


/* --- copied from lbaselib.c --- */
static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* create arg[2] if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int ipairsaux (lua_State *L) {
  /* stack[1] = LUA_TTABLE; stack[2] = int */
  luaL_checktype(L, 1, LUA_TTABLE);
  int i = luaL_checkint(L, 2);
  i++;  /* next value */
  lua_pushinteger(L, i);
  lua_rawgeti(L, 1, i);
  /*
  lua_pushvalue(L, -1); [* copy i to be consumed in value lookup *]
  lua_gettable(L, 1); [* honor __index? *]
  */
  return (lua_isnil(L, -1)) ? 1 : 2;
}


/* --- adapted from 5.2's lbaselib.c --- */
static int pairsmeta (lua_State *L, const char *meth, int iszero) {
  /* stack[1]=object, stack[2]=default_generator */
  if (!luaL_getmetafield(L, 1, meth)) {  /* no metamethod? */
    luaL_checktype(L, 1, LUA_TTABLE);  /* argument must be a table */
    lua_pushvalue(L, 1);  /* return default_generator, state, */
    if (iszero) lua_pushinteger(L, 0);  /* and initial value */
    else lua_pushnil(L);
  }
  else {
    // lua_remove(L, 2); /* delete default_generator */
    lua_pushvalue(L, 1);  /* argument 'self' to metamethod */
    lua_call(L, 1, 3);  /* get 3 values from metamethod */
  }
  return 3;
}


/* Requires ipairsaux as upvalue(1) */
static int luaB_ipairs (lua_State *L) {
  lua_settop(L, 1);
  /* try __ipairs, return <metamethod or ipairsaux>, object, 0 */
  lua_pushvalue(L, lua_upvalueindex(1)); /* push default_generator */
  return pairsmeta(L, "__ipairs", 1);
  /* formerly we also checked for LUA_TSTRING or __index */
}


/* Requires luaB_next as upvalue(1) */
static int luaB_pairs (lua_State *L) {
  lua_settop(L, 1);
  /* try __pairs, return <metamethod or luaB_next>, object, nil */
  lua_pushvalue(L, lua_upvalueindex(1)); /* push default_generator */
  return pairsmeta(L, "__pairs", 0);
  /* formerly we fell pack to ipairs, in case the object was a string or handled __index */
}


extern int luaopen_fivetwo_pairs (lua_State *L) {
    lua_pushcfunction(L, ipairsaux);  /* ipairs needs ipairsaux as upvalue */
    lua_pushcclosure(L, luaB_ipairs, 1);
    lua_setglobal(L, "ipairs");

    lua_getglobal(L, "next");  /* pairs needs next as upvalue */
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushcfunction(L, luaB_next);
    }
    lua_pushcclosure(L, luaB_pairs, 1);
    lua_setglobal(L, "pairs");

    return 0;
}
