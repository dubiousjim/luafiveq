function test() return 1+2 end

; (function()
  module("mymodule", package.seeall);

  (function()
    module("test.more") -- fails: name conflict for module 'test.more'
    function hello() return 1+2 end
  end)(_ENV)
end)(_ENV)

print(mymodule.test.more.hello() == 3)
