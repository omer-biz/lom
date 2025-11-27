local core = require("parser.core")

local M = {}
for k, v in pairs(core) do
    M[k] = v
end


---@param str string checkes wheatear str is a whitespace chararcter
---@return boolean
local function is_whitespace(str)
    return str:match("^%s*$") ~= nil
end

function M.set_inspect(parser, inspect_str)
    parser.inspect = inspect_str
    return parser
end

function M.whitespace_char()
    return M.set_inspect(core.any_char():pred(is_whitespace), "whitespace_char")
end

function M.space1()
    return M.set_inspect(M.whitespace_char():one_or_more(), "space1")
end

function M.space0()
    return M.set_inspect(M.whitespace_char():zero_or_more(), "space0")
end

function M.quoted_string()
    return M.set_inspect(
        M.space0():drop_for(
            core.literal('"'):drop_for(
                core.any_char():pred(
                    function(char)
                        return char ~= '"'
                    end
                ):zero_or_more():take_after(M.literal('"'))
            ):map(
                function(chars)
                    return table.concat(chars, "")
                end
            )
        ),
        "quoted_string"
    )
end

function M.identifier()
    return M.set_inspect(
        core.any_char():pred(
            function(ch)
                return ch:match("[%a_]") ~= nil
            end
        ):and_then(
            function(first)
                return core.any_char():pred(
                    function(ch)
                        return ch:match("[%w_%-]") ~= nil
                    end
                ):zero_or_more():map(
                    function(rest)
                        table.insert(rest, 1, first)
                        return table.concat(rest, "")
                    end
                )
            end
        ),
        "identifier"
    )
end

function M.pure(id)
    local p = core.new(function(input)
        return id, input
    end)

    return M.set_inspect(p, string.format("pure(%q)", id))
end

function M.consume_until(mark)
    local p = core.new(function(input)
        local start_pos, end_pos = input:find(mark, 1, true)
        if not start_pos then
            return nil, input
        end

        local out = input:sub(1, end_pos)
        local rest = input:sub(end_pos + 1, input:len())

        return out, rest
    end)

    return M.set_inspect(p, string.format("consume_until(%q)", mark))
end

M.utils = {}

function M.utils.print(t, indent)
    indent = indent or 0
    local spacing = string.rep(" ", indent)
    if type(t) ~= "table" then
        print(spacing .. tostring(t))
        return
    end
    print("{")
    for k, v in pairs(t) do
        io.write(spacing .. " " .. tostring(k) .. " = ")
        if type(v) == "table" then
            M.utils.print(v, indent + 1)
        else
            print(tostring(v) .. ",")
        end
    end
    print(spacing .. "},")
end

function M.utils.tables_equal(t1, t2, visited)
    visited = visited or {}
    if visited[t1] and visited[t1][t2] then
        return true
    end

    if t1 == t2 then
        return true
    end
    if type(t1) ~= "table" or type(t2) ~= "table" then
        return false
    end

    visited[t1] = visited[t1] or {}
    visited[t1][t2] = true

    for k, v in pairs(t1) do
        local v2 = t2[k]
        if type(v) == "table" and type(v2) == "table" then
            if not M.utils.tables_equal(v, v2, visited) then
                return false
            end
        elseif v2 ~= v then
            return false
        end
    end

    -- check for extra keys in t2
    for k, _ in pairs(t2) do
        if t1[k] == nil then
            return false
        end
    end
    return true
end

return M
