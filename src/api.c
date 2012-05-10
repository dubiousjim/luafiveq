/*
 * api.c: elements of Lua 5.2's API backported to lua 5.1.4, and vice-versa
 */

#include "fiveq.h"

#include <string.h>

/*
 * Based on luaL_findtable from lauxlib.c.
 * Tries to deeply get the table value at idx[fields],
 * creating intervening tables as necessary.
 * If successful, returns NULL, and pushes retrieved or new table onto stack
 * (sets existing to true/false if it's non-NULL).
 * If fails (some intervening value isn't a table), returns the problematic 
 * part of fields and returns stack unaltered.
 *
 * bool luaL_getsubtable(L, idx, fields) in 5.2.0 does this shallowly, returns
 * existing
 */
extern const char *luaQ_getdeeptable (lua_State *L, int idx, const char
        *fields, int szhint, int *existing) {
    const char *e;
    if (idx) lua_pushvalue(L, idx);
    if (existing) *existing = 1;
    do {
        /* point e to the next '.' in fields, or the terminal \0 */
        e = strchr(fields, '.');
        if (e == NULL) e = fields + strlen(fields);
        /* stack[+2] <- stack[+1][fields[0:e]] */
        lua_pushlstring(L, fields, e - fields);
        lua_rawget(L, -2); /* or lua_gettable to honor __index */
        if (lua_isnil(L, -1)) {
            /* no such field, create a new table at stack[+2] and assign it to
             * stack[+1][fields[0:e]] */
            if (existing) *existing = 0;
            lua_pop(L, 1);  /* remove nil at stack[+2] */
            lua_createtable(L, 0, (*e == '.' ? 1 : szhint));
            lua_pushlstring(L, fields, e - fields);
            lua_pushvalue(L, -2);
            lua_settable(L, -4);  /* set field to new table */
        }
        else if (!lua_istable(L, -1)) {
            /* non-table value */
            lua_pop(L, 2); /* remove table and value */
            return fields;
        }
        /* else final or intervening table, fall through */
        lua_remove(L, -2); /* remove previous table */
        fields = e + 1;
    } while (*e == '.');
    return NULL;
}


/*
 * Tries to deeply get the value at idx[fields], without creating any tables.
 * If successful, returns NULL, and pushes retrieved value or nil onto stack.
 * If fails (some intervening value isn't a table), returns the problematic 
 * part of fields and returns stack unaltered.
 */
extern const char *luaQ_getdeepvalue (lua_State *L, int idx, const char
        *fields) {
    const char *e;
    if (idx) lua_pushvalue(L, idx);
    do {
        /* point e to the next '.' in fields, or the terminal \0 */
        e = strchr(fields, '.');
        if (e == NULL) e = fields + strlen(fields);
        /* stack[+2] <- stack[+1][fields[0:e]] */
        lua_pushlstring(L, fields, e - fields);
        lua_rawget(L, -2); /* or lua_gettable to honor __index */
        if (lua_isnil(L, -1)) {
            /* couldn't find value, return with stack[+1] containing nil */
            lua_remove(L, -2);
            return NULL;
        }
        else if (*e != '.' || lua_istable(L, -1)) {
            /* final value or intervening table */
            lua_remove(L, -2); /* remove previous table */
            fields = e + 1;
        }
        else {
            /* intervening non-table value */
            lua_pop(L, 2); /* remove table and value */
            return fields;
        }
    } while (*e == '.');
    return NULL;
}


/*
 * Tries to deeply set idx[fields] to the value at top of stack,
 * creating intervening tables as necessary.
 * If successful, returns NULL and pops value from top of stack.
 * If fails (some intervening value isn't a table), returns the problematic 
 * part of fields and leaves value on stack.
 */
extern const char *luaQ_setdeepvalue (lua_State *L, int idx, const char
        *fields) {
    const char *e;
    lua_pushvalue(L, idx);
    do {
        /* point e to the next '.' in fields, or the terminal \0 */
        e = strchr(fields, '.');
        if (e == NULL) e = fields + strlen(fields);
        /* stack[+2] <- stack[+1][fields[0:e]] */
        lua_pushlstring(L, fields, e - fields);
        lua_rawget(L, -2); /* or lua_gettable to honor __index */
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);  /* remove nil at stack[+2] */
            if (*e == '.') {
                /* no such field, create a new table at stack[+2] and assign it
                 * to stack[+1][fields[0:e]] */
                lua_createtable(L, 0, 1);
                lua_pushlstring(L, fields, e - fields);
                lua_pushvalue(L, -2);
                lua_settable(L, -4);  /* set field to new table */
            }
            else {
                /* assign value at stack[+0] to table at stack[+1] */
                lua_pushlstring(L, fields, e - fields);
                lua_pushvalue(L, -3); /* push value */
                lua_settable(L, -3);
                lua_pop(L, 1); /* pop value and table */
                return NULL;
            }
        }
        else if (*e != '.' || !lua_istable(L, -1)) {
            /* final value or intervening non-table value */
            lua_pop(L, 2); /* remove table and value */
            return fields;
        }
        /* else intervening table, fall through */
        lua_remove(L, -2); /* remove previous table */
        fields = e + 1;
    } while (*e == '.');
    return NULL;
}


