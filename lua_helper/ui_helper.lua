UI = {}
UI.OnClick = {}
UI.OnChanged = {}
UI.OnTimer = {}

function UI.UpdateText(name)
    sui:find(name).text = sui:find(name).text
end

UIGroup = {}

local clsUITab = {
    Name = '$Nav',
    List = {},
    Layout = nil,
    LayoutName = ''
}
clsUITab.__index = clsUITab

function clsUITab:SetList(list)
    local name
    for i = 1, #list do
      name = list[i]
      list[name] = i
    end
    self.List = list
end

function clsUITab:BindLayout(name)
    self.LayoutName = name
    self.Layout = sui:find(name)
end

function clsUITab:Click(tab)
    sui:click(self.Name .. '[' .. tab .. ']')
end

function clsUITab:OnClick(tab, ctrl)
    App:Debug('UITab:OnClick - ' .. tab  .. ' ' .. tostring(ctrl))
    self.Layout.selectedid = self.List[tab]
end

clsUITab.DefOnClick = clsUITab.OnClick

---
UITab = {}
function UITab.New(name)
    local o = {}
    setmetatable(o, clsUITab)
    o.Name = name
    UIGroup[name .. '[*]'] = o
    return o
end

UIPages = {
    tab = nil,
    index = 1
}

function UIPages:Init(page, tab)
    local page = UIPages[page]
    if tab then UIPages.tab = tab end
    if page then page:Init(tab) end
    UIPages.index = UIPages.index + 1
end

UICheckBox = {}
function UICheckBox.Check(elem, val)
    val = val or 1
    if type(elem) == "string" then elem = sui:find(elem) end
    elem.selected = val
end

UIOption = {}
function UIOption.Set(ctrl, val, opts)
    local name
    -- true, false
    if type(val) == "boolean" then
        local i = 2
        if val then i = 1 end
        name = ctrl:gsub('?', opts[i])
    elseif type(val) == "number" then
        name = ctrl:gsub('?', opts[val])
    else
        name = ctrl:gsub('?', opts[tostring(val)])
    end

    sui:find(name).selected = 1
end


UISwitch = {}
function UISwitch.Set(elem, val)
    if type(elem) == "string" then elem = sui:find(elem) end
    elem.selected = val
    if val == 0 then elem.text = "%{Off}" else elem.text = "%{On}" end
end

function UISwitch.SetText(elem, val)
    if type(elem) == "string" then elem = sui:find(elem) end
    if val == 0 then elem.text = "%{Off}" else elem.text = "%{On}" end
end


---
UIDispatcher = {}

UIEvent = {}
UIEvent.Trigger = true

UIEvent.OnTimer = nil
UIEvent.OnClick = nil
UIEvent.OnChanged = nil

local function OverRideFunc(funcs, ...)
    for i = 1, #funcs do
        if type(funcs[i]) == 'function' then
            funcs[i](...)
            return 1
        end
    end
    return nil
end

---
-- TID_APP = 10000
-- TID_AUTO = 20000
-- TID_USER = 30000

UITimer = {}
UITimerId = {}
UITimerStrId = {}
UITimer.NextId = TID_AUTO

function UI:SetTimer(tid, interval)
    local strid = tostring(tid)
    local timerid = UITimerId[strid]

    -- not exist
    if timerid == nil then
        if math.type(tid) == 'integer' then
            timerid = tid
        else
            -- auto timerid
            timerid = UITimer.NextId
            UITimer.NextId = UITimer.NextId + 1 
        end

        UITimerId[strid] =  timerid
        UITimerStrId[timerid] = strid
    end

    App:Debug("[DEBUG] UI:SetTimer(" .. timerid .. "['" .. strid .. "'] ," .. interval .. ")")
    if strid:sub(1, 6) == 'RESET.' then
        UI:KillTimer(strid)
    end
    suilib.call('SetTimer', timerid, interval)
end

