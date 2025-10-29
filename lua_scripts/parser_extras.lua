local M = ...

local function is_whitespace(str)
  return str:match("^%s*$") ~= nil
end

function M.whitespace_char()
  return M.any_char():pred(is_whitespace)
end

function M.space1()
  return M.whitespace_char():one_or_more()
end

function M.space0()
  return M.whitespace_char():zero_or_more()
end

function M.quoted_string()
  return M.space0()
    :right(M.literal('"')
      :right(
        M.any_char()
        :pred(function(char) return char ~= '"'
        end):zero_or_more():left(M.literal('"'))
      ):map(function(chars) return table.concat(chars, "") end)
    )
end

return M
