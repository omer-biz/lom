local P = require("parser")

local out, rest = P.consume_until("end"):parse("The beginning of the end is neigh")

return out == "The beginning of the end" and rest == " is neigh"
