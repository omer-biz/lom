local P = require("parser")


describe("parser", function()
  it("should parse an identifier", function()
    local p = P.identifier("my_num")
    local out, rest = p:parse("my_num = 10")

    assert.are.equal(out, "my_num")
    assert.are.equal(rest, " = 10")
  end)

  it("should ignore non identifiers", function()
    local p = P.identifier("my_num")
    local out, rest = p:parse("-my_num = 10")

    assert.is.falsy(out)
    assert.are.equal(rest,  "-my_num = 10")
  end)
end)
