#!/bin/sh

export LUA_INIT=

lua-5.1 -e 'print "==== native 5.1 ===="; require "base"; require "base-seeall"; require "assign"; require "assign-seeall"; require "get"; require "get-seeall"'

lua-5.2 -lfiveq -e 'print "==== -lfiveq 5.2 ===="; require "base"; require "base-seeall"; require "assign"; require "assign-seeall"; require "get"; require "get-seeall"'

lua-5.1 -lfiveqplus -e 'print "==== -lfiveqplus 5.1 ===="; require "base"; require "base-seeall"; require "base-strict"; require "base-seeall-strict"; require "base-strict-seeall"; require "assign"; require "assign-seeall"; require "assign-strict"'

lua-5.1 -lfiveqplus -e 'require "assign-seeall-strict"'

lua-5.1 -lfiveqplus -e 'require "assign-strict-seeall"'

lua-5.1 -lfiveqplus -e 'require "get"; require "get-seeall"; require "get-strict"'

lua-5.1 -lfiveqplus -e 'require "get-seeall-strict"'

lua-5.1 -lfiveqplus -e 'require "get-strict-seeall"'

lua-5.2 -lfiveqplus -e 'print "==== -lfiveqplus 5.2 ===="; require "base"; require "base-seeall"; require "base-strict"; require "base-seeall-strict"; require "base-strict-seeall"; require "assign"; require "assign-seeall"; require "assign-strict"'

lua-5.2 -lfiveqplus -e 'require "assign-seeall-strict"'

lua-5.2 -lfiveqplus -e 'require "assign-strict-seeall"'

lua-5.2 -lfiveqplus -e 'require "get"; require "get-seeall"; require "get-strict"'

lua-5.2 -lfiveqplus -e 'require "get-seeall-strict"'

lua-5.2 -lfiveqplus -e 'require "get-strict-seeall"'
