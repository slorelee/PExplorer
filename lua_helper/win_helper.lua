-- require 'winapi'

WM_SYSCOMMAND = 0x0112
SC_CLOSE      = 0xf060

function ShowWindow(class, title, show)
  local win
  if title and title:find('*') ~= nil then
    win = winapi.find_window_match(title)
  else
    win = winapi.find_window(class, title)
  end
  if show == nil then show = winapi.SW_SHOW end
  if win ~= nil then win:show(show) end
end

function HideWindow(class, title)
  ShowWindow(class, title, winapi.SW_HIDE)
end

function MinimizeWindow(class, title)
  ShowWindow(class, title, winapi.SW_MINIMIZE)
end


function CloseWindow(class, title)
  local win, hwnd
  if title and title:find('*') ~= nil then
    win = winapi.find_window_match(title)
  else
    win = winapi.find_window(class, title)
  end
  if win ~= nil then 
    hwnd = win:get_handle()
    app:print('CloseWindow(' .. string.format("%s, %s, 0x%x", title, class, hwnd) .. ')')
    win:send_message(WM_SYSCOMMAND, SC_CLOSE, 0)
    return hwnd
  end
  return nil
end
