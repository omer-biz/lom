local P = require("parser")

local out, rest = P.pure("hello"):parse("Hello, World!")

return out == "hello" and rest == "Hello, World!"
