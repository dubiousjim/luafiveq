/*
 * fiveq.h: elements of Lua 5.2's API backported to lua 5.1.4, and vice-versa
 * includes lua.h, lauxlib.h, unsigned.h
 */


#ifndef FIVEQ_API_H
#define FIVEQ_API_H

/* <luaconf.h> -- uses <limits>,<stddef> */
/* <lua.h> -- uses luaconf,<stddef>,<stdarg> */
#include <lua.h>
/* <lauxlib.h> -- uses lua,<stddef>,<stdio> */
#include <lauxlib.h>
/* <lualib.h> -- uses lua, provides LUA_FILEHANDLE,LUA_{CO,TAB,IO,OS,STR,MATH,DB,LOAD}LIBNAME, luaopen_foo, luaL_openlibs, lua_assert ~~> nop */


# ifdef LUA_FIVEQ_PLUS
#define luaQ_register(L,idx,name,f)  (lua_pushcfunction(L, (f)), lua_setfield(L,(((idx)<0) ? (idx)-1 : (idx)),(name)))

extern const char *luaQ_getdeeptable (lua_State *L, int idx, const char *fields, int szhint, int *existing);
extern const char *luaQ_getdeepvalue (lua_State *L, int idx, const char *fields);
extern const char *luaQ_setdeepvalue (lua_State *L, int idx, const char *fields);
extern void luaQ_getfenv (lua_State *L, int level, const char *fname);
extern void luaQ_setfenv (lua_State *L, int level, const char *fname);
extern void luaQ_checklib (lua_State *L, const char *libname);
# endif



/* ----------- for 5.1.4 ---------- */
#if LUA_VERSION_NUM == 501

#ifdef lua_assert
#define api_check(L,e,msg)	lua_assert((e) && msg)
#else
#define lua_assert(c)		((void)0)
#define api_check(L,e,msg)	luai_apicheck(L, (e) && msg)
#endif

#include "unsigned.h"


/* ----- adapted from lua-5.2.0 lua.h: ----- */

#define lua_pushglobaltable(L) lua_pushvalue(L, LUA_GLOBALSINDEX)
/* lua_getglobal(L, key): pushes _G[key] onto stack in both versions */
/* lua_setglobal(L, key): _G[key] = pops[-1] from stack in both versions */


/* ----- adapted from lua-5.2.0 lapi.c: ----- */

#define lua_absindex(L,idx)	((idx) > 0 || (idx) <= LUA_REGISTRYINDEX ? (idx) : lua_gettop(L) + (idx) + 1)

#define lua_copy(L,from,to)   (lua_pushvalue(L, (from)), lua_replace(L, (to)))

#define lua_getuservalue(L,idx)  (api_check(L, lua_type(L, (idx)) == LUA_TUSERDATA, "userdata expected"), lua_getfenv(L, (idx)))
            
/* the 5.2.0 version can also assign nil, but setfenv and this macro only assign tables */
#define lua_setuservalue(L,idx)  (api_check(L, lua_type(L, (idx)) == LUA_TUSERDATA, "userdata expected"), (void)lua_setfenv(L, (idx)))


#define lua_rawgetp(L,idx,p) (api_check(L, lua_type(L, (idx)) == LUA_TTABLE, "table expected"), lua_pushlightuserdata(L, (p)), lua_rawget(L, (idx))) 

#define lua_rawsetp(L,idx,p) (api_check(L, lua_type(L, (idx)) == LUA_TTABLE, "table expected"), lua_pushlightuserdata(L, (p)), lua_insert(L, -2), lua_rawset(L, (idx)))


extern lua_Number lua_tonumberx (lua_State *L, int idx, int *isnum);
extern lua_Integer lua_tointegerx (lua_State *L, int idx, int *isnum);
extern lua_Unsigned lua_tounsignedx (lua_State *L, int idx, int *isnum);

#define lua_tounsigned(L,idx) lua_tounsignedx(L, (idx), NULL)


#define lua_rawlen  lua_objlen

extern void lua_len (lua_State *L, int idx);

#define LUA_OPEQ	0
#define LUA_OPLT	1
#define LUA_OPLE	2

#define LUA_OPADD	0	/* ORDER TM */
#define LUA_OPSUB	1
#define LUA_OPMUL	2
#define LUA_OPDIV	3
#define LUA_OPMOD	4
#define LUA_OPPOW	5
#define LUA_OPUNM	6

