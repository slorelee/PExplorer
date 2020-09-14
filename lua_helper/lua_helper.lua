app:print("lua_helper init ...")

-- short alias
str = str or string
p = p or print

if os.getenv('PROCESSOR_ARCHITECTURE') == 'AMD64' then
  ARCH = 'x64'
else
  ARCH = 'x86'
end

-- update package.path, package.cpath

local root = os.getenv('WINXSHELL_MODULEPATH')
if os.getenv('WINXSHELL_DEBUG') then
  root = root .. [[\..\..]]
end
local luapath = '.\\Libs\\?.lua;'
if root then
  luapath = root .. '.\\Libs\\?.lua;'
end
package.path = luapath .. package.path

local dllpath = str.format('.\\Libs\\%s\\?.dll;', ARCH)
if root then
  dllpath = str.format(root .. '\\Libs\\%s\\?.dll;', ARCH)
end
package.cpath = dllpath .. package.cpath
