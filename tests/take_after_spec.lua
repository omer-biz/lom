local P = require("parser")

local lit1 = P.literal("123")
local lit2 = P.literal("456")

local input = "123456890"

describe("parser", function()
  it("should take after parsing", function()
    -- take after parser
    local take_after_parser = lit1:take_after(lit2)
    local out, rest = take_after_parser:parse(input)

    assert.are.equal(out, "123")
    assert.are.equal(rest, "890")
  end)

  it("should drop after parsing", function()
    -- drop for parser
    local drop_for_parser = lit1:drop_for(lit2)
    local out, rest = drop_for_parser:parse(input)

    assert.are.equal(out, "456")
    assert.are.equal(rest, "890")
  end)

  it("should parse a self closing xml tag", function()
    -- tag test
    local input = "<first-element/>"
    local tag_opener = P.literal("<"):drop_for(P.identifier())
    local out, rest = tag_opener:parse(input)

    assert.are.equal(out, "first-element")
    assert.are.equal(rest, "/>")
  end)
end)
