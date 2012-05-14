#!/bin/sh

printf -- '--- 1. should print false, true ---\n'
LUA_INIT= lua-5.1 critique1.lua
printf -- '--- 1. should print false, true ---\n'
LUA_INIT= lua-5.2 -lfiveq critique1.lua
printf -- '--- 1. should print true, false ---\n'
LUA_INIT= lua-5.1 -lfiveqplus critique1.lua
printf -- '--- 1. should print true, false ---\n'
LUA_INIT= lua-5.2 -lfiveqplus critique1.lua

printf -- '--- 2. should fail ---\n'
LUA_INIT= lua-5.1 critique2.lua
printf -- '--- 2. should fail ---\n'
LUA_INIT= lua-5.2 -lfiveq critique2.lua
printf -- '--- 2. should print true ---\n'
LUA_INIT= lua-5.1 -lfiveqplus critique2.lua
printf -- '--- 2. should print true ---\n'
LUA_INIT= lua-5.2 -lfiveqplus critique2.lua

printf -- '--- 4. should fail ---\n'
LUA_INIT= lua-5.1 critique4.lua
printf -- '--- 4. should fail ---\n'
LUA_INIT= lua-5.2 -lfiveq critique4.lua
printf -- '--- 4. should print true ---\n'
LUA_INIT= lua-5.1 -lfiveqplus critique4.lua
printf -- '--- 4. should print true ---\n'
LUA_INIT= lua-5.2 -lfiveqplus critique4.lua

printf -- '--- 5. should print true,true ---\n'
LUA_INIT= lua-5.1 critique5.lua
printf -- '--- 5. should print true,true ---\n'
LUA_INIT= lua-5.2 -lfiveq critique5.lua
printf -- '--- 5. should print true,true ---\n'
LUA_INIT= lua-5.1 -lfiveqplus critique5.lua
printf -- '--- 5. should print true,true ---\n'
LUA_INIT= lua-5.2 -lfiveqplus critique5.lua

