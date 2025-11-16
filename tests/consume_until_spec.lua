local busted = require("busted")
local P = require("parser")

describe("parser", function()
  it("should consume until a mark", function()
    local out, rest = P.consume_until("end"):parse("The beginning of the end is neigh")

    assert.are.equal(out, "The beginning of the end")
    assert.are.equal(rest, " is neigh")
  end)
end)
