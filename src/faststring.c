/*
 * Faster string routines
 *
 *  Half of the time when doing string search and replace we do not require the
 *  regular expression functionality that comes with the string.gsub function. I
 *  have done a benchmark test with this new function and found it to be overall
 *  more than twice faster than gsub. The speed difference can be easily seen when
 *  doing search and replace on field values of thousands of records.
 *
 */

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "fiveq.h"


/* ----- from lstrlib.c ----- */

static const char *lmemfind (const char *s1, size_t l1,
                               const char *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const char *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}

static int str_replace(lua_State *L) {
    size_t l1, l2, l3;
    const char *src = luaL_checklstring(L, 1, &l1);
    const char *p = luaL_checklstring(L, 2, &l2);
    const char *p2 = luaL_checklstring(L, 3, &l3);
    int count;
    if (lua_isnoneornil(L, 4)) {
        count = -1;
    }
    else {
        count = luaL_checkint(L, 4);
        luaL_argcheck(L, count >= 0, 4, "negative count");
        if (count == 0) {
           lua_settop(L, 1);
           return 1; 
        }
    }

    const char *s2 = NULL;
    int n = 0;
    int init = 0;

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    while (count--) {
        s2 = lmemfind(src+init, l1-init, p, l2);
        if (s2) {
            luaL_addlstring(&b, src+init, s2-(src+init));
            luaL_addlstring(&b, p2, l3);
            init = init + (s2-(src+init)) + l2;
            n++;
        } else {
            luaL_addlstring(&b, src+init, l1-init);
            break;
        }
    }

    if (s2)
        luaL_addlstring(&b, src+init, l1-init);

    luaL_pushresult(&b);
    lua_pushnumber(L, (lua_Number)n);  /* number of substitutions */
    return 2;
}


static int str_starts(lua_State *L) {
    size_t slen, plen;
    const char *s, *p, *q;
    s = luaL_checklstring(L, 1, &slen);
    int i, j;
    int top = lua_gettop(L);
    luaL_argcheck(L, top > 1, 2, "string expected");
    if (slen == 0) {
        luaL_checkstring(L, 2);
        lua_pushvalue(L, 2);
        return 1;
    }
    for (i=2; i <= top; i++) {
        p = luaL_checklstring(L, i, &plen);
        if (plen > slen)
            continue;
        int matches = 1;
        for (j = 0, q = s; (size_t)j < plen; j++, q++, p++) {
            if (*q != *p) {
                matches = 0;
                break;
            }
        }
        if (matches) {
            lua_pushvalue(L, i);
            return 1;
        }
    }
    lua_pushboolean(L, 0);
    return 1;
}


static int str_ends(lua_State *L) {
    size_t slen, plen;
    const char *s, *p, *q;
    s = luaL_checklstring(L, 1, &slen);
    int i, j;
    int top = lua_gettop(L);
    luaL_argcheck(L, top > 1, 2, "string expected");
    if (slen == 0) {
        luaL_checkstring(L, 2);
        lua_pushvalue(L, 2);
        return 1;
    }
    for (i=2; i <= top; i++) {
        p = luaL_checklstring(L, i, &plen);
        if (plen > slen)
            continue;
        int matches = 1;
        for (j = 0, q = s + slen - 1, p = p + plen - 1; (size_t)j < plen; j++, q--, p--) {
            if (*q != *p) {
                matches = 0;
                break;
            }
        }
        if (matches) {
            lua_pushvalue(L, i);
            return 1;
        }
    }
    lua_pushboolean(L, 0);
    return 1;
}

static const luaL_Reg R[] =
{
        { "gsubplain",  str_replace},
        { "starts",     str_starts},
        { "ends",       str_ends},
        { NULL,		NULL	}
};

extern int luaopen_faststring (lua_State *L) {
    luaQ_checklib(L, LUA_STRLIBNAME);
    luaL_setfuncs(L, R, 0);
    return 0;
}
