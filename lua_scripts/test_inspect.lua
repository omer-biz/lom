local P = require("parser")


local attribute_pair = P.identifier()
  :pair(P.literal("=")
        :drop_for(P.quoted_string()))

local attributes = P.space1()
  :drop_for(attribute_pair)
  :zero_or_more()

print(P.inspect(attributes, 0))

return true;
