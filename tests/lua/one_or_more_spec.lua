local P = require("parser")

describe("parser", function()
  it("should parse one or more times", function()
    local parser = P.literal("ha"):one_or_more()

    local out, rest = parser:parse("hahahaah")
    assert.are.same(out, { "ha", "ha", "ha" })
    assert.are.equal(rest, "ah")

    local out, rest = parser:parse("ahah")
    assert.is.falsy(out)
    assert.are.equal(rest, "ahah")
  end)
end)
