/*
 * This is the bit32 library (lbitlib.c) from lua 5.2.0,
 * backported to lua 5.1.
 *
 * version 5.2.0-backport2
 *
 * Copyright (C) 1994-2011 Lua.org, PUC-Rio.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* ===== begin modifications to lbitlib.c ===== */

#include "fiveq.h"
#include "unsigned.h"

/* also in api.c, but not exported */
static void pushunsigned (lua_State *L, lua_Unsigned u) {
  lua_Number n;
  n = lua_unsigned2number(u);
  lua_pushnumber(L, n);
}

/* ===== end modifications to lbitlib.c ===== */

/* number of bits to consider in a number */
#if !defined(LUA_NBITS)
#define LUA_NBITS	32
#endif


#define ALLONES		(~(((~(lua_Unsigned)0) << (LUA_NBITS - 1)) << 1))

/* macro to trim extra bits */
#define trim(x)		((x) & ALLONES)


/* builds a number with 'n' ones (1 <= n <= LUA_NBITS) */
#define mask(n)		(~((ALLONES << 1) << ((n) - 1)))


typedef lua_Unsigned b_uint;

static b_uint andaux (lua_State *L) {
  int i, n = lua_gettop(L);
  b_uint r = ~(b_uint)0;
  for (i = 1; i <= n; i++)
    r &= luaL_checkunsigned(L, i);
  return trim(r);
}


static int b_and (lua_State *L) {
  b_uint r = andaux(L);
  pushunsigned(L, r);
  return 1;
}


static int b_test (lua_State *L) {
  b_uint r = andaux(L);
  lua_pushboolean(L, r != 0);
  return 1;
}


static int b_or (lua_State *L) {
  int i, n = lua_gettop(L);
  b_uint r = 0;
  for (i = 1; i <= n; i++)
    r |= luaL_checkunsigned(L, i);
  pushunsigned(L, trim(r));
  return 1;
}


static int b_xor (lua_State *L) {
  int i, n = lua_gettop(L);
  b_uint r = 0;
  for (i = 1; i <= n; i++)
    r ^= luaL_checkunsigned(L, i);
  pushunsigned(L, trim(r));
  return 1;
}


static int b_not (lua_State *L) {
  b_uint r = ~luaL_checkunsigned(L, 1);
  pushunsigned(L, trim(r));
  return 1;
}


static int b_shift (lua_State *L, b_uint r, int i) {
  if (i < 0) {  /* shift right? */
    i = -i;
    r = trim(r);
    if (i >= LUA_NBITS) r = 0;
    else r >>= i;
  }
  else {  /* shift left */
    if (i >= LUA_NBITS) r = 0;
    else r <<= i;
    r = trim(r);
  }
  pushunsigned(L, r);
  return 1;
}


static int b_lshift (lua_State *L) {
  return b_shift(L, luaL_checkunsigned(L, 1), luaL_checkint(L, 2));
}


static int b_rshift (lua_State *L) {
  return b_shift(L, luaL_checkunsigned(L, 1), -luaL_checkint(L, 2));
}


static int b_arshift (lua_State *L) {
  b_uint r = luaL_checkunsigned(L, 1);
  int i = luaL_checkint(L, 2);
  if (i < 0 || !(r & ((b_uint)1 << (LUA_NBITS - 1))))
    return b_shift(L, r, -i);
  else {  /* arithmetic shift for 'negative' number */
    if (i >= LUA_NBITS) r = ALLONES;
    else
      r = trim((r >> i) | ~(~(b_uint)0 >> i));  /* add signal bit */
    pushunsigned(L, r);
    return 1;
  }
}


static int b_rot (lua_State *L, int i) {
  b_uint r = luaL_checkunsigned(L, 1);
  i &= (LUA_NBITS - 1);  /* i = i % NBITS */
  r = trim(r);
  r = (r << i) | (r >> (LUA_NBITS - i));
  pushunsigned(L, trim(r));
  return 1;
}


static int b_lrot (lua_State *L) {
  return b_rot(L, luaL_checkint(L, 2));
}


static int b_rrot (lua_State *L) {
  return b_rot(L, -luaL_checkint(L, 2));
}


/*
** get field and width arguments for field-manipulation functions,
** checking whether they are valid
*/
static int fieldargs (lua_State *L, int farg, int *width) {
  int f = luaL_checkint(L, farg);
  int w = luaL_optint(L, farg + 1, 1);
  luaL_argcheck(L, 0 <= f, farg, "field cannot be negative");
  luaL_argcheck(L, 0 < w, farg + 1, "width must be positive");
  if (f + w > LUA_NBITS)
    luaL_error(L, "trying to access non-existent bits");
  *width = w;
  return f;
}


static int b_extract (lua_State *L) {
  int w;
  b_uint r = luaL_checkunsigned(L, 1);
  int f = fieldargs(L, 2, &w);
  r = (r >> f) & mask(w);
  pushunsigned(L, r);
  return 1;
}


static int b_replace (lua_State *L) {
  int w;
  b_uint r = luaL_checkunsigned(L, 1);
  b_uint v = luaL_checkunsigned(L, 2);
  int f = fieldargs(L, 3, &w);
  int m = mask(w);
  v &= m;  /* erase bits outside given width */
  r = (r & ~(m << f)) | (v << f);
  pushunsigned(L, r);
  return 1;
}


static const luaL_Reg bitlib[] = {
  {"arshift", b_arshift},
  {"band", b_and},
  {"bnot", b_not},
  {"bor", b_or},
  {"bxor", b_xor},
  {"btest", b_test},
  {"extract", b_extract},
  {"lrotate", b_lrot},
  {"lshift", b_lshift},
  {"replace", b_replace},
  {"rrotate", b_rrot},
  {"rshift", b_rshift},
  {NULL, NULL}
};


extern int luaopen_fiveq_bitlib (lua_State *L) {
    luaL_newlib(L, bitlib);
    return 1;
}
