require 'winapi'

function HideWindow(class, name)
  local win = winapi.find_window(class, name)
  if win ~= nil then win:show(winapi.SW_HIDE) end
end

function WaitForSession(user)
    app:call('WaitForSession', user)
end

function SwitchSession(user)
    app:call('SwitchSession', user)
end


