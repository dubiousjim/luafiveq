/*
 * fiveq.c: elements of Lua 5.2's libraries backported to lua 5.1.4, and
 * vice-versa
 */


#include "fiveq.h"
#include <lualib.h>


/* --- adapted from lua-5.2.0's lmathlib.c --- */

#include <stdlib.h>
#include <math.h>

typedef unsigned char lu_byte;

static int floorlog2 (unsigned int v) {
  static const lu_byte log_2[256] = {
   -1,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
  };
  unsigned r = 0;
  register unsigned int t, tt;
  if ((tt = v >> 16)) {
    r = (t = tt >> 8) ? 24 + log_2[t] : 16 + log_2[tt];
  }
  else {
    r = ((t = v >> 8)) ? 8 + log_2[t] : log_2[v];
  }
  return r;
}


static int math_log (lua_State *L) {
  lua_Number x = luaL_checknumber(L, 1);
  lua_Number res;
  if (lua_isnoneornil(L, 2))
    res = (log)(x);
  else {
    lua_Number base = luaL_checknumber(L, 2);
    if (base == 10.0 || x == 0.0) res = (log10)(x);
    else if (base == 2.0) {
      unsigned int i = luaL_checkinteger(L, 1);
      res = (floorlog2)(i);
    }
    else res = (log)(x)/(log)(base);
  }
  lua_pushnumber(L, res);
  return 1;
}

static int math_trunc (lua_State *L) {
    lua_Integer n = luaL_checkinteger(L, 1);
    lua_pushinteger(L, n);
    return 1;
}

/* --- adapted from lua-5.2.0's ltablib.c --- */

static int table_pack (lua_State *L) {
  int n = lua_gettop(L);  /* number of elements to pack */
# ifdef LUA_FIVEQ_PLUS
#  if 0
  lua_createtable(L, n, 2);  /* create result table */
  lua_pushinteger(L, n);
  lua_setfield(L, -2, "#");  /* t['#'] = number of elements */
  lua_pushinteger(L, n);
  lua_setfield(L, -2, "n");  /* t['n'] also = number of elements */
#  else
  lua_createtable(L, n, 1);  /* create result table */
  lua_pushinteger(L, n);
  lua_setfield(L, -2, "#");  /* t['#'] = number of elements */
#  endif
# else
  lua_createtable(L, n, 1);  /* create result table */
  lua_pushinteger(L, n);
  lua_setfield(L, -2, "n");  /* t['n'] = number of elements */
# endif
  if (n > 0) {  /* at least one element? */
    int i;
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1);  /* insert first element */
    lua_replace(L, 1);  /* move table into index 1 */
    for (i = n; i >= 2; i--)  /* assign other elements */
      lua_rawseti(L, 1, i);
  }
/* alternative idea: return table and number of elements
 *   lua_pushinteger(L, n);
 *   return 2;
 */
  return 1;  /* return table */
}

