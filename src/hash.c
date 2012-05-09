/*
 * Hashing library for Lua 5.1.4.
 * Used by tuple.lua and multiset.lua.
 * Exports:
 *      hash.tuple(...)
 *      hash.set(seed,value,[count=1]) ; doesn't check for duplicates
 *      hash.unset(seed,value,[oldcount=1],[newcount=0])
 *      hash.xor(string1, equallengthstring2)
 *      hash.unbox(obj)   ; for gc-able objects, convert to lightuserdata
 *                        ; for others, return unchanged
 *      hash.pointer(obj) ; for gc-able objects and lightuserdata, return "%p"
 *                        ; for others, return nil
 */

#include <string.h>
#include <limits.h>

#include <lua.h>
#include <lauxlib.h>

#include "fiveq.h"
#include "unsigned.h"

// #define _WITH_DPRINTF
// #include <stdio.h>


/* ----- adapted from lua-5.2.0-beta llimits.h: ----- */

/*
** luai_hashnum is a macro to hash a lua_Number value into an integer.
** The hash must be deterministic and give reasonable values for
** both small and large values (outside the range of integers).
*/

#if defined(LUA_IEEE754TRICK)

#define luai_hashnum(i,n)  \
  { volatile union luai_Cast2 u; u.l_d = (n) + 1.0;  /* avoid -0 */ \
    (i) = u.l_p[0] + u.l_p[1]; }  /* add double bits for his hash */

#else

#include <float.h>
#include <math.h>

#define luai_hashnum(i,n) { int e;  \
  n = frexp((lua_Number)n, &e) * (lua_Number)(INT_MAX - DBL_MAX_EXP);  \
  lua_number2int(i, n); i += e; }

#endif



typedef unsigned char byte;
typedef const byte *byteptr;
typedef const unsigned short *ushortptr;
#define IntPoint(p)  ((unsigned int)(long)(p))
#define hashbyte(h, b) ((h) ^ (((h)<<5) + ((h)>>2) + ((byte)(b))))
#define rol(x, shift) (((x) << (shift)) | ((x) >> (sizeof(int)*8 - (shift))))

#define hashshort(h, s) ((*(ushortptr)(s) > 0xff) ? (hashbyte(hashbyte((h), ((byteptr) (s))[0]), ((byteptr) (s))[1])) : (hashbyte((h), *(ushortptr)(s))))  

/*
#define mask UINT_MAX
#define numints  (int)(sizeof(lua_Number)/sizeof(int))
#define luai_numeq(a,b)  ((a)==(b))
#define hash32(h, i) (hashbyte(hashbyte(hashbyte(hashbyte((h), ((byteptr) (i))[0]), ((byteptr) (i))[1]), ((byteptr) (i))[2]), ((byteptr) (i))[3]))
#define numbytes  (int)(sizeof(int)/sizeof(byte))
#define hashshort(h, s) (hashbyte(hashbyte((h), (s) & 0xff), (s) >> 8))
*/


static lua_Unsigned gethash (lua_State *L, int idx) {
  // const TValue *v = L->top - 1;
  switch (lua_type(L, idx)) {
    case LUA_TNUMBER: {
      lua_Number n = lua_tonumber(L, idx); // nvalue(v)
/*
      if (luai_numeq(n, 0)) {
        [* avoid problems with -0 *]
        return 0x1f;
      } else {
        lua_Unsigned a[numints];
        int j;
        memcpy(a, &n, sizeof(a));
        for (j = 1; j < numints; j++) a[0] += a[j];
        return a[0] % mask;
      }
*/
      int i;
      luai_hashnum(i, n);
      if (i < 0) {
        if (((unsigned int) i) == 0u - i) /* use unsigned to avoid overflows */
          i = 0x1c; /* handle INT_MIN */
        i = -i; /* must be positive */
      } else if (i == 0) {
        i = 0x1b;
      }
      return i; // % mask
    }
    case LUA_TSTRING: {
      /*
      TString *p = ((TString *) lua_tostring(L, idx)) - 1; // rawtsvalue(v)
      lua_Unsigned h = p->tsv.hash;
      */
      size_t len, j;
      const char *str = lua_tolstring(L, idx, &len);
      lua_Unsigned seed = len;
      if (seed == 0) {
        return 0x20;
      } else {
        size_t step = (len>>5) + 1; /* if string is too long, don't hash all its chars */
        for (j=len; j>=step; j-=step) /* compute hash */
            seed = hashbyte(seed, str[j-1]);
        return seed; // & mask
      }
    }
    case LUA_TBOOLEAN: {
      int h = lua_toboolean(L, idx); // bvalue(v)
      return h ? 0x1f : 0x1e;
    }
    case LUA_TNIL: {
      return 0x1d;
    }
    default: {
      const void *p = lua_topointer(L, idx);  // TLIGHTUSERDATA: pvalue(v) else: gcvalue(v)
      return IntPoint(p); // % mask
    }
  }
}

