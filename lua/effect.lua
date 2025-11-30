local M = {}

---@class Effect
---@field kind EffectType

---@alias EffectType
---| "error"
---| "network"
---| "file"

--- Used to report error in an environment where there is no console output
---@param msg string error message
---@return Effect
function M.error(msg)
  return {
    kind = "error",
    data = { message = msg }
  }
end

--@alias Mode
--| "w" # truncate the file if it exists, or create it
--| "a" # append to the file if it exists, or create it
--| "x" # fail if the file exists, create it otherwise

---@class Mode
---@field Write "w"
---@field Append "a"
---@field Exclusive "x"
local Mode = {
  Write = "w",
  Append = "a",
  Exclusive = "x"
}

---@class FileEffectOpts
---@field mode Mode|nil # how the file should be opened for writing, default to "w"
---@field path string # file path relative to the granted folder

---@param content string # content to be written to the file
---@param opts FileEffectOpts # options controlling how the file is written
---@return Effect
function M.file(content, opts)
  return {
    kind = "file",
    data = { content = content, opts = opts }
  }
end

---Network modes determine *what kind of outbound network action* this effect represents.
---@class NetworkMode
---@field Post "post" # send data to a remote endpoint (body required)
---@field Put "put" # replace data at the endpoint (body required)
---@field Patch "patch" # partially update data (body required)
---@field Send "send" # raw send, no assumptions about protocol/body
local NetworkMode = {
  Post = "post",
  Put = "put",
  Patch = "patch",
  Send = "send",
}


---@class NetworkEffectOpts
---@field url string            # remote address or endpoint
---@field mode NetworkMode      # what type of network operation to perform
---@field headers table<string,string>|nil # optional HTTP-like headers
---@field timeout number|nil    # optional timeout in milliseconds

---@param content string|nil    # content to be sent
---@param opts NetworkEffectOpts
---@return Effect
function M.network(content, opts)
  return {
    kind = "network",
    data = { content = content, opts = opts }
  }
end

return M
