print('--- Lua parser combinators test ---')
local parser = require('parser')
-- literal parser
local lit = parser.literal('foo')

-- map example: uppercase the result
local upper = lit:map(function(s) return string.upper(s) end)

-- identifier parser
local ident = parser.identifier()

-- zero_or_more example
local many_idents = ident:zero_or_more()

-- and_then example: after matching 'foo', match 'bar'
local combined = lit:and_then(function(_) return parser.literal('bar')
end)

-- one_or_more example
local repeated = parser.literal('a'):one_or_more()

-- or_else example
local choice = parser.literal('x'):or_else(parser.literal('y'))

-- pred example: only keep 'foo' if length is 3
local pred_test = lit:pred(function(s) return #s == 3 end)

-- test inputs
local function run(p, input)
  local out, rest = p:parse(input)
  if out then
    print('Parse succeeded! Output:', out, 'Remaining:', rest)
  else
    print('Parse failed. Remaining input:', rest)
  end
end

print('--- map test ---')
run(upper, 'foo baz')

print('--- zero_or_more test ---')
run(many_idents, 'abc123 xyz')

print('--- and_then test ---')
run(combined, 'foobar')

print('--- one_or_more test ---')
run(repeated, 'aaaaab')

print('--- or_else test ---')
run(choice, 'yabc')

print('--- pred test ---')
run(pred_test, 'foo bar')

collectgarbage()
print('--- done ---');