/*
 * Retrieves the environment table of calling function.
 * If fname is NULL, doesn't complain about caller being a C function.
 * Note that in 5.1.4 but not 5.2.0, tail calls increment the "level".
 */
extern void luaQ_getfenv (lua_State *L, int level, const char *fname) {
  if (level) {
    lua_Debug ar;
    if (fname == NULL) {
      if (lua_getstack(L, level, &ar) == 0 ||
          lua_getinfo(L, "f", &ar) == 0)  /* get calling function */
        luaL_error(L, "couldn't identify the calling function");
    }
    else {
      if (lua_getstack(L, level, &ar) == 0 ||
          lua_getinfo(L, "f", &ar) == 0 ||  /* get calling function */
          lua_iscfunction(L, -1))
        luaL_error(L, LUA_QS " not called from a Lua function", fname);
    }
#if LUA_VERSION_NUM == 501
    lua_getfenv(L, -1);
#else
    const char *var;
    int i;
    for(i=1;;i++) {
      var = lua_getlocal(L, &ar, i);
      if (!var || strcmp("_ENV", var)==0)
        break;
      lua_pop(L, 1);
    }
    if (!var) {
      for(i=1;;i++) {
        var = lua_getupvalue(L, -1, i);
        if (!var || strcmp("_ENV", var)==0)
          break;
        lua_pop(L, 1);
      }
    }
    if (!var) {
      /* luaL_error(L, "couldn't identify the calling function's _ENV"); */
      lua_pushglobaltable(L);
    }
#endif
    lua_remove(L, -2);  /* remove function */
  }
  else {
    lua_pushglobaltable(L);
  }
}


/*
 * Pops table from stack and assigns it as environment of function
 * at call stack `level`.
 */
extern void luaQ_setfenv (lua_State *L, int level, const char *fname) {
  lua_Debug ar;
  if (lua_getstack(L, level, &ar) == 0 ||
      lua_getinfo(L, "f", &ar) == 0 ||  /* get calling function */
      lua_iscfunction(L, -1))
    luaL_error(L, LUA_QS " not called from a Lua function", fname);
  lua_insert(L, -2); /* move below env table */
#if LUA_VERSION_NUM == 501
  lua_setfenv(L, -2);
#else
  lua_setupvalue(L, -2, 1);
#endif
  lua_pop(L, 1);  /* remove function */
}


#if LUA_VERSION_NUM == 501 || defined(LUA_FIVEQ_PLUS) || \
    !defined(LUA_COMPAT_MODULE)

static int libsize (const luaL_Reg *l) {
  int size = 0;
  for (; l && l->name; l++) size++;
  return size;
}

/*
 * Find or create a module table with a given name. The function first 
 * looks for _LOADED[fields], and if that fails, looks (deeply) for an 
 * environment variable at fields, creating tables as necessary, and 
 * writes to _LOADED[fields]. Uses global environment if level==0, else 
 * the env of the function at call stack `level`. If `caller` is non-NULL, 
 * fails if that function is a CFunction (and uses `caller` in the error 
 * message). If successful, module table is left on the stack.
 */
extern void luaQ_pushmodule (lua_State *L, const char *modname, int szhint,
  int level, const char *caller) {
    /* get _LOADED table */
    luaQ_getdeeptable(L, LUA_REGISTRYINDEX, "_LOADED", 1, NULL);
    lua_getfield(L, -1, modname);  /* get _LOADED[modname] */
    if (!lua_istable(L, -1)) {  /* not found? */
        lua_pop(L, 1);  /* remove previous result */
        /* try environment variable (and create one if it does not exist) */
        luaQ_getfenv(L, level, caller);
        if (luaQ_getdeeptable(L, 0, modname, szhint, NULL) != NULL)
            luaL_error(L, "name conflict for module " LUA_QS, modname);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, modname);  /* _LOADED[modname] = new table */
    }
    lua_remove(L, -2);  /* remove _LOADED table */
}

