local P = require("parser")

local lit1 = P.literal("123")
local lit2 = P.literal("456")

local input = "123456890"

-- take after parser
local take_after_parser = lit1:take_after(lit2)
local out, rest = take_after_parser:parse(input)
local take_after_result = out == "123" and rest == "890"

-- drop for parser
local drop_for_parser = lit1:drop_for(lit2)
local out, rest = drop_for_parser:parse(input)
local drop_for_result = out == "456" and rest == "890"

-- tag test
local input = "<first-element/>"
local tag_opener = P.literal("<"):drop_for(P.identifier())
local out, rest = tag_opener:parse(input)
local tag_result = out == "first-element" and rest == "/>"

return take_after_result and drop_for_result and tag_result