/* the following operations need the math library */
#include <math.h>
#define luai_nummod(L,a,b)	((a) - floor((a)/(b))*(b))
#define luai_numpow(L,a,b)	(pow(a,b))

/* these are quite standard operations */
#define luai_numadd(L,a,b)	((a)+(b))
#define luai_numsub(L,a,b)	((a)-(b))
#define luai_nummul(L,a,b)	((a)*(b))
#define luai_numdiv(L,a,b)	((a)/(b))
#define luai_numunm(L,a)	(-(a))
#define luai_numeq(a,b)		((a)==(b))
#define luai_numlt(L,a,b)	((a)<(b))
#define luai_numle(L,a,b)	((a)<=(b))
#define luai_numisnan(L,a)	(!luai_numeq((a), (a)))

extern int lua_compare (lua_State *L, int idx1, int idx2, int op);
extern void lua_arith (lua_State *L, int op);


/* ----- adapted from lua-5.2.0 lauxlib.c: ----- */

extern lua_Unsigned luaL_checkunsigned (lua_State *L, int arg);
extern lua_Unsigned luaL_optunsigned (lua_State *L, int narg, lua_Unsigned def);

extern const char *luaL_tolstring (lua_State *L, int idx, size_t *len);

extern int luaL_len (lua_State *L, int idx);


/*
 * luaL_Buffer b;
 * char *luaL_buffinitsize(L, &b, sz) is not available in 5.1.4
 *   equiv to: (luaL_buffinit(L, &b), luaL_prepbuffsize(&b, sz) <-- also new)
 * char *luaL_prepbuffer(&b) -- reserves LUAL_BUFFERSIZE
 * -- copy string to that buffer
 * void luaL_addsize(&b, actual_sz)
 * void luaL_pushresultsize(&b, sz)
 *   equiv to: (luaL_addsize(&b, sz), luaL_pushresult(&b))
 */


/*
 * bool lua_getmetatable (lua_State *L, int idx); ~~> true, stack[+1]; or false, stack[unchanged]
 *
 * int luaL_getmetafield (lua_State *L, int idx, const char *field); ~~> true, stack[+1]=field; or false, stack[unchanged]
 */

/*
 * shallowly ensure that stack[idx][field] is a table and push it onto stack
 *   ~~> true, stack[+1] if already existed; or false, stack[+1] if allocated
 */
extern int luaL_getsubtable (lua_State *L, int idx, const char *field);


/*
 * luaL_newmetatable(L, "type"); ~~> true, stack[+1] if allocated; or false, stack[+1]
 *
 * void *luaL_checkudata(L, idx, "type");
 *
 * void luaL_getmetatable(L, "type"); ~~> stack[+1]
 */

/* like luaL_checkudata, but returns NULL instead of error when non-matching */
extern void *luaL_testudata (lua_State *L, int ud, const char *tname);

/* assign registry[tname] as metatable for stack[-1] */
extern void luaL_setmetatable (lua_State *L, const char *tname);



# ifdef LUA_COMPAT_OPENLIB
/* luaL_openlib has already been exported, in place of luaI_openlib */
#  undef luaI_openlib
#  define luaL_openlib luaI_openlib
# endif
extern void luaL_openlib (lua_State *L, const char *libname, const luaL_Reg *l, int nup);

/* 
 * we want luaL_register to use the possibly-enhanced version of luaL_openlib we define here
 * since we are now always exporting luaL_openlib, we can shadow the existing
 * definition of luaL_register with a macro.
 */

#define luaL_register(L,n,l)	(luaL_openlib(L,(n),(l),0))


# ifdef LUA_FIVEQ_PLUS
#  define luaL_pushmodule(L, modname, szhint)  luaQ_pushmodule(L, (modname), (szhint), 0, NULL)
extern void luaQ_pushmodule (lua_State *L, const char *modname, int szhint, int level, const char *caller);
# else
extern void luaL_pushmodule (lua_State *L, const char *modname, int szhint);
# endif

extern void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup);

#define luaL_newlibtable(L,l)   lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)
#define luaL_newlib(L,l)	(luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

