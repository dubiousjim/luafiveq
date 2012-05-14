-- hello/world.lua
module(..., package.seeall)
local function test(n) print(n) end
function test1() test(123) end
function test2() test1(); test1() end
