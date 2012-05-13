#!/bin/sh

printf -- '--- should succeed ---\n'
LUA_INIT= lua-5.2 -lfiveqplus -e 'require "withenv"'

printf -- '--- should fail ---\n'
LUA_INIT= lua-5.2 -lfiveqplus -e 'require "withoutenv"'
