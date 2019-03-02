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

