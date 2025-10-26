local parser = require("parser")

-- Basic parsers
local lt  = parser.literal("<")
local gt  = parser.literal(">")
local slash = parser.literal("/")
local eq  = parser.literal("=")
local quote = parser.literal('"')
local dash = parser.literal("-")
local space = parser.literal(" "):zero_or_more()
local name_char = parser.identifier()  -- letters, digits, '-'
local name = name_char:one_or_more():map(function(chars)
    return table.concat(chars, "")
end)

-- Attribute parser: key="value"
local attr = name:and_then(function(key)
    return eq:and_then(function()
        return quote:and_then(function()
            return name:and_then(function(value)
                return quote:map(function(_) return {key, value} end)
            end)
        end)
    end)
end)

local attrs = attr:zero_or_more():map(function(list) return list end)

-- Element parsers
local open_tag = lt:and_then(function()
    return name:and_then(function(tag_name)
        return space:and_then(function()
            return attrs:and_then(function(attributes)
                return gt:map(function() return {tag_name = tag_name, attributes = attributes, children = {}} end)
            end)
        end)
    end)
end)

local close_tag = lt:and_then(function()
    return slash:and_then(function()
        return name:and_then(function(tag_name)
            return gt:map(function() return tag_name end)
        end)
    end)
end)

-- Self-closing tag: <element ... />
local self_closing_tag = lt:and_then(function()
    return name:and_then(function(tag_name)
        return space:and_then(function()
            return attrs:and_then(function(attributes)
                return slash:and_then(function()
                    return gt:map(function()
                        return {tag_name = tag_name, attributes = attributes, children = {}}
                    end)
                end)
            end)
        end)
    end)
end)

-- Recursive element parser
local function element()
    return parser.or_else(self_closing_tag, 
        open_tag:and_then(function(open)
            return element():zero_or_more():and_then(function(children)
                return close_tag:map(function(close_name)
                    if close_name ~= open.tag_name then
                        error("Mismatched closing tag: " .. close_name)
                    end
                    open.children = children
                    return open
                end)
            end)
        end)
    )
end

-- Parse the input
local input = [[
<parent-element>
  <single-element attribute="value" />
</parent-element>
]]

local tree, remaining = element():parse(input)
print(require("inspect")(tree))  -- assuming you have `inspect` for pretty printing

