local P = require "parser"

local out, rest = P.pure("hello"):pair(P.literal("Hi")):parse("Hi, World!")

return rest == ", World!" and out ~= nil and out[1] == "hello" and out[2] == "Hi"
