local P = require("parser")

local parser = P.literal("ha"):one_or_more()
local out, rest = parser:parse("hahahaah")
local result1 = out[1] == "ha" and out[2] == "ha" and  out[3] == "ha" and  #out == 3 and rest == "ah"

local out, rest = parser:parse("ahah")
local result2 = out == nil and rest == "ahah"

return result1 and result2
