-- require 'reg_helper'

System = {}
local regkey_user = 'HKEY_CURRENT_USER'
if os.getenv('USERNAME') == 'SYSTEM' then regkey_user = 'HKEY_LOCAL_MACHINE' end
local regkey_colortheme = [[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize]]

function System:GetSetting(key)
  if key == 'AppsColorTheme' then
    return (Reg:Read(regkey_colortheme, 'AppsUseLightTheme') or 1) | 0 -- convert to integer
  elseif key == 'SysColorTheme' then
    return (Reg:Read(regkey_colortheme, 'SystemUsesLightTheme') or 1) | 0
  elseif key == 'ShellColorPrevalence' then
    return (Reg:Read(regkey_colortheme, 'ColorPrevalence') or 1) | 0
  elseif key == 'WindowColorPrevalence' then
    local regkey = regkey_user .. [[\SOFTWARE\Microsoft\Windows\DWM]]
    return (Reg:Read(regkey, 'ColorPrevalence') or 1) | 0
  elseif key == 'Colors.Transparency' then
    return (Reg:Read(regkey_colortheme, 'EnableTransparency') or 1) | 0
  end
  return 0
end

function System:SetSetting(key, val)
  if val ~= 0 then val = 1 end
  if key == 'ShellColorPrevalence' then
    Reg:Write(regkey_colortheme, 'ColorPrevalence', val, winapi.REG_DWORD)
    App:Call('System::ChangeColorThemeNotify')
  elseif key == 'WindowColorPrevalence' then
    local regkey = regkey_user .. [[\SOFTWARE\Microsoft\Windows\DWM]]
    Reg:Write(regkey, 'ColorPrevalence', val, winapi.REG_DWORD)
  elseif key == 'Colors.Transparency' then
    Reg:Write(regkey_colortheme, 'EnableTransparency', val, winapi.REG_DWORD)
    App:Call('System::ChangeColorThemeNotify')
  end
  return 0
end

function System:SysColorTheme(mode)
    if mode == 'light' then
        Reg:Write(regkey_colortheme, 'SystemUsesLightTheme', 1, winapi.REG_DWORD)
    else
        Reg:Write(regkey_colortheme, 'SystemUsesLightTheme', 0, winapi.REG_DWORD)
    end
    App:Call('System::ChangeColorThemeNotify')
end

function System:AppsColorTheme(mode)
    if mode == 'light' then
        Reg:Write(regkey_colortheme, 'AppsUseLightTheme', 1, winapi.REG_DWORD)
    else
        Reg:Write(regkey_colortheme, 'AppsUseLightTheme', 0, winapi.REG_DWORD)
    end
    App:Call('System::ChangeColorThemeNotify')
end

function System:ReloadCursors()
    App:Call('system::setcursors')
end

local function power_helper(wu_param, sd_param)
  local sd = os.getenv("SystemDrive")
  if File.Exists(sd ..'\\Windows\\System32\\Wpeutil.exe') then
    App:Run('wpeutil.exe', wu_param, 0) -- SW_HIDE(0)
    return 0
  elseif File.exists(sd ..'\\Windows\\System32\\shutdown.exe') then
    App:Run('shutdown.exe', sd_param .. ' -t 0')
    return 0
  end
  return 1
end

function System:CreatePageFile(file, min, max)
  return App:Call('System::CreatePageFile', file, min, max);
end

function System:NetJoin(domain, joinOpt, server, accountOU, account, password)
  domain = domain or 'WORKGROUP'
  return os.rundll('Netapi32.dll', 'NetJoinDomain', server, domain, accountOU, account, password, joinOpt)
end

function System:EnableEUDC(fEnableEUDC)
  return App:Call('System::EnableEUDC', fEnableEUDC);
end

function System:AppxSysprepInit()
  return App:Call('System::AppxSysprepInit');
end

function System:Reboot()
  return power_helper('Reboot', '-r')
end

function System:Shutdown()
  return power_helper('Shutdown', '-s')
end


WinPE = {}
function WinPE:SystemInit()
  System:EnableEUDC(1)
  System:AppxSysprepInit()
end

