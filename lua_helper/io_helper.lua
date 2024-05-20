Disk = {}
File = {}
Folder = {}

function Disk.BitLockerProtection(path)
  return App.Call('volume::bitlockerprotection', path)
end

function Disk.IsLocked(path)
  return Disk.BitLockerProtection(path) == 6
end

function File.Exists(path)
  return App.Call('file::exists', path) == 1
end

function File.Delete(path)
  local f = App.Call('envstr', path)
  return os.remove(f)
end

File.Remove = File.Delete

function File.GetFullPath(path)
  return App.Call('path::getfullpath', path)
end

function File.GetShortPath(path)
  return App.Call('path::getshortpath', path)
end

function Folder.Exists(path)
  return App.Call('folder::exists', path) == 1
end

Folder.GetFullPath = File.GetFullPath
Folder.GetShortPath = File.GetShortPath

function os.exists(path)
  if path == nil then return 1 end
  local f = App.Call('envstr', path)
  if File.Exists(path) then return 1 end
  if Folder.Exists(path) then return 1 end
  return 0
end
