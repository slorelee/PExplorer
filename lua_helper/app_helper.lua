
function os.putenv(var, str)
    App.Call('putenv', var, str)
end

os.setenv = os.putenv

function os.info(...)
    return App.Call('os::info', ...)
end

function string.envstr(str)
    return App.Call('envstr', str)
end

function string.resstr(str)
    return App.Call('resstr', str)
end

function math.band(a, b)
    return App.Call('band', a, b)
end

--

Alert = App.Alert
alert = Alert

app = App