extern void luaL_requiref (lua_State *L, const char *libname, lua_CFunction luaopen_lib, int global_idx);


/* ----------- for 5.2.0 ---------- */
#elif LUA_VERSION_NUM == 502

#if defined(LUA_USE_APICHECK)
#include <assert.h>
#define luai_apicheck(L,e)	assert(e)
#else
#define luai_apicheck(L,e)	lua_assert(e)
#endif
#ifndef lua_assert
#define lua_assert(c)		((void)0)
#endif
#define api_check(l,e,msg)	luai_apicheck(l,(e) && msg)


/* internal assertions for in-house debugging */
#if !defined(lua_assert)
# define lua_assert(c)		((void)0)
#endif
#if !defined(luai_apicheck)
# if defined(LUA_USE_APICHECK)
#  include <assert.h>
#  define luai_apicheck(L,e)	assert(e)
# else
#  define luai_apicheck(L,e)	lua_assert(e)
# endif
#endif
#define api_check(l,e,msg)	luai_apicheck(l,(e) && msg)


#define lua_cpcall(L,f,u)  (lua_pushcfunction(L, (f)), lua_pushlightuserdata(L,(u)), lua_pcall(L,1,0,0))

#define lua_objlen(L,i) lua_rawlen(L, (i))
#define lua_strlen(L,i) lua_rawlen(L, (i))
/* void lua_len(L, i) honors __len */
/* int luaL_len(L, i) effectively does luaL_checkint on lua_len(L, i), leaves raw len on stack and returns it converted to int */

#define lua_equal(L,idx1,idx2)		lua_compare(L,(idx1),(idx2),LUA_OPEQ)
#define lua_lessthan(L,idx1,idx2)	lua_compare(L,(idx1),(idx2),LUA_OPLT)

extern int luaL_typerror (lua_State *L, int narg, const char *tname);

/* pre-5.1 stuff */

#define luaL_getn(L,i)    ((int)lua_rawlen(L, (i)))
#define luaL_setn(L,i,j)  ((void)0)  /* no op! */

#define luaL_reg	luaL_Reg
#define lua_open()	luaL_newstate()
#define lua_getregistry(L)	lua_pushvalue(L, LUA_REGISTRYINDEX)
#define lua_getgccount(L)	lua_gc(L, LUA_GCCOUNT, 0)
#define lua_Chunkreader		lua_Reader
#define lua_Chunkwriter		lua_Writer
#define luaL_putchar(B,c)	luaL_addchar(B,c)

#define lua_ref(L,lock) ((lock) ? luaL_ref(L, LUA_REGISTRYINDEX) : \
(lua_pushstring(L, "unlocked references are obsolete"), lua_error(L), 0))

#define lua_unref(L,ref)        luaL_unref(L, LUA_REGISTRYINDEX, (ref))
#define lua_getref(L,ref)       lua_rawgeti(L, LUA_REGISTRYINDEX, (ref))



# ifdef LUA_FIVEQ_PLUS
/* we replace 5.2.0's requiref with a version that will write to stack[global_idx] when global_idx is other than 0 or 1 */
#  define luaL_requiref luaQ_requiref
extern void luaL_requiref (lua_State *L, const char *libname, lua_CFunction luaopen_lib, int global_idx);
# endif

# ifndef LUA_COMPAT_MODULE
#  define luaL_register(L,n,l)	(luaL_openlib(L,(n),(l),0))
extern void luaL_openlib (lua_State *L, const char *libname, const luaL_Reg *l, int nup);
# elif defined(LUA_FIVEQ_PLUS)
/* luaL_openlib has already been defined, but we shadow it */
#  define luaL_openlib luaI_openlib
# endif

# ifdef LUA_FIVEQ_PLUS
/* 
 * if LUA_COMPAT_MODULE, luaL_pushmodule will already have been defined;
 * but it's harmless to shadow it with this macro (behavior is exactly the same).
 */ 
#  define luaL_pushmodule(L, modname, szhint)  luaQ_pushmodule(L, (modname), (szhint), 0, NULL)
extern void luaQ_pushmodule (lua_State *L, const char *modname, int szhint, int level, const char *caller);
# elif !(defined LUA_COMPAT_MODULE)
extern void luaL_pushmodule (lua_State *L, const char *modname, int szhint);
# endif

#endif
#endif
