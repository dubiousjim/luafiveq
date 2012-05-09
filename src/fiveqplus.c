/*
 * fiveqplus.c: elements of Lua 5.2's libraries backported to lua 5.1.4, and vice-versa
 */

#define LUA_FIVEQ_PLUS
#include "fiveq.c"

extern int luaopen_fiveqplus (lua_State *L) {
  return luaopen_fiveq(L);
}
