local P = require "parser"

describe("parser", function()
  it("should parse an identifier", function()
    local out, rest = P.pure("hello"):pair(P.literal("Hi")):parse("Hi, World!")

    assert.are.same(out, { "hello", "Hi" })
    assert.are.equal(rest, ", World!")
  end)
end)
