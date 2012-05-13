-- require "module"

print "-------- base-strict (should succeed) ----------"

local print,next = print,next
local _ENV=_G
module(..., package.strict)


gamma = 30
delta = nil
epsilon = {}

function beta()
    print("gamma=30",gamma==30)
    print("delta=nil",delta==nil)
    print("epsilon={}",next(epsilon)==nil)
    print("setting delta")
    delta=50
    print("setting epsilon")
    epsilon=60
end

function alpha()
    beta()
end

function zeta()
    alpha()
end

zeta()