/*
static int rawhash (lua_State *L) {
    luaL_checkany(L, 1);
    lua_Unsigned h = gethash(L, 1);
    // lua_settop(L, 0);
    lua_pushinteger(L, h);
    return 1;
}
*/

static int tuplehash (lua_State *L) {
    int nargs = lua_gettop(L);
    lua_Unsigned seed = (nargs > 0) ? nargs - 1 : 0;
    int j;
    lua_Unsigned h;
    for (j=1; j<=nargs; j++) {
        h = rol(gethash(L, j), j - 1);
        seed = seed ^ h;
    }
    // lua_settop(L, 0);
    lua_pushnumber(L, (lua_Number)seed);
    return 1;
}

static int sethash (lua_State *L) {
    /* arg1=seed, arg2=value, [count=1] */
    lua_Unsigned seed = luaL_checkint(L, 1);
    if (lua_isnoneornil(L, 2)) {
      return luaL_argerror(L, 2, "non-nil value expected");
    }
    int count = luaL_optint(L, 3, 1);
    luaL_argcheck(L, 0 < count && count <= 0xffff, 3, "out of range");
    lua_Unsigned h = gethash(L, 2);
    unsigned short w = count;
    h = hashshort(h, &w);
    seed = seed ^ h;
    // lua_settop(L, 0);
    lua_pushnumber(L, (lua_Number)seed);
    return 1;
}

static int unsethash (lua_State *L) {
    /* arg1=seed, arg2=value, [oldcount=1, [newcount=0]] */
    lua_Unsigned seed = luaL_checkint(L, 1);
    if (lua_isnoneornil(L, 2)) {
      return luaL_argerror(L, 2, "non-nil value expected");
    }
    int count = luaL_optint(L, 3, 1);
    luaL_argcheck(L, 0 < count && count <= 0xffff, 3, "out of range");
    int newcount = luaL_optint(L, 4, 0);
    luaL_argcheck(L, 0 <= newcount && newcount <= 0xffff, 4, "out of range");
    if (count == newcount) {
      lua_settop(L, 1);
    } else {
      lua_Unsigned h = gethash(L, 2);
      unsigned short w;
      if (count != 0) {
        w = (unsigned short)count;
        seed = seed ^ hashshort(h, &w);
      }
      if (newcount != 0) {
        w = (unsigned short)newcount;
        seed = seed ^ hashshort(h, &w);
      }
      // lua_settop(L, 0);
      lua_pushnumber(L, (lua_Number)seed);
    }
    return 1;
}


/**
*  From Ierusalimschy's md5lib.
*  X-Or. Does a bit-a-bit exclusive-or of two strings.
*  @param s1: arbitrary binary string.
*  @param s2: arbitrary binary string with same length as s1.
*  @return  a binary string with same length as s1 and s2,
*   where each bit is the exclusive-or of the corresponding bits in s1-s2.
*/
static int ex_or (lua_State *L) {
  size_t l1, l2;
  const char *s1 = luaL_checklstring(L, 1, &l1);
  const char *s2 = luaL_checklstring(L, 2, &l2);
  luaL_Buffer b;
  luaL_argcheck( L, l1 == l2, 2, "lengths must be equal" );
  luaL_buffinit(L, &b);
  while (l1--) luaL_addchar(&b, (*s1++)^(*s2++));
  luaL_pushresult(&b);
  return 1;
}


static int unbox (lua_State *L) {
    const void *p = lua_topointer(L, 1);
    if (p != NULL) {
        lua_pushlightuserdata(L, p);
    }
    else {
        lua_pushvalue(L, 1);
    }
    return 1;
}

static int pointer (lua_State *L) {
    const void *p = lua_topointer(L, 1);
    if (p != NULL) { // && !lua_islightuserdata(L, 1)) {
        lua_pushfstring(L, "%p", p);
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}



static const luaL_Reg hlib[] = {
  /* {"raw", rawhash}, */
  {"tuple", tuplehash},
  {"set", sethash},
  {"unset", unsethash},
  {"xor", ex_or},
  {"unbox", unbox},
  {"pointer", pointer},
  {NULL, NULL}
};

extern int luaopen_fiveq_hash (lua_State *L) {
    lua_newtable(L);
    // luaL_setfuncs(L, hlib, 0); // or luaL_openlib(L, NULL, hlib, 0);
    luaL_openlib(L, "hash", hlib, 0);
    return 1;
}
