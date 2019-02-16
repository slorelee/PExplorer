require('lua_helper.lua_helper')
require 'winapi'

WM_SYSCOMMAND = 0x0112
SC_CLOSE      = 0xf060

ICP_TIMER_ID = 10001 -- init control panel timer

cmd_line = app:info('cmdline')
app_path = app:info('path')
win_ver = app:info('winver')
is_x = (os.getenv("SystemDrive") == 'X:')
is_pe = (app:info('iswinpe') == 1)                              -- Windows Preinstallation Environment
is_wes = (string.find(cmd_line, '-wes') and true or false)      -- Windows Embedded Standard
is_win = (string.find(cmd_line, '-windows') and true or false)  -- Normal Windows

--[[ add one more '-' to be '---', will enable this function
function do_ocf(lnkfile, realfile) -- handle open containing folder menu
  -- local path = realfile:match('(.+)\\')
  -- app:run('cmd', '/k echo ' .. path)
  app:run('X:\\Progra~1\\TotalCommander\\TOTALCMD64.exe', '/O /T /A \"' .. realfile .. '\"')
end
--]]

function onload()
  -- app:call('run', 'notepad.exe')
  -- app:run('notepad.exe')
  app:print('WinXShell.exe loading...')
  app:print('CommandLine:' .. cmd_line)
  app:print('WINPE:'.. tostring(is_pe))
  app:print('WES:' .. tostring(is_wes))
end

function ondaemon()
  regist_shortcut_ocf()
  regist_system_property()
  regist_ms_settings_url()
end

function onshell()
  regist_shortcut_ocf()
  regist_system_property()
  regist_ms_settings_url()
end

function onfirstrun()
  -- VERSTR = reg_read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]], 'CurrentVersion')
  if is_wes then
    if win_ver == '6.2' or win_ver == '6.3' then -- only Windows 8, 8.1
      app:call('SetTimer', ICP_TIMER_ID, 200) -- use timer to make main shell running
    end
  end
end

function ms_settings(url)
    app:print(url)
    local exe = app_path .. '\\WinXShell.exe'
    if url == "ms-settings:taskbar" then
      app:run(exe, ' -ui -jcfg wxsUI\\UI_Settings\\main.jcfg')
    elseif url == "ms-settings:dateandtime" then
      app:run('timedate.cpl')
    elseif  url == "ms-settings:display" then
      app:run(exe, ' -ui -jcfg wxsUI\\UI_Resolution\\main.jcfg')
    elseif url == "ms-settings:personalization-background" then
      app:run('control.exe')
    else
      app:run('control.exe')
    end
end

function onclick(ctrl)
  if ctrl == 'startmenu_reboot' then
    return onclick_startmenu_reboot()
  elseif ctrl == 'startmenu_shutdown' then
    return onclick_startmenu_shutdown()
  elseif ctrl == 'startmenu_controlpanel' then
    return onclick_startmenu_controlpanel()
  elseif ctrl == 'tray_clockarea' then
    return onclick_tray_clockarea()
  elseif ctrl == 'tray_clockarea(double)' then
    return onclick_tray_clockarea(true)
  end
  return 1 -- continue shell action
end

local function power_helper(wu_param, sd_param)
  local sd = os.getenv("SystemDrive")
  if File.exists(sd ..'\\Windows\\System32\\Wpeutil.exe') then
    app:run('wpeutil.exe', wu_param, 0) -- SW_HIDE(0)
    return 0
  elseif File.exists(sd ..'\\Windows\\System32\\shutdown.exe') then
    app:run('shutdown.exe', sd_param .. ' -t 0')
    return 0
  end
  return 1
end

local function reboot()
    return power_helper('Reboot', '-r')
end

local function shutdown()
    return power_helper('Shutdown', '-s')
end

function onclick_startmenu_reboot()
  -- restart computer directly
  -- reboot()
  app:run(app_path .. '\\WinXShell.exe', ' -ui -jcfg wxsUI\\UI_Shutdown.zip\\full.jcfg')
  return 0
  -- return 1 -- for call system dialog
end

function onclick_startmenu_shutdown()
  -- restart computer directly
  -- shutdown()
  app:run(app_path .. '\\WinXShell.exe', ' -ui -jcfg wxsUI\\UI_Shutdown.zip\\full.jcfg')
  return 0
  -- return 1 -- for call system dialog
end

function onclick_startmenu_controlpanel()
  if is_wes then
    app:run('control.exe')
    return 0
  end
  return 1
end

