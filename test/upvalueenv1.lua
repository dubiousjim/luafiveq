local print,_G=print,_G
local z = {}
(function(...)
    local z0 = z
    local res = module(...)
    print("inside", _NAME, res==_M)
    return "foo"
end)(...)