# if !defined(LUA_FIVEQ_PLUS)
extern void luaL_pushmodule (lua_State *L, const char *modname, int szhint) {
    luaQ_pushmodule(L, modname, szhint, 0, NULL);
}
# endif

#endif



#if LUA_VERSION_NUM == 501 || defined(LUA_FIVEQ_PLUS)

/*
 * Stripped-down 'require'.
 * Call as: luaL_requiref(L, "libname", luaopen_lib, assign in _G?)
 * Leaves single return value of luaopen_lib (or nil) at stack[+1.
 * Note this will *always* run luaopen_lib and overwrite REG._LOADED[libname].
 * Assigns to stack[gidx][libname], or _G[libname] if gidx==1,
 * or nowhere if gidx==0.
 */
extern void luaL_requiref (lua_State *L, const char *libname,
                               lua_CFunction luaopen_lib, int gidx) {
  lua_pushcfunction(L, luaopen_lib);
  lua_pushstring(L, libname);  /* argument to open function */
  lua_call(L, 1, 1);  /* open module */
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_pushvalue(L, -2);  /* make copy of module (call result) */
  lua_setfield(L, -2, libname);  /* _LOADED[libname] = module */
  lua_pop(L, 1);  /* remove _LOADED table */
  if (gidx != 0) {
    if (gidx == 1)
      /* for compatibility with 5.2.0 version */
      lua_pushglobaltable(L);
    else
      /* when gidx is other than 0 or 1, we write to stack[gidx] */
      lua_pushvalue(L, gidx);
    lua_pushvalue(L, -2);  /* copy of 'mod' */
    lua_setfield(L, -2, libname);  /* _G[libname] = module */
    lua_pop(L, 1);  /* remove _G table */
    // stack[+1] = open_lib("libname")
  }
}
#endif


# if defined(LUA_FIVEQ_PLUS) || (LUA_VERSION_NUM == 501 && \
        !defined(LUA_COMPAT_OPENLIB)) || (LUA_VERSION_NUM == 502 && \
        !defined(LUA_COMPAT_MODULE))
/*
 * If "libname", pushmodule to find existing module or write to 
 * caller's/global environment, else assumes there's already a table 
 * underneath upvalues.
 * Merges functions from luaL_Reg* into that table with luaL_setfuncs,
 * leaving that table on top of stack.
 */
extern void luaL_openlib (lua_State *L, const char *libname, const luaL_Reg *l,
        int nup) {
#  if LUA_VERSION_NUM == 502
    luaL_checkversion(L);
#  endif
    if (libname) {
#  if defined(LUA_FIVEQ_PLUS)
        /* check whether lib already exists, writing to caller's environment if
         * not */
        int level = 2;
        if (0x3 & ((size_t) l)) {
            level += 0x3 & ((size_t) l);
            l = (const luaL_Reg *)(2 - level + ((size_t) l));
        }
        luaQ_pushmodule(L, libname, libsize(l), level, NULL);
#  else
        /* check whether lib already exists, writing to global environment if
         * not */
        luaQ_pushmodule(L, libname, libsize(l), 0, NULL);
#  endif
        lua_insert(L, -(nup + 1));  /* move library table to below upvalues */
    }
    if (l)
        luaL_setfuncs(L, l, nup);
    else
        lua_pop(L, nup);  /* remove upvalues */
}
# elif LUA_VERSION_NUM == 501
#  undef luaL_openlib
extern void luaI_openlib (lua_State *L, const char *libname, const luaL_Reg *l,
        int nup) {
  if (l)
    luaL_openlib(L, libname, l, nup);
  else {
    const luaL_Reg empty[] = {
      {NULL, NULL}
    };
    luaL_openlib(L, libname, empty, nup);
  }
}
# endif


/* ----------- for 5.1.4 ---------- */
#if LUA_VERSION_NUM == 501

/* ----- adapted from lua-5.2.0 lapi.c: ----- */

extern lua_Number lua_tonumberx (lua_State *L, int idx, int *isnum) {
  lua_Number n = lua_tonumber(L, idx);
  if (n == 0 && !lua_isnumber(L, idx)) {
    if (isnum) *isnum = 0;
    return 0;
  }
  else {
    if (isnum) *isnum = 1;
    return n;
  }
}

extern lua_Integer lua_tointegerx (lua_State *L, int idx, int *isnum) {
  lua_Number n = lua_tonumber(L, idx);
  if (n == 0 && !lua_isnumber(L, idx)) {
    if (isnum) *isnum = 0;
    return 0;
  }
  else {
    if (isnum) *isnum = 1;
    lua_Integer r;
    lua_number2integer(r, n);
    return r;
  }
}

