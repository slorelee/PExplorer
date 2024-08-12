
function string.envstr(str)
    return App.Call('envstr', str)
end

function string.resstr(str)
    return App.Call('resstr', str)
end

function math.band(a, b)
    return App.Call('band', a, b)
end

--

local clsOption = {
    cmd = nil,
}
clsOption.__index = clsOption

function clsOption:Print()
    print(self.cmd)
end

function clsOption:HasOption(opt)
    local cmd = self.cmd
    return (string.find(cmd, opt) and true or false)
end

function  clsOption:GetOption(opt)
    local val, val2
    local cmd = self.cmd
    val = string.match(cmd, opt .. '%s(.+)$')
    if val == nil then return nil end

    --  -xyz 'v a l u e' -abc
    val2 = string.match(val, '^[\'](.+)[\']%s')
    if val2 ~= nil then return val2 end
    --  -xyz "v a l u e" -abc
    val2 = string.match(val, '^[\"](.+)[\"]%s')
    if val2 ~= nil then return val2 end

    --  -xyz 'v a l u e'
    val2 = string.match(val, '^[\'](.+)[\']$')
    if val2 ~= nil then return val2 end
    --  -xyz "v a l u e"
    val2 = string.match(val, '^[\"](.+)[\"]$')
    if val2 ~= nil then return val2 end

    --  -xyz abc -abc
    val2 = string.match(val, '([^%s]+)')
    if val2 ~= nil then return val2 end

    return val
end

Option = {}
function Option.New(cmd)
    local o = {}
    setmetatable(o, clsOption)
    if cmd == nil then cmd = "" end
    o.cmd = cmd
    return o
end

--

Alert = App.Alert
alert = Alert

--

App.Version =  App:Info('Version')
App.Ver = App.Version

Lua = {}
Lua.Version =  App:Info('LuaVersion')
Lua.Ver = Lua.Version

App.Path = App:Info('Path')
App.Name = App:Info('Name')
App.FullPath = App:Info('FullPath')

App.CmdLine = App:Info('CmdLine')
App.Option = Option.New(App.CmdLine)

App.ScriptEncoding = 'ANSI'

function App:HasOption(...)
  return self.Option:HasOption(...)
end

function App:GetOption(...)
  return self.Option:GetOption(...)
end

function App:Pause()
  App:Call('Pause')
end

function TEXT(s)
  if App.ScriptEncoding == 'ANSI' then
    return s
  elseif App.ScriptEncoding == 'UTF-8' then
    return App:Call('utf8toansi', s)
  end

  return s
end

--

TID_APP = 10000
TID_AUTO = 20000
TID_USER = 30000

AppTimer = {}
AppTimerId = {}
AppTimerStrId = {}
AppTimer.NextId = TID_AUTO

function App:SetTimer(tid, interval)
  local timerid = 0
  local strid = tostring(tid)
  if type(tid) == "number" then
     if math.type(tid) == "integer" then
        timerid = tid
     end
  end
  if timerid == 0 then
    timerid = AppTimer.NextId
    AppTimer.NextId = AppTimer.NextId + 1
  end
  AppTimerId[timerid] = strid
  AppTimerStrId[strid] = timerid
  App:Debug("[DEBUG] App:SetTimer(" .. timerid .. "['" .. strid .. "'] ," .. interval)
  App.Call('SetTimer', timerid, interval)
end

function App:KillTimer(tid)
  local timerid = AppTimerStrId[tostring(tid)]
  if timerid ~= nil then
    App.Call('KillTimer', timerid)
  end
end


function App:_onTimer(tid)
  local strid = AppTimerId[tid]
  if strid == nil then
    App:Info("[INFO] The AppTimer['" .. tid .."'] function is not defined.")
  elseif type(AppTimer[strid]) ~= "function" then
    App:Error("[ERROR] The AppTimer['" .. tid .."'] is not function.")
    return
  end
  if strid == nil then
    App:onTimer(tid)
    return
  end
  AppTimer[strid](tid)
end

function App:onTimer(tid)
  App:Info("[INFO] App:onTimer(" .. tid ..").")
end

---

function App:_onFirstShellRun()
  -- VERSTR = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]], 'CurrentVersion')
  local win_ver = os.info('winver')['1.2']
  if App.Arg:Has('-wes') then
    if win_ver == '6.2' or win_ver == '6.3' then -- only Windows 8, 8.1
      -- init control panel timer
      App:SetTimer('_InitControlPanel', 200) -- use timer to make main shell running
    end
  end
end

AppTimer['_InitControlPanel'] = function(tid)
  local win_ver = os.info('winver')['1.2']
  App:initControlPanel(win_ver)
  App:KillTimer(tid)
end

function App:_onDaemon()
  regist_shortcut_ocf()
  regist_system_property()
  regist_protocols()
end

function App:_PreShell()
end

function App:_onShell()
  regist_folder_shell()

  App:_onDaemon()
end

function App:WxsProtocol(url, dumy)
  return wxs_protocol(url)
end

function App:initControlPanel(ver)
  --  4161    Control Panel
  local ctrlPanelTitle = string.resstr('#{@shell32.dll,4161}')
  App:Run('control.exe')
  App:Sleep(500)
  if Window.Find('CabinetWClass', ctrlPanelTitle):Close() == 0 then
    -- 32012    All Control Panel Items
    ctrlpanel_title = string.resstr('#{@shell32.dll,32012}')
    Window.Find('CabinetWClass', ctrlPanelTitle):Close()
  end
end

--

Cmd = {}
function Cmd:Echo(s)
    App.Write(1, s)
end

function Cmd:Error(s)
    App.Write(2, s)
end

