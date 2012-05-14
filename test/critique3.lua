(function(_ENV)
  module("test")
  function check() return true end
end)(_ENV)

; (function(_ENV)
  module("test.check") -- fails: name conflict for module 'test.check'
  function hello() return 1+2 end
end)(_ENV);

print(test)
