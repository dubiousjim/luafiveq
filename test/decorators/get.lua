-- require "module"

print "-------- get (should succeed) ----------"

local print = print
local _ENV=_G
module(...)


gamma = 30
delta = nil
-- epsilon = {}

function beta()
    print("gamma=30",gamma==30)
    print("delta=nil",delta==nil)
    print("epsilon=nil",epsilon==nil)
end

function alpha()
    beta()
end

function zeta()
    alpha()
end

zeta()

