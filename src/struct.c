
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

#define LUA_FIVEQ_PLUS
#include "fiveq.h"

#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif


/*
** {======================================================
** Library for packing/unpacking structures.
** $Id: struct.c 20405 2009-10-15 07:06:45Z fm $
** See Copyright Notice at the end of this file
** =======================================================
*/
/*
** Valid formats:
** > - big endian
** < - little endian
** ![num] - alignment
** x[num]   - pad num bytes, default 1
** X[num]   - pad to num align, default MAXALIGN
** b/B - signed/unsigned byte
** h/H - signed/unsigned short
** l/L - signed/unsigned long
** i/I[num] - signed/unsigned integer with size 'n' (default is size of int)
** cn - sequence of 'n' chars (from/to a string); when packing, n==0 means
        the whole string; when unpacking, n==0 means use the previous
        read number as the string length
** s        - zero-terminated string
** f        - float
** d        - double
** ' '      - ignored
** '(' ')'  - stop assigning items. ')' start assigning (padding when packing)
** '='      - return current position / offset
*/


/* is 'x' a power of 2? */
#define isp2(x)		((x) > 0 && ((x) & ((x) - 1)) == 0)

/* dummy structure to get alignment requirements */
struct cD {
  char c;
  double d;
};


#define PADDING		(sizeof(struct cD) - sizeof(double))
#define MAXALIGN  	(PADDING > sizeof(int) ? PADDING : sizeof(int))


/* endian options */
#define BIG	0
#define LITTLE	1


static union {
  int dummy;
  char endian;
} const native = {1};

static union dblswap {
  long long dummy;
  double dbl;
  long l[2];
} const swaptest = {0x00000000c0000000};


typedef struct Header {
  int endian;
  int align;
  bool noassign;
  bool dblswap;
} Header;


static size_t getnum (const char **fmt, size_t df) {
  if (!isdigit(**fmt))  /* no number? */
    return df;  /* return default value */
  else {
    size_t a = 0;
    do {
      a = a*10 + *((*fmt)++) - '0';
    } while (isdigit(**fmt));
    return a;
  }
}


#define defaultoptions(h)	((h)->endian = native.endian, (h)->align = 1, \
                                (h)->noassign = false, (h)->dblswap = false)



static size_t optsize (lua_State *L, char opt, const char **fmt) {
  switch (opt) {
    case 'B': case 'b': return sizeof(char);
    case 'H': case 'h': return sizeof(short);
    case 'L': case 'l': return sizeof(long);
    case 'f':  return sizeof(float);
    case 'd':  return sizeof(double);
    case 'x': return getnum(fmt, 1);
    case 'X': return getnum(fmt, MAXALIGN);
    case 'c': return getnum(fmt, 1);
    case 's':
    case ' ':
    case '<':
    case '>':
    case '(':
    case ')':
    case '!':
    case '=':
              return 0;
    case 'i': case 'I': {
      int sz = getnum(fmt, sizeof(int));
      if (!isp2(sz))
        luaL_error(L, "integer size %d is not a power of 2", sz);
      return sz;
    }
    default: {
      const char *msg = lua_pushfstring(L, "invalid format option [%c]", opt);
      return luaL_argerror(L, 1, msg);
    }
  }
}


static int gettoalign (size_t len, Header *h, int opt, size_t size) {
  if (size == 0 || opt == 'c' || opt == 's') return 0;
  if (size > (size_t)h->align) size = h->align;  /* respect max. alignment */
  return  (size - (len & (size - 1))) & (size - 1);
}


static void commoncases (lua_State *L, int opt, const char **fmt, Header *h) {
  switch (opt) {
    case  ' ': return;  /* ignore white spaces */
    case '>': h->endian = BIG; (h)->dblswap = (swaptest.dbl == -2); return;
    case '<': h->endian = LITTLE; (h)->dblswap = (swaptest.dbl == -2); return;
    case '(': h->noassign = true; return;
    case ')': h->noassign = false; return;
    case '!': {
      int a = getnum(fmt, MAXALIGN);
      if (!isp2(a))
        luaL_error(L, "alignment %d is not a power of 2", a);
      h->align = a;
      return;
    }
    default: assert(0);
  }
}


static void putinteger (lua_State *L, luaL_Buffer *b, int arg, int endian,
                        int size) {
  lua_Number n = luaL_checknumber(L, arg);
  unsigned long long value;
  if (n < (lua_Number)LLONG_MAX)
    value = (long long)n;
  else
    value = (unsigned long long)n;
  if (endian == LITTLE) {
    int i;
    for (i = 0; i < size; i++)
      luaL_addchar(b, (value >> 8*i) & 0xff);
  }
  else {
    int i;
    for (i = size - 1; i >= 0; i--)
      luaL_addchar(b, (value >> 8*i) & 0xff);
  }
}


