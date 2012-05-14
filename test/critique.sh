#!/bin/sh

printf -- '--- should print false, true ---\n'
LUA_INIT= lua-5.1 critique.lua
printf -- '--- should print false, true ---\n'
LUA_INIT= lua-5.2 -lfiveq critique.lua
printf -- '--- should print true, false ---\n'
LUA_INIT= lua-5.1 -lfiveqplus critique.lua
printf -- '--- should print true, false ---\n'
LUA_INIT= lua-5.2 -lfiveqplus critique.lua