local function PinCommand(class, target, name, param, icon, index, showcmd)
  local ext = string.sub(target, -4)
  local case_ext = string.lower(ext)

  local pinned_path = [[%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned\]] .. class .. '\\'
  local lnk_name = name
  if lnk_name == nil then lnk_name = string.match(target, '([^\\]+)' .. ext .. '$') end
  lnk_name = lnk_name .. '.lnk'
  if case_ext == '.lnk' then
    if App:Info('isshell') ~= 0 then
      exec('/hide', 'cmd.exe /c copy /y \"' .. target .. '\" \"' .. pinned_path .. lnk_name .. '\"')
    else
      App:Call(class .. '::Pin', target)
    end
    return
  end
  if case_ext ~= '.exe' then return end

  local lnk = target
  if name ~= nil or param ~= nil or icon ~= nil then
    if App:Info('isshell') ~= 0 then
      lnk = pinned_path .. lnk_name
    else
      lnk = '%TEMP%\\' .. class .. 'Pinned\\' .. lnk_name
    end
    App:Call('link', lnk, target, param, icon, index, showcmd)
  end
  if App:Info('isshell') ~= 0 then
    if lnk == target then
      lnk = pinned_path .. lnk_name
      App:Call('link', lnk, target)
    end
  else
    App:Call(class .. '::Pin', lnk)
  end
end

Shell ={}
function Shell:Close()
  App:Call('closeshell')
  App:Sleep(500)
end

function Shell:WaitAndClose()
  Taskbar:WaitForReady()
  Shell:Close()
end

Desktop = {}
Desktop.Path = ""

function Desktop:Refresh()
  App:Call('Desktop::Refresh')
end

function Desktop:GetPath()
  return App:Call('Desktop::GetPath')
end

function Desktop:GetWallpaper()
  return App:Call('Desktop::Getwallpaper')
end

function Desktop:SetWallpaper(wallpaper)
  App:Call('Desktop::Setwallpaper', wallpaper)
end

function Desktop:SetIconSize(size)
  App:Call('Desktop::SetIconSize', size)
end

function Desktop:AutoArrange(checked)
  App:Call('Desktop::AutoArrange', checked)
end

function Desktop:SnapToGrid(checked)
  App:Call('Desktop::SnapToGrid', checked)
end

function Desktop:ShowIcons(checked)
  App:Call('Desktop::ShowIcons', checked)
end

function Desktop:Link(lnk, ...)
  os.link(Desktop.Path .. '\\' .. lnk , ...)
end

Desktop.Path = Desktop:GetPath()


Taskbar = {}
local regkey_setting = [[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced]]

function Taskbar:IsReady(sec)
  local sh_win = winapi.find_window('Shell_TrayWnd', nil)
  local n = -1
  while (n <= sec and (sh_win == nil or sh_win:get_handle() == 0)) do
    App:Print("shell Handle:0x0")
    App:Sleep(1000)
    sh_win = winapi.find_window('Shell_TrayWnd', nil)
    if sec ~= -1 then n = n + 1 end
  end
  if sh_win == nil or sh_win:get_handle() == 0 then
    return false
  end
  return true
end

function Taskbar:WaitForReady()
  Taskbar:IsReady(-1)
end

function Taskbar:GetSetting(key)
  if key == 'AutoHide' then return App:Call('Taskbar::AutoHideState') end
  return Reg:Read(regkey_setting, key)
end

function Taskbar:SetSetting(key, value, type)
  return Reg:Write(regkey_setting, key, value, type)
end

function Taskbar:CombineButtons(value, update)
  if value == 'always' then value = 0
  elseif value == 'auto' then value = 1
  else value = 2 end --never

  Taskbar:SetSetting('TaskbarGlomLevel', value, winapi.REG_DWORD)
  if update ~= 0 then App:Call('Taskbar::ChangeNotify') end
end

function Taskbar:UseSmallIcons(value, update)
  Taskbar:SetSetting('TaskbarSmallIcons', value, winapi.REG_DWORD)
  if update ~= 0 then App:Call('Taskbar::ChangeNotify') end
end

function Taskbar:AutoHide(value)
  App:Call('Taskbar::AutoHide', value)
end

