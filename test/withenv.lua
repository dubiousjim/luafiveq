local req,print,_G=req,print,_G
local _ENV = _G
local res=module(...)
print("inside",_NAME,res==_M)
return "foo"
