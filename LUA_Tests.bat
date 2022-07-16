--[=[ 2>nul

set WINXSHELL_LOGFILE=C:\Windows\Temp\WinXShell.log
:LOOP
x64\Debug\WinXShell.exe -console -script "%~dpnx0"
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

function test_builtinlib()
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
"======================string/math extends======================",
"\nstring.envstr:",
string.envstr([[%ProgramFiles%\WinXShell\WinXShell.exe]]),
"\nstring.resstr:",
string.resstr('#{@shell32.dll,9315}'),
"\nmath.band:",
math.band(4, 5),
"\n"
)

end


-- App Object

function test_app()

p(
"======================App Object======================",
"\nCmdLine:",
App:Info('CmdLine'),
"\nPath:",
App:Info('Path'),
"\nName:",
App:Info('Name'),
"\nFullPath:",
App:Info('FullPath')
)
App:Sleep(50)
App:Error("[ERROR] Failed to do something.")
App:Warn("CAUTION !!!")
App:InfoLog("notice message")
App:Debug("message in one line")
App:Debug("one", "two")

end

--- Reg Object

function test_reg()

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




-- Proc lib
function test_proc()

p(
"======================Proc lib======================"
)
App:Run("notepad.exe")

App:Sleep(500)
local proc = Proc.Find("�ޱ��� - ���±�")

p(
  proc:GetClassName(),
  proc:GetFileName(),
  proc:GetHandle(),
  str.fmt("Handle = 0x%x", proc:GetHandle())
)
p(proc)
proc:Close()

App:Sleep(500)
obj = Proc.Find("�ޱ��� - ���±�")
p(obj, str.fmt("Handle = 0x%x", obj:GetHandle()))
p("Proc End")

end

test_builtinlib()
test_app()
test_reg()
test_proc()

