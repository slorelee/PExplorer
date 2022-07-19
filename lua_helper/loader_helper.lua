function WaitForSession(user)
    App:Call('WaitForSession', user)
end

function SwitchSession(user)
    App:Call('SwitchSession', user)
end

function CloseShellWindow()
  Taskbar:WaitForReady()
  App:Call('closeshell')
  App:Sleep(500)
end

function ShellDaemon(wait, cmd)
  while (1) do
    print('ShellDaemon')
    if wait == true then
      Taskbar:WaitForReady()
      App:Print('WaitForReady')
      App:Sleep(5000)
    end
    wait = false
    if Taskbar:IsReady(5) == false then
      exec('/wait', cmd)
    end
    App:Sleep(5000)
  end
end

function shel(cmd)
  local wait_opt = 'false'
  if os.getenv('USERNAME') ~= 'SYSTEM' then wait_opt = 'true' end
  exec('WinXShell.exe -luacode "ShellDaemon(' .. wait_opt .. ',[[' .. cmd .. ']])"')
end