extern lua_Unsigned lua_tounsignedx (lua_State *L, int idx, int *isnum) {
  lua_Number n = lua_tonumber(L, idx);
  if (n == 0 && !lua_isnumber(L, idx)) {
    if (isnum) *isnum = 0;
    return 0;
  }
  else {
    if (isnum) *isnum = 1;
    lua_Unsigned r;
    lua_number2unsigned(r, n);
    return r;
  }
}


static void pushunsigned (lua_State *L, lua_Unsigned u) {
  lua_Number n;
  n = lua_unsigned2number(u);
  lua_pushnumber(L, n);
}

extern void lua_len (lua_State *L, int idx) {
  if (!luaL_callmeta(L, idx, "__len"))
    pushunsigned(L, lua_rawlen(L, idx));
}


extern int lua_compare (lua_State *L, int idx1, int idx2, int op) {
  int i = 0;
  switch (op) {
    case LUA_OPEQ: {
      i = lua_equal(L, idx1, idx2); /* honors __eq */
      break;
    }
    case LUA_OPLT: {
      i = lua_lessthan(L, idx1, idx2); /* honors __lt */
      break;
    }
    case LUA_OPLE: {
      int t1 = lua_type(L, idx1);
      int t2 = lua_type(L, idx2);
      if (t1 == t2 && (t1 == LUA_TNUMBER || t1 == LUA_TSTRING)) {
        i = !lua_lessthan(L, idx2, idx1);
      } else if (luaL_getmetafield(L, idx1, "__le") || luaL_getmetafield(L,
                  idx2, "__le")) {
        lua_pushvalue(L, idx1);
        lua_pushvalue(L, idx2);
        lua_call(L, 2, 1);
        i = lua_toboolean(L, -1);
      } else if (luaL_getmetafield(L, idx1, "__lt")) {
        lua_pushvalue(L, idx2);
        lua_pushvalue(L, idx1);
        lua_call(L, 2, 1);
        i = !lua_toboolean(L, -1);
      } else {
        i = !lua_lessthan(L, idx2, idx1);
      }
    }
    default: api_check(L, 0, "invalid option");
  }
  return i;
}


extern void lua_arith (lua_State *L, int op) {
  int numeric1, numeric2;
  if (op == LUA_OPUNM) {
    luaL_checkany(L, 1);
    numeric1 = 1;
  } else {
    luaL_checkany(L, 2);
    numeric1 = lua_isnumber(L, -2);
  }
  numeric2 = lua_isnumber(L, -1);
  if (numeric1 && numeric2) {
    lua_Number n2 = lua_tonumber(L, -1);
    lua_Number n1;
    if (op == LUA_OPUNM) {
      lua_pop(L, 1);
    } else {
      n1 = lua_tonumber(L, -2);
      lua_pop(L, 2);
    }
    switch (op) {
      case LUA_OPADD: lua_pushnumber(L, luai_numadd(L, n1, n2));
      case LUA_OPSUB: lua_pushnumber(L, luai_numsub(L, n1, n2));
      case LUA_OPMUL: lua_pushnumber(L, luai_nummul(L, n1, n2));
      case LUA_OPDIV: lua_pushnumber(L, luai_numdiv(L, n1, n2));
      case LUA_OPMOD: lua_pushnumber(L, luai_nummod(L, n1, n2));
      case LUA_OPPOW: lua_pushnumber(L, luai_numpow(L, n1, n2));
      case LUA_OPUNM: lua_pushnumber(L, luai_numunm(L, n2));
      default: api_check(L, 0, "invalid option");
    }
  } else {
    switch (op) {
      case LUA_OPADD:
        if (luaL_getmetafield(L, -2, "__add") || luaL_getmetafield(L, -1,
                    "__add")) {
          lua_insert(L, -3);
          lua_call(L, 2, 1);
      } else break;
      case LUA_OPSUB:
        if (luaL_getmetafield(L, -2, "__sub") || luaL_getmetafield(L, -1,
                    "__sub")) {
          lua_insert(L, -3);
          lua_call(L, 2, 1);
      } else break;
      case LUA_OPMUL:
        if (luaL_getmetafield(L, -2, "__mul") || luaL_getmetafield(L, -1,
                    "__mul")) {
          lua_insert(L, -3);
          lua_call(L, 2, 1);
      } else break;
      case LUA_OPDIV:
        if (luaL_getmetafield(L, -2, "__div") || luaL_getmetafield(L, -1,
                    "__div")) {
          lua_insert(L, -3);
          lua_call(L, 2, 1);
      } else break;
      case LUA_OPMOD:
        if (luaL_getmetafield(L, -2, "__mod") || luaL_getmetafield(L, -1,
                    "__mod")) {
          lua_insert(L, -3);
          lua_call(L, 2, 1);
      } else break;
      case LUA_OPPOW:
        if (luaL_getmetafield(L, -2, "__pow") || luaL_getmetafield(L, -1,
                    "__pow")) {
          lua_insert(L, -3);
          lua_call(L, 2, 1);
      } else break;
      case LUA_OPUNM:
        if (luaL_getmetafield(L, -1, "__unm")) {
          lua_insert(L, -2);
          lua_call(L, 1, 1);
      } else break;
      default: api_check(L, 0, "invalid option");
    }
    int idx = numeric1 ? -2 : -1;
    // alternatively, use luaL_error formatting, but then give up automatic
    // function name lookup
    lua_pushfstring(L, "attempt to perform arithmetic on a %s value",
            lua_typename(L, lua_type(L, idx)));
    luaL_argerror(L, (op == LUA_OPUNM) ? 1 : 3 + idx, lua_tostring(L, -1));
  }
}


