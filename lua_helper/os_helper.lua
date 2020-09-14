-- require 'reg_helper'

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
