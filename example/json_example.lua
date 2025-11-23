-- From the root of the run with
-- $ LUA_PATH="./dist/share/lua/5.4/?.lua;dist/share/lua/5.4/?/init.lua;;"  LUA_CPATH="./dist/lib/lua/5.4/?.so;;" lua example/json_example.lua

local json = require "example.json"
local P = require "parser"

local data = [[
{
  "name": "John",
  "age": 30,
  "nums": [1, 2, 3],
  "active": true,
  "nested": { "x": 10 }
}
]]

local result, err = json.parse_json(data)

if result then
  P.utils.print(result, 0)
else
  print("Error:", err)
end
