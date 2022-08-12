-- require('io_helper')

WxsHandler = {}
TrayNotify = {}
TrayClock = {}

-- 'auto', 'ui_systemInfo', 'system', '' or nil
WxsHandler.SystemProperty = 'auto'
WxsHandler.HandleShowDesktop = true

 -- nil or a handler function
WxsHandler.OpenContainingFolder = nil
WxsHandler.DisplayChangedHandler = nil

WxsHandler.TrayClockTextFormatter = nil


function wxsUI(ui, jcfg, opt, app_path)
  if jcfg == nil then jcfg = 'main.jcfg' end
  if opt == nil then opt = '' else opt = ' ' .. opt end
  if app_path == nil then app_path = App.FullPath end
  if File.Exists(App.Path .. '\\wxsUI\\' .. ui .. '.zip') then
    ui = ui .. '.zip'
  end
  App:Run(app_path, ' -ui -jcfg wxsUI\\' .. ui .. '\\' .. jcfg .. opt)
end

function wxs_ui(url)
    App:Debug(url)
    if string.find(url, 'wxs-ui:', 1, true) then
      url = url:gsub('wxs[-]ui:', '')
    end

    if url == "systeminfo" then
      wxsUI('UI_SystemInfo')
    elseif url == "settings" then
      wxsUI('UI_Settings', 'main.jcfg', '-fixscreen')
    elseif url == "wifi" then
      wxsUI('UI_WIFI')
    elseif url == "volume" then
      wxsUI('UI_Volume')
    end
end

function wxs_open(url)
    App:Debug(url)
    local sd = os.getenv('SystemDrive')
    if string.find(url, 'wxs-open:', 1, true) ~= nil then
      url = url:gsub('wxs[-]open:', '')
    end

    if url == 'controlpanel' then
      if not File.Exists(sd .. '\\Windows\\explorer.exe') then return end
      App:Run('control.exe')
    elseif url == 'system' then
      App:Call('wxs_open', 'system')
    elseif url == 'netsettings' then
      if File.Exists(sd .. '\\Windows\\System32\\netcenter.dll') then
        App:Call('wxs_open', 'networkcenter')
      elseif File.Exists(sd .. '\\Windows\\System32\\netshell.dll') then
        App:Call('wxs_open', 'networkconnections')
      end
    elseif url == 'networkconnections' then
      App:Call('wxs_open', 'networkconnections')
    elseif url == 'printers' then
      App:Call('wxs_open', 'printers')
    elseif url == 'userslibraries' then
      App:Call('wxs_open', 'userslibraries')
    elseif url == 'devices' then
      App:Call('wxs_open', 'devicesandprinters')
    elseif url == 'wifi' then
      wxsUI('UI_WIFI')
    elseif url == 'volume' then
      wxsUI('UI_Volume')
    end
end

function ms_settings(url)
    App:Debug(url)
    if string.find(url, 'ms-settings:', 1, true) == nil then return end
    url = url:gsub('ms[-]settings:', '')

    if url == 'taskbar' then
      wxsUI('UI_Settings', 'main.jcfg', '-fixscreen')
    elseif url == 'dateandtime' then
      App:Run('timedate.cpl')
    elseif url == 'display' then
      --wxsUI('UI_Resolution', 'main.jcfg')
      wxsUI('UI_Settings', 'main.jcfg', '-display -fixscreen')
    elseif url == 'personalization' then
      wxsUI('UI_Settings', 'main.jcfg', '-colors -fixscreen')
    elseif url == 'personalization-background' then
      wxsUI('UI_Settings', 'main.jcfg', '-colors -fixscreen')
    elseif url == 'sound' then
      wxsUI('UI_Volume')
    elseif url == 'network' then
      wxs_open('networkconnections')
    elseif url == 'about' then
      wxsUI('UI_SystemInfo')
    else
       -- winapi.show_message('', url)
       wxs_open('controlpanel')
    end
end

function wxs_protocol(url)
  App:Print(url)
  if url == 'ms-availablenetworks:' then
    wxsUI('UI_WIFI')
  else
    ms_settings(url)
  end
end

function handle_showdesktop_switcher()
  local win_ver = os.info('winver')[3]

  App:Var('HandleShowDesktop', WxsHandler.HandleShowDesktop)

  if WxsHandler.HandleShowDesktop == true and win_ver >= 22621 then
    WxsHandler.HandleShowDesktop = false
    App:Var('HandleShowDesktop', WxsHandler.HandleShowDesktop)
  end

end

function regist_folder_shell()
  local sd = os.getenv("SystemDrive")
  if File.Exists(sd .. '\\Windows\\explorer.exe') then return end

  local key = [[HKEY_CLASSES_ROOT\Folder\shell]]
  local val = Reg:Read(key, 'WinXShell_Registered')

  if val ~= nil then return end
  Reg:Write(key, 'WinXShell_Registered', 'done')

  key = [[HKEY_CLASSES_ROOT\Folder\shell\open\command]]
  val = Reg:Read(key, 'DelegateExecute')
  if val ~= nil then Reg:Write(key, 'DelegateExecute_Backup', val) end
  Reg:Delete(key, 'DelegateExecute')
  Reg:Write(key, '', '"' .. App.FullPath .. '" "%1"')

  -- explore,opennewprocess,opennewtab
end

local function regist_protocol(protocol, type)
  local key = 'HKEY_CLASSES_ROOT\\' .. protocol
  local val = Reg:Read(key .. [[\Shell\Open\Command]], 'DelegateExecute')
  App:Print(val)
  if val == nil or val ~= '{C59C9814-F038-4B71-A341-6024882458AF}' then
    Reg:Write(key, '', 'URL:' .. protocol)
    Reg:Write(key, 'URL Protocol', '')
    Reg:Write(key .. [[\Shell\Open\Command]], 'DelegateExecute', '{C59C9814-F038-4B71-A341-6024882458AF}')

    if type == 'App' then
      Reg:Write(key .. [[\Application]], 'AppUserModelId', '')
      Reg:Write(key .. [[\Shell\Open]], 'PackageId', '')
    end
  end
