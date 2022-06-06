ICP_TIMER_ID = 10001 -- init control panel timer

cmd_line = App:Info('cmdline')
app_path = App:Info('path')
win_ver = App:Info('winver')
is_x = (os.getenv("SystemDrive") == 'X:')
is_pe = (App:Info('iswinpe') == 1)                              -- Windows Preinstallation Environment
is_wes = (string.find(cmd_line, '-wes') and true or false)      -- Windows Embedded Standard
is_win = (string.find(cmd_line, '-windows') and true or false)  -- Normal Windows

-- 'auto', 'ui_systemInfo', 'system', '' or nil
handle_system_property = 'auto'

--[[ add one more '-' to be '---', will enable this function
function do_ocf(lnkfile, realfile) -- handle open containing folder menu
  -- local path = realfile:match('(.+)\\')
  -- App:Run('cmd', '/k echo ' .. path)

  -- totalcmd
  App:Run('X:\\Progra~1\\TotalCommander\\TOTALCMD64.exe', '/O /T /A \"' .. realfile .. '\"')
  -- XYplorer
  App:Run('X:\\Progra~1\\XYplorer\\XYplorer.exe', '/select=\"' .. realfile .. '\"')
end
--]]

function App:onLoad()
  -- App:Run('notepad.exe')
  print('WinXShell.exe loading...')
  print('CommandLine:' .. cmd_line)
  print('WINPE:'.. tostring(is_pe), 123, 'test', App)
  print('WES:' .. tostring(is_wes))
end

function App:onDaemon()
  regist_shortcut_ocf()
  regist_system_property()
  regist_protocols()
end

function App:onShell()
  regist_folder_shell()
  regist_shortcut_ocf()
  regist_system_property()
  regist_protocols()

  -- wxsUI('UI_WIFI', 'main.jcfg', '-notrayicon -hidewindow')
  -- wxsUI('UI_Volume', 'main.jcfg', '-notrayicon -hidewindow')
end

-- if you want to use the custom event function,
-- rename the function name to be ondisplaychanged()
function ondisplaychanged_sample()
  local cur_res_x = Screen:GetX()
  if last_res_x == cur_res_x then return end
  last_res_x = cur_res_x
  if last_res_x >= 3840 then
    Screen:DPI(150)
  elseif last_res_x >= 1440 then
    Screen:DPI(125)
  elseif last_res_x >= 800 then
    Screen:DPI(100)
  end
end

-- return the resource id for startmenu logo
function startmenu_logoid()
  local map = {
    ["none"] = 0, ["windows"] = 1, ["winpe"] = 2,
    ["custom1"] = 11, ["custom2"] = 12, ["custom3"] = 13,
    ["default"] = 1
  }
  -- use next line for custom (remove "--" and change "none" to what you like)
  -- if true then return map["none"] end
  if is_pe then return map["winpe"] end
  return map["windows"]
end

-- if you want to use the custom clock text,
-- rename the function name to be update_clock_text()
-- sample for:
--[[
    |  22:00 Sat  |
    |  2019-9-14  |
]]
-- FYI:https://www.lua.org/pil/22.1.html
function update_clock_text_sample()
  local now_time = os.time()
  local clocktext = os.date('%H:%M %a\r\n%Y-%m-%d', now_time)
  app:call('SetVar', 'ClockText', clocktext)
end

function App:onFirstShellRun()
  -- VERSTR = reg_read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]], 'CurrentVersion')
  if is_wes then
    if win_ver == '6.2' or win_ver == '6.3' then -- only Windows 8, 8.1
      app:call('SetTimer', ICP_TIMER_ID, 200) -- use timer to make main shell running
    end
  end
end

function Shell:onClick(ctrl)
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

function onclick_startmenu_reboot()
  -- restart computer directly
  -- System:Reboot()
  wxsUI('UI_Shutdown', 'full.jcfg')
  return 0
  -- return 1 -- for call system dialog
end

function onclick_startmenu_shutdown()
  -- shutdown computer directly
  -- System::Shutdown()
  wxsUI('UI_Shutdown', 'full.jcfg')
  return 0
  -- return 1 -- for call system dialog
end

function onclick_startmenu_controlpanel()
  if is_wes then
    App:Run('control.exe')
    return 0
  end
  return 1
end

function onclick_tray_clockarea(isdouble)
  if isdouble then
    App:Run('control.exe', 'timedate.cpl')
  else
    wxsUI('UI_Calendar', 'main.jcfg')
  end
  return 0
end


function App:onTimer(tid)
  if tid == ICP_TIMER_ID then
    initcontrolpanel(win_ver)
    App:KillTimer(tid)
  end
end

-- ======================================================================================
function initcontrolpanel(ver)
  --  4161    Control Panel
  local ctrlpanel_title = app:call('resstr', '#{@shell32.dll,4161}')
  App:Run('control.exe')
  App:Sleep(500)
  if CloseWindow('CabinetWClass', ctrlpanel_title) == 0 then
    -- 32012    All Control Panel Items
    ctrlpanel_title = app:call('resstr', '#{@shell32.dll,32012}')
    CloseWindow('CabinetWClass', ctrlpanel_title)
  end
end

