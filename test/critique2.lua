function test() return 1+2 end

; (function(_ENV)
  module("mymodule", package.seeall);

  (function(_ENV)
    module("test.more") -- fails: name conflict for module 'test.more'
    function hello() return 1+2 end
  end)(_ENV)
end)(_ENV)

print(mymodule.test.more.hello() == 3)