/* ----- adapted from lua-5.2.0 lauxlib.c: ----- */

extern lua_Unsigned luaL_checkunsigned (lua_State *L, int arg) {
  lua_Number n = lua_tonumber(L, arg);
  if (n == 0) luaL_checktype(L, arg, LUA_TNUMBER);
  lua_Unsigned r;
  lua_number2unsigned(r, n);
  return r;
}

/* define luaL_opt(L,f,n,d)	(lua_isnoneornil(L,(n)) ? (d) : f(L,(n))) */
extern lua_Unsigned luaL_optunsigned (lua_State *L, int narg, lua_Unsigned def)
{
  return luaL_opt(L, luaL_checkunsigned, narg, def);
}


extern const char *luaL_tolstring (lua_State *L, int idx, size_t *len) {
  if (!luaL_callmeta(L, idx, "__tostring")) {  /* no metafield? */
    switch (lua_type(L, idx)) {
      case LUA_TNUMBER:
      case LUA_TSTRING:
        lua_pushvalue(L, idx);
        break;
      case LUA_TBOOLEAN:
        lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
        break;
      case LUA_TNIL:
        lua_pushliteral(L, "nil");
        break;
      default:
        lua_pushfstring(L, "%s: %p", luaL_typename(L, idx),
                                            lua_topointer(L, idx));
        break;
    }
  }
  return lua_tolstring(L, -1, len);
}


extern int luaL_len (lua_State *L, int idx) {
  lua_len(L, idx);
  if (lua_isnumber(L, -1)) {
    int len = lua_tonumber(L, -1);
    lua_pop(L, 1); /* remove object */
    return len;
  } else {
    return luaL_error(L, "object length is not a number");
  }
}


/*
 * Shallowly ensure that stack[idx][field] is a table and push it onto stack.
 *   ~~> true, stack[+1] if already existed; or false, stack[+1] if allocated
 */
extern int luaL_getsubtable (lua_State *L, int idx, const char *field) {
  lua_getfield(L, idx, field);
  if (lua_istable(L, -1)) return 1;  /* table already there */
  else {
    idx = lua_absindex(L, idx);
    lua_pop(L, 1);  /* remove previous result */
    lua_newtable(L);
    lua_pushvalue(L, -1);  /* copy to be left at top */
    lua_setfield(L, idx, field);  /* assign new table to field */
    return 0;  /* false, because did not find table there */
  }
}


/* 
 * Like luaL_checkudata, but returns NULL instead of error when non-matching.
 */
extern void *luaL_testudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get correct metatable */
      if (!lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}


/* 
 * Assign REG[tname] as metatable for stack[-1].
 */
extern void luaL_setmetatable (lua_State *L, const char *tname) {
  luaL_getmetatable(L, tname);
  lua_setmetatable(L, -2);
}


/*
 * Merge functions from list 'l' into table at top - 'nup'; each
 * function gets the 'nup' elements at the top as upvalues.
 * When returning, pops upvalues but leaves table on stack.
 */
extern void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_setfield(L, -(nup + 2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}



/* ----------- for 5.2.0 ---------- */
#elif LUA_VERSION_NUM == 502

extern int luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

#endif

/*
 * Asserts that `libname` has already been loaded and pushes it to stack.
 * This is always exported, but it's only in fiveq.h if LUA_FIVEQ_PLUS.
 */
extern void luaQ_checklib (lua_State *L, const char *libname) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, libname);
    if (lua_isnil(L, -1))
        luaL_error(L, "can't open " LUA_QS " library", libname);
    lua_replace(L, -2);
}