static int table_unpack (lua_State *L) {
  int i, e, n;
  luaL_checktype(L, 1, LUA_TTABLE);
  i = luaL_optint(L, 2, 1);
  if (lua_isnoneornil(L, 3)) {
# ifdef LUA_FIVEQ_PLUS
    lua_getfield(L, 1, "#");
    if (lua_type(L, -1) == LUA_TNUMBER)
      e = lua_tointeger(L, -1);
    else {
#  if 0
      lua_pop(L, 1);
      lua_getfield(L, 1, "n");
      if (lua_type(L, -1) == LUA_TNUMBER)
        e = lua_tointeger(L, -1);
      else
#  endif
# else
    lua_getfield(L, 1, "n");
    if (lua_type(L, -1) == LUA_TNUMBER)
      e = lua_tointeger(L, -1);
    else {
# endif
      e = luaL_len(L, 1);
    }
    lua_pop(L, 1);
  }
  else {
    e = luaL_checkint(L, 3);
  }
  if (i > e) return 0;  /* empty range */
  n = e - i + 1;  /* number of elements */
  if (n <= 0 || !lua_checkstack(L, n))  /* n <= 0 means arith. overflow */
    return luaL_error(L, "too many results to unpack");
  lua_rawgeti(L, 1, i);  /* push arg[i] (avoiding overflow problems) */
  while (i++ < e)  /* push arg[i + 1...e] */
    lua_rawgeti(L, 1, i);
  return n;
}

#ifdef LUA_FIVEQ_PLUS
extern int getfenv(lua_State *L) {
    int level = luaL_checkint(L, 1);
    if (level > 0) {
        luaQ_getfenv(L, level, NULL);
    }
    else {
        luaL_argcheck(L, level == 0, 1, "negative level");
        lua_pushglobaltable(L);
    }
    return 1;
}
#endif


static int newproxy (lua_State *L) {
  lua_settop(L, 1);
  lua_newuserdata(L, 0);  /* create proxy */
  if (lua_isfunction(L, 1)) {
    lua_newtable(L);
    lua_pushvalue(L, 1);
    lua_setfield(L, -2, "__gc");
    lua_pushvalue(L, -1);
    lua_pushboolean(L, 1);
    lua_rawset(L, lua_upvalueindex(1)); /* weaktable[meta] = true */
  }
  else if (lua_toboolean(L, 1) == 0)
    return 1; /* return userdata with no metatable */
  else if (lua_isboolean(L, 1)) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_pushboolean(L, 1);
    lua_rawset(L, lua_upvalueindex(1));  /* weaktable[meta] = true */
  }
  else {
    int validproxy = 0;  /* check if weaktable[metatable(stack[1])] == true */
    if (lua_getmetatable(L, 1)) {
      lua_rawget(L, lua_upvalueindex(1));
      validproxy = lua_toboolean(L, -1);
      lua_pop(L, 1);
    }
    luaL_argcheck(L, validproxy, 1, "boolean or proxy expected");
    lua_getmetatable(L, 1);  /* metatable is valid */
  }
  lua_setmetatable(L, 2);
  return 1;
}


/* expects REGISTRY[_LOADED] at top of stack */
static void require(lua_State *L, const char *modname, lua_CFunction openf) {
  lua_getfield(L, -1, modname);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    /* gidx == 0: we want the library in _LOADED but not in _G or anywhere else */
    luaL_requiref(L, modname, openf, 0);
    if (lua_isnil(L, -1)) {
      luaL_error(L, "can't open " LUA_QS " library", modname);
    }
  }
}

/* registers one function in table at top of stack */
static void set1func(lua_State *L, const char *funcname, lua_CFunction f) {
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, funcname);
}


extern int luaopen_fiveq_module (lua_State *L);
extern int luaopen_fiveq_iter (lua_State *L);
extern int luaopen_fiveq_metafield (lua_State *L);
extern int luaopen_fiveq_faststring (lua_State *L);
extern int luaopen_fiveq_err (lua_State *L);
extern int luaopen_fiveq_hash (lua_State *L);
extern int luaopen_fiveq_struct (lua_State *L);


/* expects stack[1]=msg, stack[2]=level; upvalue[1]=io.stderr, upvalue[2]=debug.traceback */
extern int traceback(lua_State *L) {
    lua_settop(L, 2);
    if (!luaL_getmetafield(L, lua_upvalueindex(1), "write"))
        lua_pop(L, 2);
    else {
        lua_pushvalue(L, lua_upvalueindex(1)); /* io.stderr:write(...) */
        lua_pushstring(L, "\n");
        lua_pushvalue(L, -3);
        lua_pushvalue(L, -3);
    lua_pushvalue(L, lua_upvalueindex(2)); /* debug.traceback */
        lua_pushvalue(L, 1); /* msg */
        lua_pushinteger(L, lua_tointeger(L, 2) + 1); /* level */
    lua_call(L, 2, 1);
        lua_call(L, 2, 0); /* write traceback; discard return value */
        lua_call(L, 2, 0); /* write \n; discard return value */
    }
    return 0;
}

/* ----------- for 5.1.4 ---------- */
#if LUA_VERSION_NUM == 501


/* --- adapted from lua-5.2.0's lbaselib.c --- */