function onclick_tray_clockarea(isdouble)
  if isdouble then
    app:run('control.exe', 'timedate.cpl')
  else
    app:run(app_path .. '\\WinXShell.exe', ' -ui -jcfg wxsUI\\UI_Calendar\\Calendar.jcfg')
  end
  return 0
end


function ontimer(tid)
  if tid == ICP_TIMER_ID then
    initcontrolpanel(win_ver)
    app:call('KillTimer', tid)
  end
end

-- ======================================================================================
function initcontrolpanel(ver)
  local ctrlpanel_title = ''
  --  4161    Control Panel
  -- 32012    All Control Panel Items
  if ver == '6.1' then
    ctrlpanel_title = app:call('resstr', '#{@shell32.dll,4161}')
  else
    ctrlpanel_title = app:call('resstr', '#{@shell32.dll,32012}')
  end
  app:print(ctrlpanel_title)
  app:run('control.exe')
  app:call('sleep', 500)
  local cp_win = winapi.find_window('CabinetWClass', ctrlpanel_title)
  app:print(string.format("Control Panel Handle:0x%x", cp_win:get_handle()))
  cp_win:send_message(WM_SYSCOMMAND, SC_CLOSE, 0)
end

function regist_ms_settings_url()
  if win_ver ~= "10.0" then return end
  local key = [[HKEY_CLASSES_ROOT\ms-settings]]
  local val = reg_read(key .. [[\Shell\Open\Command]], 'DelegateExecute')
  app:print(val)
  if val == nil or val ~= '{C59C9814-F038-4B71-A341-6024882458AF}' then
    reg_write(key, '', 'URL:ms-settings')
    reg_write(key, 'URL Protocol', '')
    reg_write(key .. [[\Shell\Open\Command]], 'DelegateExecute', '{C59C9814-F038-4B71-A341-6024882458AF}')
  end
  app:run(app_path .. '\\WinXShell.exe', ' -Embedding')
end

function regist_system_property() -- handle This PC's property menu
    if not is_x then return end
    --if File.exists('X:\\Windows\\explorer.exe') then return end

    local key = [[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties]]
    if reg_read(key, '') then return end -- already exists
    -- show This PC on the Desktop
    reg_write([[HKEY_USERS\.DEFAULT\Software\Microsoft\Windows\CurrentVersion\Explorer\HideDesktopIcons\NewStartPanel]], '{20D04FE0-3AEA-1069-A2D8-08002B30309D}', 0, winapi.REG_DWORD)
    -- handle Property menu to UI_SystemInfo
    reg_write([[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties]], '', '@shell32.dll,-33555')
    reg_write([[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties]], 'Position', 'Bottom')
    reg_write([[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties\command]], '', app_path .. '\\WinXShell.exe -ui -jcfg wxsUI\\UI_SystemInfo\\main.jcfg')

end

function regist_shortcut_ocf() -- handle shortcut's OpenContainingFolder menu
    if not is_x then return end
    if File.exists('X:\\Windows\\System32\\ieframe.dll') then return end

    local key = [[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]]
    if reg_read(key, '') then return end -- already exists
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], '', 'Open Containing Folder Menu Wrap')
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], '#MUIVerb', '@shell32.dll,-1033')
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], 'Extended', '')
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], 'Position', 'Bottom')
    local explorer_opt = ''
    if File.exists('X:\\Windows\\explorer.exe') then explorer_opt = '-explorer' end
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub\command]], '', app_path .. '\\WinXShell.exe '.. explorer_opt ..' -ocf \"%1\"')

    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shellex\ContextMenuHandlers\OpenContainingFolderMenu]], '', 'disable-{37ea3a21-7493-4208-a011-7f9ea79ce9f5}')
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shellex\ContextMenuHandlers\OpenContainingFolderMenu_wxsStub]], '', '{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}')
    reg_write([[HKEY_CLASSES_ROOT\lnkfile\shellex\PropertySheetHandlers\wxsStub]], '', '{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}')

    reg_write([[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}]], '', 'wxsStub')
    local stub_dll = 'wxsStub.dll'
    if ARCH ~= 'x64' then stub_dll = 'wxsStub32.dll' end
    reg_write([[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], '', app_path .. '\\' .. stub_dll)
    reg_write([[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], 'ThreadingModel', 'Apartment')

    if ARCH == 'x64' then
      reg_write([[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}]], '', 'wxsStub')
      reg_write([[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], '', app_path .. '\\wxsStub32.dll')
      reg_write([[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], 'ThreadingModel', 'Apartment')
    end

end
