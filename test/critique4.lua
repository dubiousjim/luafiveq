(function()
  module("mymodule", package.seeall);

  (function()
    module("test.more")
    function hello() return 1+2 end
  end)(_ENV)

  function greet()
    return test.more.hello()  -- fails -- attempt to index global 'test' (a function value)
  end
end)(_ENV);

function test()
  return mymodule.greet()
end

print(test() == 3)
