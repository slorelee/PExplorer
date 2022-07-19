-- require 'winapi'

WM_QUIT       =   0x12 
WM_SYSCOMMAND = 0x0112
SC_CLOSE      = 0xf060

function FindWindow(class, title)
  local win = nil
  if title and title:find('*') ~= nil then
    win = winapi.find_window_match(title)
  else
    win = winapi.find_window(class, title)
  end
  return win
end

function ShowWindow(class, title, show)
  local win = FindWindow(class, title)
  if show == nil then show = winapi.SW_SHOW end
  if win ~= nil then win:show(show) end
end

function HideWindow(class, title)
  ShowWindow(class, title, winapi.SW_HIDE)
end

function MinimizeWindow(class, title)
  ShowWindow(class, title, winapi.SW_MINIMIZE)
end

function SendWindow(class, title, msg, wparam, lparam)
  local win, hwnd
  win = FindWindow(class, title)
  if win ~= nil then 
    hwnd = win:get_handle()
    App:Print('SendWindow(' .. string.format("%s, %s, 0x%x", title, class, hwnd) .. ')')
    win:send_message(msg, wparam, lparam)
    return hwnd
  end
  return nil
end

function PostWindow(class, title, msg, wparam, lparam)
  local win, hwnd
  win = FindWindow(class, title)
  if win ~= nil then 
    hwnd = win:get_handle()
    App:Print('PostWindow(' .. string.format("%s, %s, 0x%x", title, class, hwnd) .. ')')
    win:post_message(msg, wparam, lparam)
    return hwnd
  end
  return nil
end

function CloseWindow(class, title)
  return SendWindow(class, title, WM_SYSCOMMAND, SC_CLOSE, 0)
end

function QuitWindow(class, title)
  return PostWindow(class, title, WM_QUIT, 0, 0)
end
