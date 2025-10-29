local P = require("parser")

local parser = P.whitespace_char():zero_or_more()

local out, rest = parser:parse("    123")
local result1 = #out == 4 and rest == "123"
print("res1", result1)

-- since we have a whitespace parser now we can do
local quoted_string = P.space0()
  :right(P.literal('"')
  :right(
    P.any_char()
    :pred(function(char)
            return char ~= '"'
          end
         ):zero_or_more():left(P.literal('"'))
        ):map(function(chars) return table.concat(chars, "") end))

local out, rest = quoted_string:parse('   "hello" another think here')

local result2 = out == "hello" and rest == " another think here"

return result2 and result1
