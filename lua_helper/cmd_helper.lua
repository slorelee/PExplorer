--local app = _G.app

function option_parser(opt_str)
    local opt = {}
    opt.wait = false
    opt.showcmd = 1
    if opt_str == nil then return opt end
    opt_str = opt_str .. ' '
    if string.find(opt_str, '/nowait ') then opt.wait = false end
    if string.find(opt_str, '/wait ') then opt.wait = true end
    if string.find(opt_str, '/hide ') then
        opt.hide = 1
        opt.showcmd = 0
    end
    if string.find(opt_str, '/min ') then
        opt.min = 1
        opt.showcmd = 2
    end
    if string.find(opt_str, '/max ') then
        opt.max = 1
        opt.showcmd = 3
    end
    return opt
end


function exec(option, cmd)
    if cmd == nil then
        cmd = option
        option = nil
    end
    local opt = option_parser(option)
    return app:call('exec', cmd, opt.wait, opt.showcmd)
end


function link(lnk, target, param, icon, index, showcmd)
    local opt = option_parser(showcmd)
    return app:call('link', lnk, target, param, icon, index, opt.showcmd)
end


--[[
-- test
print(option_parser('/wait /hide').wait)
print(option_parser('/hide').wait)
print(option_parser('/wait /hide').showcmd)
print(option_parser('/wait').showcmd)
print(option_parser('/wait /min').showcmd)
print(option_parser('/wait /max').showcmd)
print(option_parser(nil).showcmd)
--]]