/* stack[1]=function, [2]=errfilter, [3]=args... */
static int luaB_xpcall (lua_State *L) {
  int n = lua_gettop(L);
  luaL_argcheck(L, n >= 2, 2, "value expected");
  // luaL_checktype(L, 2, LUA_TFUNCTION);

  // switch function & error filter
  lua_pushvalue(L, 1);
  lua_pushvalue(L, 2);
  lua_replace(L, 1);
  lua_replace(L, 2);

  int status = lua_pcall(L, n - 2, LUA_MULTRET, 1);
  luaL_checkstack(L, 1, NULL);
  lua_pushboolean(L, (status == 0));
  lua_replace(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static int luaB_rawlen (lua_State *L) {
  int t = lua_type(L, 1);
  luaL_argcheck(L, t == LUA_TTABLE || t == LUA_TSTRING, 1,
                   "table or string expected");
  lua_pushinteger(L, lua_rawlen(L, 1));
  return 1;
}


/* --- adapted from lua-5.2.0's lstrlib.c --- */

static int str_rep (lua_State *L) {
  size_t l, lsep;
  const char *s = luaL_checklstring(L, 1, &l);
  int n = luaL_checkint(L, 2);
  const char *sep = luaL_optlstring(L, 3, "", &lsep);
  if (n <= 0) lua_pushliteral(L, "");
  else {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    while(n-- > 1) { /* first n-1 copies (followed by separator) */
      luaL_addlstring(&b, s, l);
      luaL_addlstring(&b, sep, lsep);
    }
    luaL_addlstring(&b, s, l); /* last copy (not followed by separator) */
    luaL_pushresult(&b);
  }
  return 1;
}




/* --- adapted from lua-5.2.0's ldblib.c --- */


static int db_getuservalue (lua_State *L) {
  if (lua_type(L, 1) != LUA_TUSERDATA)
    lua_pushnil(L);
  else
    lua_getuservalue(L, 1);
  return 1;
}

static int db_setuservalue (lua_State *L) {
  if (lua_type(L, 1) == LUA_TLIGHTUSERDATA)
    luaL_argerror(L, 1, "full userdata expected, got light userdata");
  luaL_checktype(L, 1, LUA_TUSERDATA);
  if (!lua_isnoneornil(L, 2))
    luaL_checktype(L, 2, LUA_TTABLE);
  lua_settop(L, 2);
  lua_setuservalue(L, 1);
  return 1;
}

extern int luaopen_fiveq_pairs (lua_State *L);
extern int luaopen_fiveq_io (lua_State *L);
extern int luaopen_fiveq_bitlib (lua_State *L);

extern int luaopen_fiveq (lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");

  lua_register(L, "xpcall", luaB_xpcall);  /* export to _G */

  /* rawlen(string or table) is exported by the tablelen patch */
  lua_getglobal(L, "rawlen");
  if (lua_isnil(L, -1)) {
      lua_register(L, "rawlen", luaB_rawlen);  /* export to _G */
  }
  lua_pop(L, 1);

  require(L, LUA_STRLIBNAME, luaopen_string);
  set1func(L, "rep", str_rep);  /* export to string library */
  lua_pop(L, 1);  /* pop string library */

  require(L, LUA_TABLIBNAME, luaopen_table);
  set1func(L, "pack", table_pack);  /* export to table library */
# ifdef LUA_FIVEQ_PLUS
  lua_pushcfunction(L, table_unpack);
  lua_pushvalue(L, -1);  /* duplicate */
  lua_setglobal(L, "unpack");  /* with one copy, overwrite _G.unpack */
# else
  lua_getglobal(L, "unpack");
# endif
  lua_setfield(L, -2, "unpack");  /* export to table library */
  lua_pop(L, 1);  /* pop table library */

  require(L,  LUA_MATHLIBNAME, luaopen_math);
  set1func(L, "log", math_log);  /* export to math library */
# ifdef LUA_FIVEQ_PLUS
  set1func(L, "trunc", math_trunc);  /* export to math library */
# endif
  lua_pop(L, 1);  /* pop math library */

  require(L,  LUA_DBLIBNAME, luaopen_debug);
  /* export to debug library */
  set1func(L, "getuservalue", db_getuservalue);
  set1func(L, "setuservalue", db_setuservalue);
  require(L,  LUA_IOLIBNAME, luaopen_io);
  lua_getfield(L, -1, "stderr");
  lua_getfield(L, -3, "traceback");
  lua_pushcclosure(L, traceback, 2);
  lua_setfield(L, LUA_REGISTRYINDEX, "_TRACEBACK");
  lua_pop(L, 1); /* pop io library */

# ifdef LUA_FIVEQ_PLUS
  /* newproxy needs a weaktable as upvalue */
  lua_createtable(L, 0, 1);  /* new table w */
  lua_pushvalue(L, -1);  /* w will be its own metatable */
  lua_setmetatable(L, -2);
  lua_pushliteral(L, "kv");
  lua_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  lua_pushcclosure(L, newproxy, 1);  /* assign with upvalue(1) = w */
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "newproxy");  /* export to debug library */
  lua_setglobal(L, "newproxy");
# endif

  lua_pop(L, 1);  /* pop debug library */

  require(L,  LUA_LOADLIBNAME, luaopen_package);
  lua_getfield(L, -1, "loaders");  /* get package.loaders */
  lua_setfield(L, -2, "searchers");  /* export to package library */
  lua_pop(L, 1);  /* pop package library */

  lua_pop(L, 1);  /* pop REG._LOADED */

  /* redefines ipairs and pairs, returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_pairs);
  lua_pushstring(L, "fiveq.pairs");
  lua_call(L, 1, 0);

  /* redefines lines, read, write in file metatable
     redefines lines, read, open in io library
     redefines popen's "__close"
     redefines os.execute
     returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_io);
  lua_pushstring(L, "fiveq.io");
  lua_call(L, 1, 0);

  luaL_requiref(L, "bit32", luaopen_fiveq_bitlib, 1);

# ifdef LUA_FIVEQ_PLUS

  /* defines require, module, package.seeall, package.strict; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_module);
  lua_pushstring(L, "fiveq.module");
  lua_call(L, 1, 0);

  /* defines iter; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_iter);
  lua_pushstring(L, "fiveq.iter");
  lua_call(L, 1, 0);

  /* defines debug.getmetafield; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_metafield);
  lua_pushstring(L, "fiveq.metafield");
  lua_call(L, 1, 0);

  /* defines faststring methods; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_faststring);
  lua_pushstring(L, "fiveq.faststring");
  lua_call(L, 1, 0);

  /* these return 1, we use luaL_requiref to assign the value globally */
  luaL_requiref(L, "err", luaopen_fiveq_err, 1);
  luaL_requiref(L, "hash", luaopen_fiveq_hash, 1);
  luaL_requiref(L, "struct", luaopen_fiveq_struct, 1);

  lua_pushstring(L, "plus");
  lua_setglobal(L, "_FIVEQ");

# else

  lua_pushboolean(L, 1);
  lua_setglobal(L, "_FIVEQ");

# endif

  return 0;
}



/* ----------- for 5.2.0 ---------- */
#elif LUA_VERSION_NUM == 502

static int table_maxn (lua_State *L) {
  lua_Number max = 0;
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushnil(L);  /* first key */
  while (lua_next(L, 1)) {
    lua_pop(L, 1);  /* remove value */
    if (lua_type(L, -1) == LUA_TNUMBER) {
      lua_Number v = lua_tonumber(L, -1);
      if (v > max) max = v;
    }
  }
  lua_pushnumber(L, max);
  return 1;
}


static int math_log10 (lua_State *L) {
  lua_pushnumber(L, log10(luaL_checknumber(L, 1)));
  return 1;
}


/* --- adapted from lua-5.2.0's ldblib.c --- */

static int db_getuservalue (lua_State *L) {
  if (lua_type(L, 1) != LUA_TUSERDATA)
    lua_pushnil(L);
  else
    lua_getuservalue(L, 1);
  return 1;
}

static int db_setuservalue (lua_State *L) {
  if (lua_type(L, 1) == LUA_TLIGHTUSERDATA)
    luaL_argerror(L, 1, "full userdata expected, got light userdata");
  luaL_checktype(L, 1, LUA_TUSERDATA);
  luaL_checktype(L, 2, LUA_TTABLE);
  lua_settop(L, 2);
  lua_setuservalue(L, 1);
  return 1;
}


extern int luaopen_fiveq (lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");

  lua_getglobal(L, "load");
  lua_setglobal(L, "loadstring");

  require(L, LUA_TABLIBNAME, luaopen_table);
  lua_pushcfunction(L, table_maxn);
  lua_setfield(L, -2, "maxn");  /* export to table library */

# ifdef LUA_FIVEQ_PLUS
  set1func(L, "pack", table_pack);  /* export to table library */
  lua_pushcfunction(L, table_unpack);
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "unpack");  /* export to table library */
# else
  lua_getfield(L, -1, "unpack");  /* get table.unpack */
# endif
  lua_setglobal(L, "unpack");
  lua_pop(L, 1);  /* pop table library */

  require(L, LUA_MATHLIBNAME, luaopen_math);
  set1func(L, "log10", math_log10);  /* export to math library */
# ifdef LUA_FIVEQ_PLUS
  set1func(L, "log", math_log);  /* export to math library */
  set1func(L, "trunc", math_trunc);  /* export to math library */
# endif
  lua_pop(L, 1);  /* pop math library */


  /* debug.[gs]etfenv for userdata only */
  require(L, LUA_DBLIBNAME, luaopen_debug);
  // lua_pushcfunction(L, db_getuservalue);
  lua_getfield(L, -1, "getuservalue");  /* get debug.getuservalue */
  lua_setfield(L, -2, "getfenv");  /* export alias to debug library */
  // lua_pushcfunction(L, db_setuservalue);
  lua_getfield(L, -1, "setuservalue");  /* get debug.setuservalue */
  lua_setfield(L, -2, "setfenv");  /* export alias to debug library */

  require(L,  LUA_IOLIBNAME, luaopen_io);
  lua_getfield(L, -1, "stderr");
  lua_getfield(L, -3, "traceback");
  lua_pushcclosure(L, traceback, 2);
  lua_setfield(L, LUA_REGISTRYINDEX, "_TRACEBACK");
  lua_pop(L, 1); /* pop io library */

# ifdef LUA_FIVEQ_PLUS
  lua_register(L, "getfenv", getfenv);  /* export to _G */
# endif

  /* newproxy needs a weaktable as upvalue */
  lua_createtable(L, 0, 1);  /* new table w */
  lua_pushvalue(L, -1);  /* w will be its own metatable */
  lua_setmetatable(L, -2);
  lua_pushliteral(L, "kv");
  lua_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  lua_pushcclosure(L, newproxy, 1);  /* assign with upvalue(1) = w */
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "newproxy");  /* export to debug library */
  lua_setglobal(L, "newproxy");
  lua_pop(L, 1);  /* pop debug library */


  require(L, LUA_LOADLIBNAME, luaopen_package);
  lua_getfield(L, -1, "searchers");  /* get package.searchers */
  lua_setfield(L, -2, "loaders");  /* export alias to package library */
  lua_pop(L, 1);  /* pop package library */

  lua_pop(L, 1);  /* pop REG._LOADED */

