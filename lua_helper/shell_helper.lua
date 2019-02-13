require 'reg_helper'

Taskbar = {}
local regkey_setting = [[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced]]

function Taskbar:GetSetting(key, value)
  return reg_read(regkey_setting, key, value)
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

