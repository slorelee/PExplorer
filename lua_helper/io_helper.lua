File = {}

function File.exists(path)
  local f = io.open(path, "rb")
  if f then f:close() end
  return f ~= nil
end
