File = {}
Folder = {}

function File.Exists(path)
  return App.Call('file::exists', path) == 1
end

function File.Delete(path)
  local f = App.Call('envstr', path)
  return os.remove(f)
end

File.Remove = File.Delete

function Folder.Exists(path)
  return App.Call('folder::exists', path) == 1
end

function os.exists(path)
  if path == nil then return 1 end
  local f = App.Call('envstr', path)
  if File.Exists(path) then return 1 end
  if Folder.Exists(path) then return 1 end
  return 0
end