/*
 * unsigned.h: elements of Lua 5.2's API backported to Lua 5.1, and vice-versa
 * used by bitlib.c, hash.c, and fiveq.c
 */

#ifndef FIVEQ_UNSIGNED_H
#define FIVEQ_UNSIGNED_H

#include "fiveq.h"

/* ----- adapted from lua-5.2.0-beta luaconf.h: ----- */

#if LUA_VERSION_NUM == 501                              /* { */

/* #define LUA_INT32  LUAI_INT32 */

/*
 * @@ LUA_UNSIGNED is the integral type used by lua_pushunsigned/lua_tounsigned.
 * ** It must have at least 32 bits.
 * */
/* #define LUA_UNSIGNED    unsigned LUA_INT32 */

#if defined(LUA_NUMBER_DOUBLE) && !defined(LUA_ANSI)    /* { */

/* On a Microsoft compiler on a Pentium, use assembler to avoid clashes
   with a DirectX idiosyncrasy */
#if defined(LUA_WIN) && defined(_MSC_VER) && defined(_M_IX86)	/* { */

# define MS_ASMTRICK

#else                           /* }{ */
/* the next definition uses a trick that should work on any machine
   using IEEE754 with a 32-bit integer type */

#define LUA_IEEE754TRICK

/*
@@ LUA_IEEEENDIAN is the endianness of doubles in your machine
** (0 for little endian, 1 for big endian); if not defined, Lua will
** check it dynamically.
*/
/* check for known architectures */
#if defined(__i386__) || defined(__i386) || defined(__X86__) || \
    defined (__x86_64)
#define LUA_IEEEENDIAN  0
#elif defined(__POWERPC__) || defined(__ppc__)
#define LUA_IEEEENDIAN  1
#endif

#endif                          /* } */

#endif                  /* } */


/* ----- from lua-5.2.0-beta lua.h: ----- */

/* typedef LUA_UNSIGNED lua_Unsigned; */

#endif              /* } */

/* ----- adapted from lua-5.2.0-beta llimits.h: ----- */

/*
** lua_number2int is a macro to convert lua_Number to int.
** lua_number2integer is a macro to convert lua_Number to lua_Integer.
** lua_number2unsigned is a macro to convert a lua_Number to a lua_Unsigned.
** lua_unsigned2number is a macro to convert a lua_Unsigned to a lua_Number.
*/

#if defined(MS_ASMTRICK)        /* { */
/* trick with Microsoft assembler for X86 */

#if defined(lua_number2int)
#undef lua_number2int
#endif
#if defined(lua_number2integer)
#undef lua_number2integer
#endif
#define lua_number2int(i,n)  __asm {__asm fld n   __asm fistp i}
#define lua_number2integer(i,n)		lua_number2int(i, n)
#define lua_number2unsigned(i,n)  \
  {__int64 l; __asm {__asm fld n   __asm fistp l} i = (unsigned int)l;}

#elif defined(LUA_IEEE754TRICK)         /* }{ */
/* the next trick should work on any machine using IEEE754 with
   a 32-bit integer type */

union luai_Cast2 { double l_d; LUA_INT32 l_p[2]; };

#if !defined(LUA_IEEEENDIAN)    /* { */
#define LUAI_EXTRAIEEE  \
  static const union luai_Cast2 ieeeendian = {-(33.0 + 6755399441055744.0)};
#define LUA_IEEEENDIAN          (ieeeendian.l_p[1] == 33)
#else
#define LUAI_EXTRAIEEE          /* empty */
#endif                          /* } */

#define lua_number2int32(i,n,t) \
  { LUAI_EXTRAIEEE \
    volatile union luai_Cast2 u; u.l_d = (n) + 6755399441055744.0; \
    (i) = (t)u.l_p[LUA_IEEEENDIAN]; }

#if defined(lua_number2int)
#undef lua_number2int
#endif
#if defined(lua_number2integer)
#undef lua_number2integer
#endif
#define lua_number2int(i,n)		lua_number2int32(i, n, int)
#define lua_number2integer(i,n)		lua_number2int32(i, n, lua_Integer)
#define lua_number2unsigned(i,n)        lua_number2int32(i, n, lua_Unsigned)

#endif                          /* } */


/* the following definitions always work, but may be slow */

#if !defined(lua_number2int)
#define lua_number2int(i,n)	((i)=(int)(n))
#endif

#if !defined(lua_number2integer)
#define lua_number2integer(i,n)	((i)=(lua_Integer)(n))
#endif

#if !defined(lua_number2unsigned)       /* { */
/* the following definition assures proper modulo behavior */
#if defined(LUA_NUMBER_DOUBLE)
#include <math.h>
#define SUPUNSIGNED     ((lua_Number)(~(lua_Unsigned)0) + 1)
#define lua_number2unsigned(i,n)  \
        ((i)=(lua_Unsigned)((n) - floor((n)/SUPUNSIGNED)*SUPUNSIGNED))
#else
#define lua_number2unsigned(i,n)        ((i)=(lua_Unsigned)(n))
#endif
#endif                          /* } */

#if !defined(lua_unsigned2number)
/* on several machines, coercion from unsigned to double is slow,
   so it may be worth to avoid */
#define lua_unsigned2number(u)  \
    (((u) <= (lua_Unsigned)INT_MAX) ? (lua_Number)(int)(u) : (lua_Number)(u))
#endif

#endif
