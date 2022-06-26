require "winapi"

-- const
-- Window operations for Window.show

SW_SHOW = 0
SW_HIDE = winapi.SW_HIDE
SW_MINIMIZE = winapi.SW_MINIMIZE
SW_MAXIMIZE = winapi.SW_MAXIMIZE
SW_SHOWNORMAL = winapi.SW_SHOWNORMAL

SW_MIN = SW_MINIMIZE
SW_MAX = SW_MAXIMIZE

WM_QUIT       =   0x12 
WM_SYSCOMMAND = 0x0112
SC_CLOSE      = 0xf060

local _clsProc = {
  winObj = nil,
  procObj = nil

--[[
  GetClassName()
  GetFileName()
  GetHandle()
  Show()
  Hide()
  Minimize()
  Maximize()
  SendMessage()
  PostMessage()
  Close()
  Quit()
  Kill()
]]

}


function _clsProc.New(self, win_or_proc, o)
    o = o or {}
    self.__index = self
    setmetatable(o, self)
    self.winObj = nil
    self.procObj = nil
    if not win_or_proc then return o end

    if win_or_proc.__name == "Window" then
      if win_or_proc:get_handle() > 0 then
         self.winObj = win_or_proc
         self.procObj = win_or_proc:get_process()
      end
    end

    if win_or_proc.__name == "Process" then
        self.procObj = win_or_proc
    end

    return o
end

function _clsProc:GetClassName()
  local o = self.winObj
  if o then return o:get_class_name() end
  return ""
end

function _clsProc:GetFileName()
  local o = self.winObj
  if o then return o:get_module_filename() end
  return ""
end

function _clsProc:GetHandle()
  local o = self.winObj
  if o then return o:get_handle() | 0 end
  return 0
end


function _clsProc:Show(flag, active)
  local rc = ""
  local o = self.winObj

  if flag == nil then
    flag = SW_SHOW
    if active == nil then active = true end
  end
  if active == nil then active = false end

  if o then
    rc = o:show(flag)
    if active then o:set_foreground() end
  end
  return rc
end

function _clsProc:Hide()
  return _clsProc:Show(SW_HIDE)
end

function _clsProc:Minimize()
  return _clsProc:Show(SW_MIN)
end

function _clsProc:Maximize()
  return _clsProc:Show(SW_MAX)
end

function _clsProc:SendMessage(msg, wparam, lparam)
  local o = self.winObj
  if o then return o:send_message(msg, wparam, lparam) end
  return ""
end

function _clsProc:PostWindow(msg, wparam, lparam)
  local o = self.winObj
  if o then return o:post_message(msg, wparam, lparam) end
  return ""
end

function _clsProc:Close()
  return _clsProc:SendMessage(WM_SYSCOMMAND, SC_CLOSE, 0)
end

function _clsProc:Quit()
  return _clsProc:PostWindow(WM_QUIT, 0, 0)
end

function _clsProc:Kill()
  local o = self.procObj
  if o then return o:kill() end
  return 0
end


----------------------------------------------------
Proc = {}

-- Proc.Find
-- Proc.Match
-- Proc.Run
-- Proc.Exec

local function findByPid(pid)
end

function Proc.Find(title_or_pid, class)
  local obj = nil
  if type(title_or_pid) == "string" then
    obj = winapi.find_window(class, title_or_pid)
  elseif type(title_or_pid) == "number" then
    obj = winapi.process_from_id(title_or_pid)
  end
  return _clsProc:New(obj)
end

function Proc.Match(pattern)
  local obj = winapi.find_window_match(pattern)
  return _clsProc:New(obj)
end

--[[
local p = Proc.Find("*Untitled")
p = Proc.Find(10912)

print(p:GetClassName())
print(p:GetHandle())
winapi.show_message(p:GetClassName(), "")
p:Show()
p:Close()
p:Kill()

]]