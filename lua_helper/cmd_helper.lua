
function parse_option(opt_str)
    local opt = {}
    opt.wait = false
    opt.showcmd = 1
    opt.verb = nil
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
    if string.find(opt_str, '/admin ') then
        opt.verb = 'runas'
    end
    return opt
end


function exec(option, cmd)
    if cmd == nil then
        cmd = option
        option = nil
    end
    local opt = parse_option(option)
    return App:Call('exec', cmd, opt.wait, opt.showcmd, opt.verb)
end


function link(lnk, target, param, icon, index, showcmd)
    local opt = parse_option(showcmd)
    return App:Call('link', lnk, target, param, icon, index, opt.showcmd)
end


--[[
-- test
print(parse_option('/wait /hide').wait)
print(parse_option('/hide').wait)
print(parse_option('/wait /hide').showcmd)
print(parse_option('/wait').showcmd)
print(parse_option('/wait /min').showcmd)
print(parse_option('/wait /max').showcmd)
print(parse_option(nil).showcmd)
--]]