function UI:KillTimer(tid)
    local timerid = UITimerId[tostring(tid)]
    if timerid ~= nil then
        suilib.call('KillTimer', timerid)
    end
end

function UIDispatcher.OnTimer(tid)
    local strid = UITimerStrId[tid]
    if strid == nil then
        App:Info("[INFO] The UITimer[" .. tid .."] is not defined.")
    elseif type(UI.OnTimer[strid]) == "function" then
        if strid:sub(1, 6) == 'RESET.' then
            UI:KillTimer(strid)
        end
        UI.OnTimer[strid](tid)
        return
    end

    return OverRideFunc({UIEvent.OnTimer, ontimer}, tid)
end

---

function UIDispatcher.OnClick(ctrl)
    if UIWindow.Inited == 0 then return 0 end
    if UIEvent.Trigger ~= true then return 0 end

    local obj, group, func, param1
    if ctrl:sub(1, 1) == "$" then
        group = ctrl:match("($.+)%[")
        if group then
            App:Debug('UIEvent.OnClick:' .. group)
            param1 = ctrl:match(group .. "%[(.+)%]")
            App:Debug('UIEvent.OnClick:' .. param1)
            obj = UIGroup[group .. '[*]']
            if obj then
                obj:OnClick(param1, ctrl)
                return 1
            end
        end
    end
    func = UI.OnClick[ctrl]
    if func == nil and group ~= nil then func = UI.OnClick[group] end
    if type(func) == 'function' then
        App:Debug("[DEBUG] UIEvent.OnClick[" .. ctrl .."] function()")
        if param1 then
            func(param1, ctrl)
        else
            func(ctrl)
        end
        return 1
    end

    return OverRideFunc({UIEvent.OnClick, onclick}, ctrl)
end


function UIDispatcher.OnChanged(ctrl, val)
    if UIWindow.Inited == 0 then return 0 end
    if UIEvent.Trigger ~= true then return 0 end

    local func = UI.OnChanged[ctrl]
    App:Debug('[DEBUG] UIEvent.OnChanged', ctrl, val)
    if ctrl:sub(1, 8) == '$Switch.' then
        UISwitch.SetText(ctrl, val)
    end
    if type(func) == 'function' then
        func(val, ctrl)
        return 1
    end

    return OverRideFunc({UIEvent.OnChanged, onchanged}, ctrl, val)
end

function UIDispatcher.OnReturn(ctrl)
    return OverRideFunc({UIEvent.OnReturn, onreturn}, ctrl)
end

function UIDispatcher.OnLink(ctrl)
    return OverRideFunc({UIEvent.OnLink, onlink}, ctrl)
end

function UIDispatcher.OnFocus(ctrl)
    return OverRideFunc({UIEvent.OnFocus, onfocus}, ctrl)
end

function UIDispatcher.OnHover(ctrl)
    return OverRideFunc({UIEvent.OnFocus, onhover}, ctrl)
end

function UIDispatcher.OnHoverChanged(ctrl)
    return OverRideFunc({UIEvent.OnFocus, onhoverchanged}, ctrl)
end

function UIDispatcher.OnDeactive(ctrl)
    if UIWindow.Inited == 0 then return 0 end
    return OverRideFunc({UIEvent.OnDeactive, ondeactive}, ctrl)
end

function UIDispatcher.OnMessage(msg, wparam, lparam)
    return OverRideFunc({UIEvent.OnMessage, onmessage}, msg, wparam, lparam)
end

---
UIWindow = {}
UIWindow.Inited = 0

function UIWindow:Init()
    -- for backward compatibility
    if type(init) == 'function' then
        init()
    end
end

function UIWindow:OnLoad()
    -- for backward compatibility
    if type(onload) == 'function' then
        onload()
    end

    UIWindow.Inited = 1
end

function UIWindow:OnShow()
    -- for backward compatibility
    if type(onshow) == 'function' then
        onshow()
    end
end
