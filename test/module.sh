#!/bin/sh

u51() { LUA_INIT= lua-5.1 "$@" 2>&1; }
u52() { LUA_INIT= lua-5.2 "$@" 2>&1; }

deploy () { 
    printf -- '--- native 5.1 ---\n';
    u51 -e "local ver,require,module,print,type,assert,error,setfenv,_G=51,require,module,print,type,assert,error,setfenv,_G; $*";
    printf -- '\n\n--- -lfive 5.2 ---\n';
    u52 -lfiveq -e "local ver,require,module,print,type,assert,error,_G=52,require,module,print,type,assert,error,_G; $*";
    printf -- '\n\n--- five 5.2 ---\n';
    u52 -e 'require "fiveq"; local ver,require,module,print,type,assert,error,_G=52,require,module,print,type,assert,error,_G; '"$*";
    printf -- '\n\n--- -lplus 5.1 ---\n';
    u51 -lfiveqplus -e "local ver,require,module,print,type,assert,error,setfenv,_G=51,require,module,print,type,assert,error,setfenv,_G; $*";
    printf -- '\n\n--- plus 5.1 ---\n';
    u51 -e 'require "fiveqplus"; local ver,require,module,print,type,assert,error,setfenv,_G=51,require,module,print,type,assert,error,setfenv,_G; '"$*";
    printf -- '\n\n--- plus 5.1 with native require ---\n';
    u51 -e 'local require=require; require "fiveqplus"; local ver,module,print,type,assert,error,setfenv,_G=51,module,print,type,assert,error,setfenv,_G; '"$*";
    printf -- '\n\n--- -lplus 5.2 ---\n';
    u52 -lfiveqplus -e "local ver,require,module,print,type,assert,error,_G=52,require,module,print,type,assert,error,_G; $*";
    printf -- '\n\n--- plus 5.2 ---\n';
    u52 -e 'require "fiveqplus"; local ver,require,module,print,type,assert,error,_G=52,require,module,print,type,assert,error,_G; '"$*";
    printf -- '\n\n--- 5.2 with native require ---\n';
    u52 -e 'local require=require; require "fiveqplus"; local ver,module,print,type,assert,error,_G=52,module,print,type,assert,error,_G; '"$*"
}

deploy_module () { 
    printf -- '=== as file ===\n\n';
    printf -- '%s' "local ver,module,print,type,assert,error,setfenv,_G=_VERSION,module,print,type,assert,error,setfenv,_G; local _ENV={id=\"foo\"}; if ver ~= \"Lua 5.2\" then setfenv(1, _ENV) end $1" > foo.lua;
    deploy "$2 function bar(_ENV) _ENV=_ENV or {id=\"bar\"}; if ver==51 then setfenv(1,_ENV) end local required=require \"foo\"; $3 end $4";
    printf -- '\n\n\n=== as preload ===\n\n';
    rm -f foo.lua;
    deploy "package.preload.foo=function(...) local _ENV={id=\"foo\"}; if ver==51 then setfenv(1, _ENV) end $1 end; $2 function bar(_ENV) _ENV=_ENV or {id=\"bar\"}; if ver==51 then setfenv(1,_ENV) end local required=require \"foo\"; $3 end $4";
    printf -- '\n\n\n=== as function ===\n\n';
    deploy "local function makefoo(_ENV, ...) if ver==51 then setfenv(1, _ENV) end $1 end; $2 function bar(_ENV) _ENV=_ENV or {id=\"bar\"}; if ver==51 then setfenv(1,_ENV) end local required=makefoo({id=\"foo\"}, \"foo\"); $3 end $4"
}

deploy_module 'assert(id=="foo"); module(...) x="FOO"; assert(id==nil)' '' 'local f=_G.foo or _ENV.foo; assert(id=="bar" and f and f.x=="FOO"); print(_G.foo and "wrote to _G" or _ENV.foo and "wrote to _ENV" or "unknown")' 'bar(); assert(not _G.foo or _G.foo.x=="FOO"); print(_G.foo and "wrote to _G" or "didnt write to _G")'

# Expected results:
# Whether as file, as preload, or as function: native 5.1, -lfive 5.2, five 5.2 write to _G not to _ENV

