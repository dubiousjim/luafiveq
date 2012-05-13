-- require "module"

print "-------- base-strict-seeall (should succeed) ----------"

local _ENV=_G
module(...,package.seeall,package.strict)


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

