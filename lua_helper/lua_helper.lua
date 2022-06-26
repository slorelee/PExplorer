App:Debug("lua_helper init ...")

if os.getenv('PROCESSOR_ARCHITECTURE') == 'AMD64' then
  ARCH = 'x64'
else
  ARCH = 'x86'
end

-- update package.path, package.cpath

local root = App:Info('Path')
if App:Info('IsDebugModule') then
  root = root .. [[\..\..]]
end
local luapath = '.\\Libs\\?.lua;'
if root then
  luapath = root .. '.\\Libs\\?.lua;'
end
package.path = luapath .. package.path

local dllpath = string.format('.\\Libs\\%s\\?.dll;', ARCH)
if root then
  dllpath = string.format(root .. '\\Libs\\%s\\?.dll;', ARCH)
end
package.cpath = dllpath .. package.cpath

print = App.Print

-- short alias
str = str or string
str.fmt = str.format
p = p or print


