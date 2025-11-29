local P = require("parser")

local lit1 = P.literal("123")
local lit2 = P.literal("456")
local lit3 = P.literal("789")
local input = "123456789x0"

describe("parser", function()
  it("should pair two parsers left associated", function()
    local combined = lit1:pair(lit2):pair(lit3)
    local out, rest = combined:parse(input)

    assert.are.same(out, {
      {
        '123',
        '456',
      },
      '789'
    })
    assert.are.equal(rest, "x0")
  end)

  it("should pair two parsers right associated", function()
    local combined = lit1:pair(lit2:pair(lit3))
    local out, rest = combined:parse(input)

    assert.are.same(out, {
      '123',
      {
        '456',
        '789'
      }
    })
    assert.are.equal(rest, "x0")
  end)
end)
