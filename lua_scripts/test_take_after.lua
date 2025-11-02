local P = require("parser")

local lit1 = P.literal("123")
local lit2 = P.literal("456")

local input = "123456890"

-- take after parser
local take_after_parser = lit1:take_after(lit2)
local out, rest = take_after_parser:parse(input)
local take_after_result = out == "123" and rest == "890"

-- right parser
local right_parser = lit1:right(lit2)
local out, rest = right_parser:parse(input)
local right_result = out == "456" and rest == "890"

-- tag test
local input = "<first-element/>"
local tag_opener = P.literal("<"):right(P.identifier())
local out, rest = tag_opener:parse(input)
local tag_result = out == "first-element" and rest == "/>"

return take_after_result and right_result and tag_result
