#!/usr/local/bin/lua-5.2 -lfiveqplus

-- test/module.lua
--
-- Demonstrates some fine details of how fiveqplus's module() works.
--
-- We use module(name1) and require(name2), where these names may be different:
-- normally you wouldn't do that, but it enables us to be more specific.

-- administrative setup
local baseenv, reqenv, mod, req, modreturn
local name1, name2, modreturn, seeall, reqenv
local package, require, module = package, require, module
local assert, print = assert, print
baseenv = { _G = getfenv(0) } -- explicitly use a base _ENV other than _G
for k,v in pairs(_ENV) do baseenv[k] = v end
_ENV = baseenv


-- we vary these to verify all the assertions below remain satisfied
if bit32.btest(..., 1) then
    name1, name2 = "name1", "name2"
else
    name1, name2 = "name", "name"
end
if bit32.btest(..., 2) then
    modreturn = {}
else
    modreturn = nil
end
seeall = bit32.btest(..., 4)
if bit32.btest(..., 8) then
    reqenv = _ENV
else
    reqenv = nil
end


package.preload[name2] = function()
    -- we begin with the same environment as surrounding chunk
    assert(_ENV == baseenv)

    if seeall then
        mod = module(name1, package.seeall)
    else
        mod = module(name1)
    end

    -- but now we have a new environment
    assert(_ENV ~= baseenv)
    assert(mod == _M)
    if seeall then
        assert(mod ~= _ENV)
    else
        assert(mod == _ENV)
    end
    baseenv.modenv = _ENV

    -- module was exported to requiring environment (if it exists) but not to _G
    assert(reqenv == nil or mod == reqenv[name1])
    assert(baseenv._G[name1] == nil)
    return modreturn
end


do
    local _ENV = reqenv -- if this is nil, reqenv[name1] won't be assigned
    req = require(name2)
end

-- main chunk keeps it original environment
assert(_ENV == baseenv and _ENV ~= modenv)

assert(req == package.loaded[name2])

if modreturn ~= nil then
    assert(req == modreturn)
elseif name1 == name2 then
    assert(req == mod)
else
    assert(req == true)
end

if name1 ~= name2 then
    assert(mod == package.loaded[name1])
end

print "OK"
