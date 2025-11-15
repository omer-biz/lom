local P = require("parser")

local parser = P.any_char():pred(
    function(char)
        return char == "1"
    end)

local out, rest = parser:parse("123")
local result1 = out == "1" and rest == "23"

local out, rest = parser:parse("321")
local result2 = out == nil and rest == "321"

return result2 and result1
