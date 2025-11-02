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
    :drop_for(M.literal('"')
      :drop_for(
        M.any_char()
        :pred(function(char) return char ~= '"'
        end):zero_or_more():take_after(M.literal('"'))
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

function M.utils.tables_equal(t1, t2, visited)
    visited = visited or {}
    if visited[t1] and visited[t1][t2] then
        return true
    end

    if t1 == t2 then return true end
    if type(t1) ~= "table" or type(t2) ~= "table" then return false end

    visited[t1] = visited[t1] or {}
    visited[t1][t2] = true

    for k, v in pairs(t1) do
        local v2 = t2[k]
        if type(v) == "table" and type(v2) == "table" then
            if not M.utils.tables_equal(v, v2, visited) then return false end
        elseif v2 ~= v then
            return false
        end
    end

    -- check for extra keys in t2
    for k, _ in pairs(t2) do
        if t1[k] == nil then return false end
    end
    return true
end


return M