static void correctbytes (char *b, int size, int endian) {
  if (endian != native.endian) {
    int i = 0;
    while (i < --size) {
      char temp = b[i];
      b[i++] = b[size];
      b[size] = temp;
    }
  }
}


static int b_pack (lua_State *L) {
  luaL_Buffer b;
  const char *fmt = luaL_checkstring(L, 1);
  Header h;
  int poscnt = 0;
  int posBuf[10];
  int arg = 2;
  size_t totalsize = 0;
  defaultoptions(&h);
  lua_pushnil(L);  /* mark to separate arguments from string buffer */
  luaL_buffinit(L, &b);
  while (*fmt != '\0') {
    int opt = *fmt++;
    size_t size = optsize(L, opt, &fmt);
    int toalign = gettoalign(totalsize, &h, opt, size);
    totalsize += toalign;
    while (toalign-- > 0) luaL_addchar(&b, '\0');
    if (opt == 'X')
        size = 0;
    if (h.noassign && size)
        opt = 'x';
    switch (opt) {
      case 'b': case 'B': case 'h': case 'H':
      case 'l': case 'L': case 'i': case 'I': {  /* integer types */
        putinteger(L, &b, arg++, h.endian, size);
        break;
      }
      case 'x': case 'X': {
        size_t l = size;
        while (l-- > 0) luaL_addchar(&b, '\0');
        break;
      }
      case 'f': {
        float f = (float)luaL_checknumber(L, arg++);
        correctbytes((char *)&f, size, h.endian);
        luaL_addlstring(&b, (char *)&f, size);
        break;
      }
      case 'd': {
        union dblswap d;
        d.dbl = luaL_checknumber(L, arg++);
        correctbytes((char *)&d, size, h.endian);
        if (h.dblswap) {
            long tmp = d.l[0];
            d.l[0] = d.l[1];
            d.l[1] = tmp;
        }
        luaL_addlstring(&b, (char *)&d, size);
        break;
      }
      case 'c': case 's': {
        size_t l;
        const char *s = luaL_checklstring(L, arg++, &l);
        if (size == 0) size = l;
        luaL_argcheck(L, l >= (size_t)size, arg, "string too short");
        luaL_addlstring(&b, s, size);
        if (opt == 's') {
          luaL_addchar(&b, '\0');  /* add zero at the end */
          size++;
        }
        break;
      }
      case '=': {
        if (poscnt < (int)(sizeof(posBuf)/sizeof(posBuf[0])))
            posBuf[poscnt++] = totalsize + 1;
        break;
      }
      default: commoncases(L, opt, &fmt, &h);
    }
    totalsize += size;
  }
  luaL_pushresult(&b);
  for (arg = 0; arg < poscnt; arg++)
      lua_pushinteger(L, posBuf[arg]);
  return poscnt + 1;
}


static lua_Number getinteger (const char *buff, int endian,
                        int issigned, int size) {
  unsigned long l = 0;
  if (endian == BIG) {
    int i;
    for (i = 0; i < size; i++)
      l |= (unsigned long)(unsigned char)buff[size - i - 1] << (i*8);
  }
  else {
    int i;
    for (i = 0; i < size; i++)
      l |= (unsigned long)(unsigned char)buff[i] << (i*8);
  }
  if (!issigned)
    return (lua_Number)l;
  else {  /* signed format */
    unsigned long mask = ~(0UL) << (size*8 - 1);
    if (l & mask)  /* negative value? */
      l |= mask;  /* signal extension */
    return (lua_Number)(long)l;
  }
}

static int b_size (lua_State *L) {
  Header h;
  const char *fmt = luaL_checkstring(L, 1);
  size_t totalsize = 0;
  defaultoptions(&h);
  while (*fmt) {
    int opt = *fmt++;
    switch(opt) {
      case  ' ': break;  /* ignore white spaces */
      case '>': h.endian = BIG; break;
      case '<': h.endian = LITTLE; break;
      case '!': {
        int a = getnum(&fmt, MAXALIGN);
        if (!isp2(a))
          luaL_error(L, "alignment %d is not a power of 2", a);
        h.align = a;
        break;
      }
      default: {
        size_t size = optsize(L, opt, &fmt);
        totalsize += gettoalign(totalsize, &h, opt, size);
        if (size == 0)
          luaL_error(L, "options 'c0' and 's' have undefined sizes");
        totalsize += size;
      }
    }
  }
  lua_pushnumber(L, totalsize);
  return 1;
}

