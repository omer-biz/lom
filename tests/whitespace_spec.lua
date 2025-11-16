local P = require("parser")

local parser = P.whitespace_char():zero_or_more()


describe("parser", function()
  it("should parse whitespace", function()
    local out, rest = parser:parse("    123")
    local result1 = #out == 4 and rest == "123"
    assert.are.equal(#out, 4)
    assert.are.equal(rest, "123")
  end)

  it("should parse a quoted string", function()
    -- since we have a whitespace parser now we can do
    local quoted_string = P.space0()
        :drop_for(P.literal('"')
          :drop_for(
            P.any_char()
            :pred(function(char)
              return char ~= '"'
            end
            ):zero_or_more():take_after(P.literal('"'))
          ):map(function(chars) return table.concat(chars, "") end))

    local out, rest = quoted_string:parse('   "hello" another think here')

    assert.are.equal(out, "hello")
    assert.are.equal(rest, " another think here")
  end)
end)
