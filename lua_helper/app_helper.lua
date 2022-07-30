
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
TID_USER = 20000

ICP_TIMER_ID = TID_APP + 1 -- init control panel timer

function App:_onFirstShellRun()
  -- VERSTR = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]], 'CurrentVersion')
  local win_ver = App:Info('winver')
  if App.Arg:Has('-wes') then
    if win_ver == '6.2' or win_ver == '6.3' then -- only Windows 8, 8.1
      App:SetTimer(ICP_TIMER_ID, 200) -- use timer to make main shell running
    end
  end
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
  regist_shortcut_ocf()
  regist_system_property()
  regist_protocols()
end

function App:_onTimer(tid)
  if tid == ICP_TIMER_ID then
    App:initControlPanel(win_ver)
    App:KillTimer(tid)
  end
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

app = App
app.call = App.Call

--