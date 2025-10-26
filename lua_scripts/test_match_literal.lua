local P = require('parserlib')

local p = P.match_literal('abc')
local rest, out = p:parse('abcdef')

return rest == 'def' and out == nil;
