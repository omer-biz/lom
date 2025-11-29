local P = require("parser")

describe("parser", function()
  it("should parse zero or more times", function()
    local parser = P.literal("ha"):zero_or_more()

    local out, rest = parser:parse("hahahaah")
    assert.are.same(out, { "ha", "ha", "ha" })
    assert.are.equal(rest, "ah")

    local out, rest = parser:parse("ahah")
    assert.are.equal(#out, 0)
    assert.are.equal(rest, "ahah")
  end)
end
)
