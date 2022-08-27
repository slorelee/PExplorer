UI = {}
UI.OnClick = {}
UI.OnChanged = {}

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
    App:Debug('clsUITab:OnClick - ' .. tab  .. ' ' .. ctrl)
    self.Layout.selectedid = self.List[tab]
end

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

UIEvent = {}
function UIEvent.OnClick(ctrl)
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
        if param1 then
            func(param1, ctrl)
        else
            func(ctrl)
        end
        return 1
    end
    return nil
end

function UIEvent.OnChanged(ctrl, val)
    local func = UI.OnChanged[ctrl]
    App:Print('UIEvent.OnChanged', ctrl, val)
    if ctrl:sub(1, 8) == '$Switch.' then
        UISwitch.SetText(ctrl, val)
    end
    if type(func) == 'function' then
        func(val, ctrl)
        return 1
    end
    return nil
end
