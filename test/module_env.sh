#!/bin/sh

printf -- '--- should succeed ---\n'
LUA_INIT= lua-5.2 -lfiveqplus -e 'require "localenv"'

printf -- '--- should succeed ---\n'
LUA_INIT= lua-5.2 -lfiveqplus -e 'require "upvalueenv1"'

printf -- '--- should succeed ---\n'
LUA_INIT= lua-5.2 -lfiveqplus -e 'require "upvalueenv2"'

printf -- '--- should succeed ---\n'
LUA_INIT= lua-5.2 -lfiveqplus -e 'require "withoutenv"'
