local P = require("parser")


local attribute_pair = P.identifier()
  :pair(P.literal("=")
        :drop_for(P.quoted_string()))

local attributes = P.space1()
  :drop_for(attribute_pair)
  :zero_or_more()
  :map(function(chars) return table.concat(chars, "") end)

print(P.inspect(attributes, 0))

return true;
