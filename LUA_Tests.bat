--[=[ 2>nul

set WINXSHELL_LUASCRIPT=WinXShellTest.lua
set WINXSHELL_LOGFILE=C:\Windows\Temp\WinXShell.log

set WINXSHELL=WinXShell.exe
set x8664=x64
if not "x%PROCESSOR_ARCHITECTURE%"=="xAMD64" set x8664=x86
if not exist %WINXSHELL% set WINXSHELL=WinXShell_%x8664%.exe
if not exist %WINXSHELL%  set WINXSHELL=x64\Debug\WinXShell.exe

rem one line code
%WINXSHELL% -code "winapi.show_message('title', 'message')"

rem code file
:LOOP
%WINXSHELL% -console -script "%~dpnx0"
type "%WINXSHELL_LOGFILE%"
pause
goto :LOOP

goto :EOF
]=]

--- -- ====================  lua script  ====================

-- Alias


MsgBox = winapi.show_message

-- Downward Compatibility

print(app)

-- Tests

local Tester = {}
local r
local v

-- Functions

-- run()
-- exec()
-- link()

-- New for v5.0.0

--[[
Alert('abc', 123, App:Version())
r = MsgBox('title', 'message', 'yes-no', 'warn')
print('You clicked the [' .. r .. '] button.')
]]

function Tester.BuiltinLib()
-- os lib
local v = os.info('WinVer')
local mem = os.info('Mem')

p(
"======================os.info======================",
"\n",
'v.ver=' .. v.ver, "v[3]=" .. v[3], "v[4]="  .. v[4], "v['1.2']="  .. v["1.2"],
"\n",
os.info('Copyright'),
"\n",
os.info('CPU')['name'], os.info('CPU')['~MHz'],
"\n",
 mem[1], mem[2], mem[3],
"\n",
mem['total'], mem['used'], mem['avail'], mem['used%'],
"\n",
string.format("Total: %d GB, Used: %.2f GB, Avail: %.2f GB, Used: %.1f %%",
    mem['total_gb'], mem['used_gb'], mem['avail_gb'], mem['used%']),
"\n",
os.info('Tickcount'), os.info('LangId'), os.info('Locale'),
os.info('FirmwareType'), os.info('IsUEFIMode'),
os.info('IsWinPE'),
"\n",
math.band(4, 5),
"\n"
)

p(
"======================os.exists======================",
"\nos.exists([[%HOMEDRIVE%\\Windows\\]])",
os.exists([[%HOMEDRIVE%\Windows\]]),
"\nos.exists([[%HOMEDRIVE%\\Windows\\Explorer.exe]])",
os.exists([[%HOMEDRIVE%\Windows\Explorer.exe]]),
"\nos.exists([[C:\\WindowsPE\\]])",
os.exists([[C:\WindowsPE\]]),
"\nos.exists([[C:\\App.exe]])",
os.exists([[C:\App.exe]])
)

p("======================string/math extends======================\n")

local str1 = [[%SystemRoot%\System32\regedit.exe]]
local str2 = '#{@shell32.dll,9316}'

p(
"string.envstr:",
string.envstr([[%ProgramFiles%\WinXShell\WinXShell.exe]]),
"\nstr1:envstr()",
str1:envstr(),
"\nstring.resstr:",
string.resstr('#{@shell32.dll,9315}'),
"\nstr2:resstr()",
str2:resstr(),
"\nmath.band(4, 5):",
math.band(4, 5),
"\n"
)

end


-- App Object

function Tester.App()

p(
"======================App Object======================",
"\nCmdLine:", App.CmdLine,
"\nPath:", App.Path,
"\nName:", App.Name,
"\nFullPath:", App.FullPath,
"\n",
"\nApp:HasOption(\'-console\')", App:HasOption('-console'),
"\nApp:HasOption(\'-debug\')", App:HasOption('-debug'),
"\nApp:GetOption(\'-script\')", App:GetOption('-script'),
"\nApp:GetOption(\'-theme\')", App:GetOption('-theme'),
"\n"
)
App:Sleep(50)
App:Error("[ERROR] Failed to do something.")
App:Warn("CAUTION !!!")
App:InfoLog("notice message")
App:Debug("message in one line")
App:Debug("one", "two")

end

--- Reg Object

function Tester.Reg()

p("\n\n\n")
p(
"======================Reg Object======================"
)
local regkey=[[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]]
v = Reg:Read(regkey, {'CurrentBuild', 'UBR'})

---> v[1] = v['CurrentBuild'] = '19044'
---> v[2] = v['UBR'] = 1645
print(
  str.fmt("v[1] = %s, v[2] = %d", v[1], v[2])
)
print(
  str.fmt("v['CurrentBuild'] = %s, v['UBR'] = %d", v['CurrentBuild'], v['UBR'])
)

regkey= [[HKEY_CURRENT_USER\SOFTWARE\WinXShell\Tests]]

Reg:Write(regkey, '', 'DefaultItemValue')
Reg:Write(regkey, 'Test', 'Test String')
Reg:Write(regkey, 'number', 123, REG_DWORD)

Reg:Write(regkey .. '\\SubKey', '', 'Default')

print(Reg:Read(regkey, ''))
print(Reg:Read(regkey, 'Test'))

-- Reg:Delete([[HKEY_CURRENT_USER\SOFTWARE\WinXShell\Tests]], 'Test')
-- Reg:Delete([[HKEY_CURRENT_USER\SOFTWARE\WinXShell\Tests]])

-- Downward Compatibility

app:call('sleep', 50)

regkey=[[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]]
v = reg_read(regkey, {'CurrentBuild', 'UBR'})
print(v['UBR'])

end




-- Proc/Window lib
function Tester.Proc()

p(
"======================Proc/Window lib======================"
)
App:Run("notepad.exe")

App:Sleep(500)
local proc = Window.Find("无标题 - 记事本")

p(
  proc:GetClassName(),
  proc:GetFileName(),
  proc:GetHandle(),
  str.fmt("Handle = 0x%x", proc:GetHandle())
)
p(proc)
proc:Close()

App:Sleep(500)
obj = Window.Find("无标题 - 记事本")
p(obj, str.fmt("Handle = 0x%x", obj:GetHandle()))
p("Proc End")

end

Tester.BuiltinLib()
Tester.App()
Tester.Reg()
Tester.Proc()