function Taskbar:Pin(target, name, param, icon, index, showcmd)
  PinCommand('Taskbar',target, name, param, icon, index, showcmd)
end

function Taskbar:UnPin(name)
  App:Call('Taskbar::UnPin', name)
end

function Taskbar:Show()
    Window.Find(nil, 'Shell_TrayWnd'):Show()
end

function Taskbar:Hide()
    Window.Find(nil, 'Shell_TrayWnd'):Hide()
end

Startmenu = {}
Startmenu.ProgramsPath = ""

function Startmenu:GetProgramsPath()
  return App:Call('Startmenu::GetProgramsPath')
end

function Startmenu:Pin(target, name, param, icon, index, showcmd)
  PinCommand('Startmenu',target, name, param, icon, index, showcmd)
end

function Startmenu:UnPin(target)
  App:Call('startmenu::unpin', target)
end

function Startmenu:Link(lnk, ...)
  os.link(Startmenu.ProgramsPath .. '\\' .. lnk , ...)
end

Startmenu.ProgramsPath = Startmenu:GetProgramsPath()


Screen = {}

function Screen:Adjust()
  App:Call('Desktop::UpdateWallpaper')
  App:Sleep(200)
  App:Call('Taskbar::ChangeNotify')
end

function  Screen:Get(...)
  return App:Call('Screen::Get', ...)
end

function  Screen:Set(...)
  return App:Call('Screen::Set', ...)
end

function Screen:GetX()
  return App:Call('Screen::Get', 'x')
end

function Screen:GetY()
  return App:Call('Screen::Get', 'y')
end

function Screen:GetRotation()
  return App:Call('Screen::Get', 'rotation')
end

function Screen:GetDPI()
  return App:Call('Screen::Get', 'dpi')
end

function Screen:Disp(w, h)
  local ret = -1
  if w == nil then
    ret = App:Call('Screen::Set', 'maxresolution')
  else
    ret = App:Call('Screen::Set', 'resolution', w, h)
  end
  Screen:Adjust()
  return ret
end

-- arr = {'1152x864', '1366x768', '1024x768'}
function Screen:DispTest(arr)
  local i, w, h, ret = 0
  for i = 1, #arr do
    w, h = string.match(arr[i], '(%d+)[x*](%d+)')
    if h ~= nil then
      App:Print(w, h)
      if Screen:Disp(tonumber(w), tonumber(h)) == 0 then return end
    end
  end
end

function Screen:DPI(scale)
  App:Call('Screen::Set', 'dpi', scale)
end


Volume = {}

function Volume:GetName()
  return App:Call('Volume::GetName')
end

function Volume:GetLevel()
  return App:Call('Volume::GetLevel')
end

function Volume:SetLevel(...)
  return App:Call('Volume::SetLevel', ...)
end

function Volume:IsMuted()
  return App:Call('Volume::IsMuted')
end

function Volume:Mute(...)
  return App:Call('Volume::Mute', ...)
end


FolderOptions = {}

-- Opt =
--   'ShowAll'     - Show the hidden files / folders
--   'ShowExt'     - Show the known extension
--   'ShowSuperHidden' - Always hide the system files / folders

function FolderOptions:Set(opt, val)
  App:Call('FolderOptions::Set', opt, val)
end

function FolderOptions:Get(opt)
  return App:Call('FolderOptions::Get', opt)
end

function FolderOptions:Toggle(opt)
  local val = FolderOptions:Get(opt)
  FolderOptions:Set(opt, val - 1)
end

Dialog = {}
function Dialog:OpenFile(...)
  return App:Call('Dialog::OpenFile', ...)
end

function Dialog:SaveFile(...)
  return App:Call('Dialog::SaveFile', ...)
end

function Dialog:OpenSaveFile(...)
  return App:Call('Dialog::OpenSaveFile', ...)
end

function Dialog:BrowseFolder(...)
  return App:Call('Dialog::BrowseFolder', ...)
end

-- Helper(alias)
function LinkToDesktop(...) Desktop:Link(...) end
function LinkToStartmenu(...) Startmenu:Link(...) end

function PinToTaskbar(...) Taskbar:Pin(...) end
function PinToStartMenu(...) Startmenu:Pin(...) end
