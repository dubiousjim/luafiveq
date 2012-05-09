/*
 * fivetwoplus.c: elements of Lua 5.2's libraries backported to lua 5.1.4, and vice-versa
 */

#define LUA_FIVETWO_PLUS
#include "fivetwo.c"

extern int luaopen_fivetwoplus (lua_State *L) {
  return luaopen_fivetwo(L);
}
