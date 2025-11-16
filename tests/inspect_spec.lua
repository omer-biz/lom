local P = require("parser")



describe("parser", function()
    it("should inspect parsers", function()
        local attribute_pair = P.identifier()
            :pair(P.literal("=")
                :drop_for(P.quoted_string()))

        local attributes = P.space0()
            :drop_for(attribute_pair)
            :zero_or_more()


        local out, rest = attributes:parse('one="two"  three="four" five="six"')

        assert.are.same(out, { { "one", "two" }, { "three", "four" }, { "five", "six" } })
        assert.are.same(rest, "")

        return true;
    end)
end)
