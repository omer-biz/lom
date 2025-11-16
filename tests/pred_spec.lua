local P = require("parser")

describe("parser", function()
  it("should parse an identifier", function()
    local parser = P.any_char():pred(
      function(char)
        return char == "1"
      end)

    local out, rest = parser:parse("123")
    assert.are.equal(out, "1")
    assert.are.equal(rest, "23")

    local out, rest = parser:parse("321")
    assert.is.falsy(out)
    assert.are.equal(rest, "321")

  end)
end
)
