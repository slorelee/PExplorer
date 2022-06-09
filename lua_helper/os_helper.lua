
function os.putenv(var, str)
  App.Call('putenv', var, str)
end

os.setenv = os.putenv

function os.info(key, ...)
  local arr = {}
  if key == nil then return end
  if key:lower() == 'cpu' then
    local cpu_info = Reg:Read([[HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0]]
      ,{'ProcessorNameString', '~MHz'})

    if cpu_info then
      local cpu_f = cpu_info['~MHz']
      cpu_f = cpu_f / 1000
      cpu_info['name'] = cpu_info['ProcessorNameString']
      cpu_info['desc'] = string.format("%s %.2fGHz", cpu_info['name'], cpu_f)
      return cpu_info
    end
  elseif key:lower() == 'winver' then
    local v, v1, v2, v3, v4 =  App.Call('os::info', key, 4)
    arr[1] = v1; arr[2] = v2; arr[3] = v3; arr[4] = v4
    arr['1.2'] = string.format("%d.%d", v1, v2)
    arr['ver'] = v

    --[[

    return setmetatable(arr, {
      __tostring = function()
      return arr.ver
    end
    })

    ]]
    return arr
  elseif key:lower() == 'mem' then
    arr[1], arr[2], arr[3] = App.Call('os::info', 'mem')
    arr['installed'] = arr[1]; arr[1] = tonumber(arr[1])
    arr['total'] = arr[2]; arr[2] = tonumber(arr[2])
    arr['avail'] = arr[3]; arr[3] = tonumber(arr[3])
    arr[4] = arr[2] - arr[3]
    arr['used'] = tostring(arr[4])
    arr["used%"] = math.ceil(arr[4] * 1000 / arr['total']) / 10.0
    arr['total_gb'] = math.ceil(arr[2] / 1024 / 1024 / 1024)
    arr['avail_gb'] = arr[3] / 1024 / 1024 / 1024
    arr['used_gb'] = arr[4] / 1024 / 1024 / 1024
    return arr
  elseif key:lower() == 'iswinpe' then
     return is_winpe()
  end
   return App.Call('os::info', key, ...)
end

function is_winpe()
  local start_opt = reg_read([[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control]],
    'SystemStartOptions')
  return string.find(start_opt, 'minint') ~= nil
end


function os_ver_info()
  return reg_read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]]
    ,{'ProductName', 'CSDVersion'})
end

function cpu_info()
  return reg_read([[HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0]]
    ,{'ProcessorNameString', '~MHz'})
end

function mem_info()
  local mem = {}
  mem[1], mem[2], mem[3] = app:call('os::info', 'mem')
  return mem
end

function localename()
  return app:call('os::info', 'localename')
end

function res_str(file, id)
  local strid = string.format('#{@%s,%s}', file, id)
  return app:call('resstr', strid)
end

function mui_str(file, id)
  LN = LN or localename()
  local mui_file = string.format('%s\\%s.mui', LN, file)
  return res_str(mui_file, id)
end

function win_copyright()
  return app:call('os::info', 'copyright')
end

function call_dll(...)
  return app:call('calldll', ...)
end
