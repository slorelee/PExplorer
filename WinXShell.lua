
is_pe = (os.info('isWinPE') == 1)  -- Windows Preinstallation Environment
is_wes = App:HasOption('-wes')         -- Windows Embedded Standard
is_win = App:HasOption('-windows')  -- Normal Windows

function App:onLoad()
  -- App:Run('notepad.exe')
  print('WinXShell.exe loading...')
  print('CommandLine:' .. App.CmdLine)
  print('WINPE:'.. tostring(is_pe), 123, 'test', App)
  print('WES:' .. tostring(is_wes))

  -- 'auto', 'ui_systemInfo', 'system', '' or nil
  WxsHandler.SystemProperty = 'auto'
  -- nil or a handler function [  func(lnkfile, realfile)  ]
  -- WxsHandler.OpenContainingFolder = MyOpenContainingFolderHandler
end

function App:onDaemon()
end

function App:PreShell()
end

function App:onFirstShellRun()
end

function App:onShell()
  -- wxsUI('UI_WIFI', 'main.jcfg', '-notrayicon -hidewindow')
  -- wxsUI('UI_Volume', 'main.jcfg', '-notrayicon -hidewindow')
end

function App:onTimer(tid)
end

 -- a handler of OpenContainingFolder
function MyOpenContainingFolderHandler(lnkfile, realfile)
  -- local path = realfile:match('(.+)\\')
  App:Run('cmd', '/k echo ' .. realfile)

  -- totalcmd
  -- App:Run('X:\\Progra~1\\TotalCommander\\TOTALCMD64.exe', '/O /T /A \"' .. realfile .. '\"')
  -- XYplorer
  -- App:Run('X:\\Progra~1\\XYplorer\\XYplorer.exe', '/select=\"' .. realfile .. '\"')
end



-- 如果你想使用这个自定义事件函数,
-- 请将这个函数名变更为ondisplaychanged()。
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


-- 如果你想自定义时钟区域的显示信息,
-- 请将这个函数名变更为update_clock_text()。
-- 自定义显示示例:
--[[
    |  22:00 星期六  |
    |   2019-9-14    |
]]
-- FYI:https://www.lua.org/pil/22.1.html
function update_clock_text_sample()
  local wd_name = {'日', '一', '二', '三', '四', '五', '六'}
  local now_time = os.time()
  local wd_disname =  ' 星期' .. wd_name[os.date('%w', now_time) + 1]
  local clocktext = os.date('%H:%M' .. wd_disname .. '\r\n%Y-%m-%d', now_time)
  app:call('SetVar', 'ClockText', clocktext)
end
