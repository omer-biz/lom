local P = require('parser')


describe("parser", function()
  it("should parse a give literal string", function()
    local p = P.literal('abc')

    local out, rest = p:parse('abcdef')
    assert.are.equal(out, 'abc')
    assert.are.equal(rest, 'def')

    local out, rest = p:parse('defabc')
    assert.is.falsy(out)
    assert.are.equal(rest, "defabc")

  end)
end)
