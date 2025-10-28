local P = require("parser")

local lit1 = P.literal("123")
local lit2 = P.literal("456")
local lit3 = P.literal("789")
local input = "123456789x0"

local combined = lit1:pair(lit2):pair(lit3)
local out, rest = combined:parse(input)

-- {
--   1 = {
--     1 = 123
--     2 = 456
--   }
--   2 = 789
-- }
local result1 = out[1][1] == "123" and out[1][2] == "456" and out[2] == "789" and rest == "x0"

local combined = lit1:pair(lit2:pair(lit3))
local out, rest = combined:parse(input)

-- {
--   1 = 123
--   2 = {
--     1 = 456
--     2 = 789
--   }
-- }
local result2 = out[1] == "123" and out[2][1] == "456" and out[2][2] == "789" and rest == "x0"

return result1 and result2
