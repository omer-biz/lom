local parser = require("parser")

local M = {}

----------------------------------------------------------------------
-- Helpers
----------------------------------------------------------------------

local function token(p)
  return parser.space0():drop_for(p)
end

local function kwd(name, value)
  return token(parser.literal(name)):map(function() return value end)
end

----------------------------------------------------------------------
-- Forward declarations
----------------------------------------------------------------------

local value
local array
local object
local json_string
local null
local boolean
local number

value          = parser.lazy(function()
  return
      object
      :or_else(array)
      :or_else(json_string)
      :or_else(number)
      :or_else(boolean)
      :or_else(null)
end)

----------------------------------------------------------------------
-- null / true / false
----------------------------------------------------------------------

null           = kwd("null", nil)
boolean        = kwd("true", true):or_else(kwd("false", false))

----------------------------------------------------------------------
-- string
----------------------------------------------------------------------

json_string    = token(parser.quoted_string())

----------------------------------------------------------------------
-- number
----------------------------------------------------------------------

local digit    = parser.any_char():pred(function(c)
  return c:match("%d") ~= nil
end)

local digits   = digit:one_or_more():map(table.concat)

local sign     =
    parser.literal("-")
    :or_else(parser.literal("+"))
    :or_else(parser.pure(""))
    :map(function(s) return s end)

local fraction =
    parser.literal(".")
    :and_then(function() return digits end)
    :map(function(ds) return "." .. ds end)
    :or_else(parser.pure(""))

local exponent =
    (parser.literal("e"):or_else(parser.literal("E")))
    :and_then(function()
      return sign:pair(digits)
    end)
    :map(function(pair)
      return "e" .. pair[1] .. pair[2]
    end)
    :or_else(parser.pure(""))

number         =
    token(
      sign:pair(digits):pair(fraction):pair(exponent)
    ):map(function(tree)
      local sg = tree[1][1][1]
      local int = tree[1][1][2]
      local frac = tree[1][2]
      local exp = tree[2]
      local s = (sg or "") .. int .. (frac or "") .. (exp or "")
      return tonumber(s)
    end)

----------------------------------------------------------------------
-- array
----------------------------------------------------------------------

local comma    = token(parser.literal(","))

array          =
    token(parser.literal("["))
    :and_then(function()
      -- check for empty array
      return token(parser.literal("]")):map(function() return {} end)
          :or_else(
            value:pair((comma:drop_for(value)):zero_or_more())
            :take_after(token(parser.literal("]")))
          )
    end)
    :map(function(pair)
      if pair[1] == nil then
        return {}
      end
      local first = pair[1]
      local rest = pair[2]
      local arr = { first }
      for _, v in ipairs(rest) do
        table.insert(arr, v)
      end
      return arr
    end)

----------------------------------------------------------------------
-- object
----------------------------------------------------------------------

local colon    = token(parser.literal(":"))

object         =
    token(parser.literal("{"))
    :and_then(function()
      return token(parser.literal("}")):map(function() return {} end)
          :or_else(
            json_string:pair(colon:drop_for(value))
            :pair(
              (comma:drop_for(json_string:pair(colon:drop_for(value))))
              :zero_or_more()
            )
            :take_after(token(parser.literal("}")))
          )
    end)
    :map(function(tree)
      if tree[1] == nil then
        return {}
      end
      local first = tree[1]
      local rest = tree[2]
      local obj = {}
      obj[first[1]] = first[2]
      for _, pair in ipairs(rest) do
        obj[pair[1]] = pair[2]
      end
      return obj
    end)

----------------------------------------------------------------------
-- Public API
----------------------------------------------------------------------

function M.parse_json(str)
  local result, rest = value:parse(str)
  if not result then
    return nil, "Invalid JSON"
  end

  if rest:match("^%s*$") then
    return result
  end

  return nil, "Trailing characters after JSON"
end

return M
