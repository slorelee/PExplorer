require 'reg_helper'

System = {}
local regkey_colortheme = [[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize]]

function System:GetSetting(key)
    if key == 'AppsColorTheme' then
        return (reg_read(regkey_colortheme, 'AppsUseLightTheme') or 1) | 0 -- convert to integer
    end
    return 0
end

function System:ColorTheme(mode)
    if mode == 'light' then
        reg_write(regkey_colortheme, 'AppsUseLightTheme', 1, winapi.REG_DWORD)
    else
        reg_write(regkey_colortheme, 'AppsUseLightTheme', 0, winapi.REG_DWORD)
    end
    app:call('System::ChangeColorThemeNotify')
end


Taskbar = {}
local regkey_setting = [[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced]]

function Taskbar:GetSetting(key)
  if key == 'AutoHide' then return app:call('Taskbar::AutoHideState') end
  return reg_read(regkey_setting, key)
end

function Taskbar:SetSetting(key, value, type)
  return reg_write(regkey_setting, key, value, type)
end

function Taskbar:CombineButtons(value, update)
  if value == 'always' then value = 0
  elseif value == 'auto' then value = 1
  else value = 2 end --never

  Taskbar:SetSetting('TaskbarGlomLevel', value, winapi.REG_DWORD)
  if update ~= 0 then app:call('Taskbar::ChangeNotify') end
end

function Taskbar:UseSmallIcons(value, update)
  Taskbar:SetSetting('TaskbarSmallIcons', value, winapi.REG_DWORD)
  if update ~= 0 then app:call('Taskbar::ChangeNotify') end
end

function Taskbar:AutoHide(value)
  app:call('Taskbar::AutoHide', value)
end

local pintotaskbar = 0
local function menu_pintotaskbar()
  if pintotaskbar == 1 then return end
  pintotaskbar = 1
  local regkey = [[HKEY_CLASSES_ROOT\exefile\shell\pintotaskbar]]
  local handler = reg_read(regkey, 'ExplorerCommandHandler')
  if handler ~= nil then return end

  reg_write(regkey, 'ExplorerCommandHandler', '{90AA3A4E-1CBA-4233-B8BB-535773D48449}')
  reg_write(regkey, 'Extended', '')
  reg_write(regkey, 'MUIVerb', '@shell32.dll,-37423')
end

function Taskbar:Pin(target, name, param, icon, index, showcmd)
  menu_pintotaskbar()
  local lnk = target
  local ext = string.sub(target, -4)
  local case_ext = string.lower(ext)

  if case_ext ~= '.exe' and case_ext ~= '.lnk' then return end
  if case_ext == '.exe' then
    lnk = "%TEMP%\\TaskBarPinned\\"
    if name ~= nil then
      lnk = lnk .. name
    else
      lnk = lnk .. string.match(target, '([^\\]+)' .. ext .. '$')
    end
    lnk = lnk .. '.lnk'
    app:call('link', lnk, target, param, icon, index, showcmd)
  end
  app:call('Taskbar::Pin', lnk)
end

function Taskbar:UnPin(name)
  menu_pintotaskbar()
  app:call('Taskbar::UnPin', name)
end

Screen = {}

local function fixscreen()
  app:call('Desktop::UpdateWallpaper')
  app:call('sleep', 200)
  app:call('Taskbar::ChangeNotify')
end

function  Screen:Get(...)
  return app:call('Screen::Get', ...)
end

function Screen:GetX()
  return app:call('Screen::Get', 'x')
end

function Screen:GetY()
  return app:call('Screen::Get', 'y')
end

function Screen:GetRotation()
  return app:call('Screen::Get', 'rotation')
end

function Screen:Disp(w, h)
  local ret = app:call('Screen::Set', 'resolution', w, h)
  if ret == 0 then
    fixscreen()
  end
  return ret
end

-- arr = {'1152x864', '1366x768', '1024x768'}
function Screen:DispTest(arr)
  local i, w, h, ret = 0
  for i = 1, #arr do
    w, h = string.match(arr[i], '(%d+)[x*](%d+)')
    if h ~= nil then
      app:print(w, h)
      if Screen:Disp(tonumber(w), tonumber(h)) == 0 then return end
    end
  end
end
