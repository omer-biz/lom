local P = require('parser')

local p = P.literal('abc')

local out, rest = p:parse('abcdef')
local result1 = out == 'abc' and rest == 'def'

local out, rest = p:parse('defabc')
local result2 = out == nil and rest == 'defabc'

return result1 and result2;
