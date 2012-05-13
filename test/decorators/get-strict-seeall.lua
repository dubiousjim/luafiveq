-- require "module"

print "-------- get-strict-seeall (should fail at epsilon) ----------"

local _ENV=_G
module(...,package.seeall,package.strict)


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

