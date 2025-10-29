-- adapted from https://bodil.lol/parser-combinators/
local P = require("parser")

local attribute_pair = P.identifier()
  :pair(P.literal("=")
        :right(P.quoted_string()))

local attributes = P.space1()
  :right(attribute_pair)
  :zero_or_more()

local out, rest = attributes:parse(' one="1"    two="2"')

if out[1][1] ~= "one" or out[1][2] ~= "1" or out[2][1] ~= "two" or out[2][2] ~= "2" then
  return false
end

---------------------

local element_start = P.literal("<"):right(P.identifier():pair(attributes))

local function restructure(b)
  a = {}
  for _, value in pairs(b) do
      a[value[1]] = value[2]
  end
  return a
end

local function extract_header(out)
  local name, attrs = out[1], out[2]
  return {name = name, attributes = restructure(attrs),}
end

local single_element = element_start
  :left(P.literal("/>"))
  :map(extract_header)

local input = '<div class="float" id="num" foo="bar"/>'
local out, rest = single_element:parse(input)

local attrs = out["attributes"]
if out["name"] ~= "div" or attrs["class"] ~= "float" or attrs["foo"] ~= "bar" or attrs["id"] ~= "num" then
  return false
end

---------------------

local open_element = element_start:left(P.literal(">")):map(extract_header)
local out, rest = open_element:parse('<one two="three">rest')

local attrs = out["attributes"]
if out["name"] ~= "one" or attrs["two"] ~= "three" then
  return false
end

---------------------

local function whitespace_wrap(parser)
  return P.space0()
    :right(parser:left(P.space0()))
end


local function close_element(expected_name)
  return P.literal("</")
    :right(P.identifier():left(P.literal(">")))
    :pred(function(name) return name == expected_name end)
end

local element -- forward declaration: to asist with lazy evaluation
element = P.lazy(function()
  local parent_element = open_element:and_then(function(el)
    return element:zero_or_more()
      :left(close_element(el.name))
      :map(function(children)
        el.children = children
        return el
      end)
  end)

  return whitespace_wrap(single_element:or_else(parent_element))
end)

local function whitespace_wrap(parser)
  return P.space0()
    :right(parser:left(P.space0()))
end

----------------------------

local input = [[
<top label="Top">
    <semi-bottom label="Bottom"/>
    <middle>
        <bottom label="Another bottom"/>
    </middle>
</top> hello
]]


local out, rest = element:parse(input)

local xml_ast = {
    name = "top",
    attributes = {label = "Top"},
    children = {
        {name = "semi-bottom", attributes = {label = "Bottom"}},
        {name = "middle", attributes = {}, children = {
            {name = "bottom", attributes = {label = "Another bottom"}},
        }},
    },
}

return P.utils.tables_equal(xml_ast, out) and rest == "hello\n"
