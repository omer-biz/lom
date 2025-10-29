local M = ...

local function is_whitespace(str)
  return str:match("^%s*$") ~= nil
end

function M.whitespace_char()
  return M.any_char():pred(is_whitespace)
end

return M
