-- we need to introduce a new _ENV, else it will be a shared upvalue with the
-- parent, and when module rebinds it, that will affect the main chunk too
(function()
    local require = require
    local print = print
    local module = module
    module("yourmodule")
    -- we need to introduce a new _ENV here too
    ; (function() local x=x; module("mymodule") end)(_ENV)

    print(mymodule ~= nil) -- prints false (where is it?)
end)(_ENV)

print(mymodule ~= nil) -- prints true (where did this come from?)
