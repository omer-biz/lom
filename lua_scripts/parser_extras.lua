local M = ...

M.utils = {}

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

return M