# ifdef LUA_FIVEQ_PLUS

  /* defines require, module, package.seeall, package.strict; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_module);
  lua_pushstring(L, "fiveq.module");
  lua_call(L, 1, 0);

  /* defines iter, returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_iter);
  lua_pushstring(L, "fiveq.iter");
  lua_call(L, 1, 0);

  /* defines debug.getmetafield, returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_metafield);
  lua_pushstring(L, "fiveq.metafield");
  lua_call(L, 1, 0);

  /* defines faststring methods; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_faststring);
  lua_pushstring(L, "fiveq.faststring");
  lua_call(L, 1, 0);

  /* these return 1, we use luaL_requiref to assign the value globally */
  luaL_requiref(L, "err", luaopen_fiveq_err, 1);
  luaL_requiref(L, "hash", luaopen_fiveq_hash, 1);
  luaL_requiref(L, "struct", luaopen_fiveq_struct, 1);

  lua_pushstring(L, "plus");
  lua_setglobal(L, "_FIVEQ");

# else

#  if !defined(LUA_COMPAT_MODULE)
  /* defines require, module, package.seeall; returns 0 */
  lua_pushcfunction(L, luaopen_fiveq_module);
  lua_pushstring(L, "fiveq.module");
  lua_call(L, 1, 0);
#  endif

  lua_pushboolean(L, 1);
  lua_setglobal(L, "_FIVEQ");

# endif

  return 0;
}

#endif
