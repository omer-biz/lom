---@meta parser

local M = {}

--- Parses any single character.
---
--- **Implemented in:** C
--- @example
--- local p = parser.any_char()
--- print(p:parse("abc"))  -- → "a", "bc"
---@return Parser
function M.any_char() end

--- Lazily evaluates a parser.
--- Useful for defining mutually recursive parsers.
---
--- **Implemented in:** C
--- @example
--- local p
--- p = parser.lazy(function() return parser.literal("a") or p end)
---@param parser_f fun(): Parser A function returning a parser.
---@return Parser
function M.lazy(parser_f) end

--- Parses a given literal string.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("hello")
--- print(p:parse("hello world"))  -- → "hello", " world"
---@param s string The literal value to match.
---@return Parser
function M.literal(s) end

--- Returns a string representation of the parser’s parse tree.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("hi")
--- print(parser.inspect(p))  -- → "literal('hi')"
---@param parser Parser
---@return string
function M.inspect(parser) end

--- Sets a custom `inspect` description for a parser.
--- This description is used when calling `inspect()`.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.literal("hi")
--- parser.set_inspect(p, "greeting parser")
--- print(parser.inspect(p))  -- → "greeting parser"
---@param parser Parser
---@param inspect_str string A textual description of the parser.
---@return Parser
function M.set_inspect(parser, inspect_str) end

--- Consumes a single whitespace character.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.whitespace_char()
--- print(p:parse(" abc"))  -- → " ", "abc"
---@return Parser
function M.whitespace_char() end

--- Consumes one or more whitespace characters.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.space1()
--- print(p:parse("   xyz"))  -- → "   ", "xyz"
---@return Parser
function M.space1() end

--- Consumes zero or more whitespace characters.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.space0()
--- print(p:parse("  hi"))   -- → "  ", "hi"
---@return Parser
function M.space0() end

--- Parses a quoted string (e.g. `"hello"`) and returns the unquoted value (e.g. `hello`).
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.quoted_string()
--- print(p:parse('"hi" there'))  -- → "hi", " there"
---@return Parser
function M.quoted_string() end

--- Parses an identifier starting with an underscore or a letter,
--- followed by any combination of letters, digits, underscores, or dashes.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.identifier()
--- print(p:parse("name_123 rest"))  -- → "name_123", " rest"
---@return Parser
function M.identifier() end

---The identity function, lifts normal strings to Parser world.
---
---**Implemented in:** Lua
---**Example:**
---```lua
--- local p_hello = parser.pure("hello")
--- print(p_hello:parse("Hello, World!")) -- → "hello", "Hello, World!"
---```
---@param id string
---@return Parser
function M.pure(id) end

---@class Parser
---@field inspect string A textual description of the parser.
M.Parser = {}

--- Runs the parser one or more times, as long as it continues to succeed.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.literal("a"):one_or_more()
--- print(p:parse("aaab"))  -- → "aaa", "b"
---@param self Parser
---@return Parser
function M.Parser:one_or_more() end

--- Runs the parser zero or more times, as long as it continues to succeed.
---
--- **Implemented in:** Lua
--- @example
--- local p = parser.literal("x"):zero_or_more()
--- print(p:parse("xxxy"))  -- → "xxx", "y"
---@param self Parser
---@return Parser
function M.Parser:zero_or_more() end

--- Parses the input using `self`.
--- If successful, discards the result, then parses the remaining input with `taken`,
--- returning the result of `taken`.
---
--- **Implemented in:** C
--- @example
--- local a = parser.literal("a")
--- local b = parser.literal("b")
--- local p = a:drop_for(b)
--- print(p:parse("ab"))  -- → "b", ""
---@param self Parser
---@param taken Parser
---@return Parser
function M.Parser:drop_for(taken) end

--- Parses the input using `self`.
--- If successful, continues parsing with `ignored`, discards that result,
--- and returns the original result from `self`.
---
--- **Implemented in:** C
--- @example
--- local word = parser.literal("word")
--- local space = parser.space1()
--- local p = word:take_after(space)
--- print(p:parse("word next"))  -- → "word", "next"
---@param self Parser
---@param ignored Parser
---@return Parser
function M.Parser:take_after(ignored) end

--- Transforms the parsed value using the given function.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("1"):map(tonumber)
--- print(p:parse("1+"))  -- → 1, "+"
---@generic T
---@generic U
---@param self Parser
---@param f fun(inner: T): U A transformation function.
---@return Parser
function M.Parser:map(f) end

--- Chains another parser based on the result of this one.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("a"):and_then(function(_) return parser.literal("b") end)
--- print(p:parse("ab"))  -- → "b", ""
---@generic T
---@param self Parser
---@param f fun(inner: T): Parser A function returning the next parser.
---@return Parser
function M.Parser:and_then(f) end

--- Attempts to parse using `self`.
--- If it fails, tries the alternative parser `alt`.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("yes"):or_else(parser.literal("no"))
--- print(p:parse("nope"))  -- → nil, "nope"
---@param self Parser
---@param alt Parser
---@return Parser
function M.Parser:or_else(alt) end

--- Applies a predicate function to the parsed value.
--- The result is accepted only if the predicate returns `true`.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("a"):pred(function(v) return v == "a" end)
--- print(p:parse("a"))  -- → "a", ""
---@generic T
---@param self Parser
---@param f fun(value: T): boolean Predicate function.
---@return Parser
function M.Parser:pred(f) end

--- Combines this parser with another parser, returning a pair of their results.
---
--- **Implemented in:** C
--- @example
--- local p1 = parser.literal("a")
--- local p2 = parser.literal("b")
--- local pair = p1:pair(p2)
--- print(pair:parse("ab"))  -- → {"a", "b"}, ""
---@param self Parser
---@param p Parser
---@return Parser
function M.Parser:pair(p) end

--- Executes the parser on the given input string.
---
--- **Implemented in:** C
--- @example
--- local p = parser.literal("hi")
--- print(p:parse("hi there"))  -- → "hi", " there"
---@param self Parser
---@param input string
---@return [table | string | nil, string] The parsed result (or `nil`) and the remaining input.
function M.Parser:parse(input) end

-- Utility functions
M.utils = {}

--- Returns a formatted string representation of a table’s fields.
---
--- **Implemented in:** Lua
--- @example
--- print(parser.utils.print({a = 1, b = {2, 3}}, 0))
---@param t table The table to inspect.
---@param indent integer Indentation level.
---@return string?
function M.utils.print(t, indent) end

--- Compares two tables for field equality.
---
--- **Implemented in:** Lua
--- @example
--- print(parser.utils.tables_equal({a = 1}, {a = 1}))  -- → true
---@param t1 table
---@param t2 table
---@param visited table? Internal table used to avoid infinite recursion.
---@return boolean
function M.utils.tables_equal(t1, t2, visited) end

return M
