local P = require('parser')

local p = P.identifier('my_num')
local out, rest = p:parse('my_num = 10')

local result1 = out == 'my_num' and rest == ' = 10'

local p = P.identifier('my_num')
local out, rest = p:parse('-my_num = 10')

local result2 = out == nil and rest == '-my_num = 10'

return result1 and result2
