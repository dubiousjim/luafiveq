local _P = package.loaded
-- local _P0 = {}
-- for k,v in pairs(_P) do
--     _P0[k] = v
-- end

; (function(_ENV)
  local require = require
  local module = module
  local print = print
  local _P = package.loaded
  module('yourmodule.two');

  (function(_ENV)
    module('mymodule.one')
    return _M
  end)(_ENV)

  print(_P['mymodule.one'] ~= nil) -- prints true
  return _M
end)(_ENV);

print(_P['mymodule.one'] ~= nil) -- prints true

-- for k,v in pairs(_P) do
--     if v ~= _P0[k] then
--         print(k)
--     end
-- end