static int b_unpack (lua_State *L) {
  Header h;
  const char *fmt = luaL_checkstring(L, 1);
  size_t ld;
  const char *data;
  size_t pos;
  lua_Number lastnum = 0;
  int lastassign = -1;
  int top;

#define pushnumber(n) { lastnum = (lua_Number)(n); lastassign = !h.noassign; \
    if (!h.noassign) { if (lua_checkstack(L, ++top)) \
        lua_pushnumber(L, (lua_Number)(n)); \
        else luaL_error(L, "too many results to unpack"); } }

  if (lua_isuserdata(L, 2)) {
    data = (const char*)lua_touserdata(L, 2);
    ld = (size_t)luaL_checkinteger(L, 3);
    pos = luaL_optinteger(L, 4, 1) - 1;
  } else {
    data = luaL_checklstring(L, 2, &ld);
    pos = luaL_optinteger(L, 3, 1) - 1;
  }
  defaultoptions(&h);
  lua_settop(L, 2);
  top = 3; /* reserve space for stop position at end */
  while (*fmt) {
    int opt = *fmt++;
    size_t size = optsize(L, opt, &fmt);
    pos += gettoalign(pos, &h, opt, size);
    luaL_argcheck(L, pos+size <= ld, 2, "data string too short");
    if (opt == 'X')
        size = 0;
    switch (opt) {
      case 'b': case 'B': case 'h': case 'H':
      case 'l': case 'L': case 'i':  case 'I': {  /* integer types */
        int issigned = islower(opt);
        lua_Number res = getinteger(data+pos, h.endian, issigned, size);
        pushnumber(res);
        break;
      }
      case 'x': case 'X': {
        break;
      }
      case 'f': {
        float f;
        memcpy(&f, data+pos, size);
        correctbytes((char *)&f, sizeof(f), h.endian);
        pushnumber(f);
        break;
      }
      case 'd': {
        union dblswap d;
        memcpy(&d, data+pos, size);
        correctbytes((char *)&d, sizeof(d), h.endian);
        if (h.dblswap) {
            long tmp = d.l[0];
            d.l[0] = d.l[1];
            d.l[1] = tmp;
        }
        pushnumber(d.dbl);
        break;
      }
      case 'c': {
        if (size == 0) {
          if (lastassign < 0) {
            /* no cached lastnum available */
            luaL_error(L, "format 'c0' needs a previous size");
          } else if (lastnum < 0) {
            luaL_error(L, "format 'c0' needs a size >= 0");
          }
          size = lastnum;
          if (lastassign) {
          lua_pop(L, 1);
            top--;
          }
          luaL_argcheck(L, pos+size <= ld, 2, "data string too short");
        }
        /* we clear cached lastnum */
        lastassign = -1;
        if (!h.noassign) {
            if (lua_checkstack(L, ++top))
        lua_pushlstring(L, data+pos, size);
            else
                luaL_error(L, "too many results to unpack");
        }
        break;
      }
      case 's': {
        const char *e = (const char *)memchr(data+pos, '\0', ld - pos);
        if (e == NULL)
          luaL_error(L, "unfinished string in data");
        size = (e - (data+pos)) + 1;
        /* we clear cached lastnum */
        lastassign = -1;
        if (!h.noassign) {
            if (lua_checkstack(L, ++top))
        lua_pushlstring(L, data+pos, size - 1);
            else
                luaL_error(L, "too many results to unpack");
        }
        break;
      }
      case '=': {
        /* we clear cached lastnum */
        lastassign = -1;
        if (lua_checkstack(L, ++top))
        lua_pushinteger(L, pos + 1);
        else
            luaL_error(L, "too many results to unpack");
        break;
      }
      default: commoncases(L, opt, &fmt, &h);
    }
    pos += size;
  }
  /* push stop position */
  lua_pushinteger(L, pos + 1);
  return top - 2;
}

/* }====================================================== */



static const struct luaL_Reg slib[] = {
  {"pack", b_pack},
  {"unpack", b_unpack},
  {"size", b_size},
  {NULL, NULL}
};


LUALIB_API int luaopen_fiveq_struct (lua_State *L) {
  luaL_newlib(L, slib);
  return 1;
}



/******************************************************************************
* Copyright (C) 2010 Lua.org, PUC-Rio.  All rights reserved.
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
******************************************************************************/
