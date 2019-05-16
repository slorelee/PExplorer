File = {}

function File.exists(path)
  local f = io.open(path, "rb")
  if f then f:close() end
  return f ~= nil
end

function File.delete(path)
  local f = app:call('envstr', path)
  return os.remove(f)
end

File.remove = File.delete