end

function regist_protocols()
  if os.getenv('WIN_VSDEBUG') ~= nil then return end
  local win_ver = os.info('winver')['1.2']
  if tonumber(win_ver) < 10 then return end
  regist_protocol('ms-settings')
  regist_protocol('ms-availablenetworks', 'App')
  App:Run(App.FullPath, '-Embedding')
end

function regist_system_property() -- handle This PC's property menu
    if not os.info('isX') then return end
    --if File.Exists('X:\\Windows\\explorer.exe') then return end

    if WxsHandler.SystemProperty == nil then return end
    if WxsHandler.SystemProperty == '' then return end
    if WxsHandler.SystemProperty == 'auto' then
        -- 'control system' works in x86_x64 with explorer.exe
        if os.ARCH == 'x64' and File.Exists('X:\\Windows\\explorer.exe') and
            File.Exists('X:\\Windows\\SysWOW64\\wow32.dll') then
            return
        elseif os.ARCH == 'x86' and File.Exists('X:\\Windows\\explorer.exe') then
            return
        end
    end

    local key = [[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties]]
    if Reg:Read(key, '') then return end -- already exists
    -- show This PC on the Desktop
    -- Reg:Write([[HKEY_USERS\.DEFAULT\Software\Microsoft\Windows\CurrentVersion\Explorer\HideDesktopIcons\NewStartPanel]], '{20D04FE0-3AEA-1069-A2D8-08002B30309D}', 0, winapi.REG_DWORD)
    -- handle Property menu to UI_SystemInfo
    Reg:Write(key, '', '@shell32.dll,-33555')
    Reg:Write(key, 'Position', 'Bottom')

    if WxsHandler.SystemProperty == 'ui_systemInfo' then
      Reg:Write(key ..'\\command', '', App.FullPath .. ' wxs-ui:systeminfo')
    else
      Reg:Write(key ..'\\command', '', App.FullPath .. ' wxs-open:system')
    end
end

function regist_shortcut_ocf() -- handle shortcut's OpenContainingFolder menu
    if not os.info('isX') then return end
    if File.Exists('X:\\Windows\\explorer.exe') then
      if File.Exists('X:\\Windows\\System32\\ieframe.dll') then return end
    end

    local key = [[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]]
    if Reg:Read(key, '') then return end -- already exists
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], '', 'Open Containing Folder Menu Wrap')
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], '#MUIVerb', '@shell32.dll,-1033')
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], 'Extended', '')
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub]], 'Position', 'Bottom')
    local explorer_opt = ''
    -- if File.Exists('X:\\Windows\\explorer.exe') then explorer_opt = '-explorer' end
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shell\OpenContainingFolderMenu_wxsStub\command]], '', App.FullPath .. ' '.. explorer_opt ..' -ocf \"%1\"')

    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shellex\ContextMenuHandlers\OpenContainingFolderMenu]], '', 'disable-{37ea3a21-7493-4208-a011-7f9ea79ce9f5}')
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shellex\ContextMenuHandlers\OpenContainingFolderMenu_wxsStub]], '', '{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}')
    Reg:Write([[HKEY_CLASSES_ROOT\lnkfile\shellex\PropertySheetHandlers\wxsStub]], '', '{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}')

    Reg:Write([[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}]], '', 'wxsStub')
    local stub_dll = 'wxsStub.dll'
    if os.ARCH ~= 'x64' then stub_dll = 'wxsStub32.dll' end
    Reg:Write([[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], '', App.Path .. '\\' .. stub_dll)
    Reg:Write([[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], 'ThreadingModel', 'Apartment')

    if os.ARCH == 'x64' then
      Reg:Write([[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}]], '', 'wxsStub')
      Reg:Write([[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], '', App.Path .. '\\wxsStub32.dll')
      Reg:Write([[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\{B1FD8E8F-DC08-41BC-AF14-AAC87FE3073B}\InProcServer32]], 'ThreadingModel', 'Apartment')
    end

end

--------------------------------------------------------------------------------

-- return the resource id for startmenu logo
function Startmenu:SetLogoId()
  local map = {
    ["none"] = 0, ["windows"] = 1, ["winpe"] = 2,
    ["custom1"] = 11, ["custom2"] = 12, ["custom3"] = 13,
    ["default"] = 1
  }
  -- use next line for custom (remove "--" and change "none" to what you like)
  -- if true then return map["none"] end
  if os.info('isWinPE') then return map["winpe"] end
  return map["windows"]
end

function Startmenu:Logoff()
  -- return 1 -- for call system process
end

function Startmenu:Reboot()
  -- restart computer directly
  -- System:Reboot()
  wxsUI('UI_Shutdown', 'full.jcfg')
  return 0
  -- return 1 -- for call system dialog
end

function Startmenu:Shutdown()
  -- shutdown computer directly
  -- System::Shutdown()
  wxsUI('UI_Shutdown', 'full.jcfg')
  return 0
  -- return 1 -- for call system dialog
end

function Startmenu:ControlPanel()
  if App:HasOption('-wes') then
    App:Run('control.exe')
    return 0
  end
  return 1
end

function TrayClock:onClick()
  wxsUI('UI_Calendar', 'main.jcfg')
  return 0
end

function TrayClock:onDblClick()
  App:Run('control.exe', 'timedate.cpl')
  return 0
end
