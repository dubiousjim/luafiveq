(function()
  module("test")
  function check() return true end
end)(_ENV)

; (function()
  module("test.check") -- fails: name conflict for module 'test.check'
  function hello() return 1+2 end
end)(_ENV);

print(test)
