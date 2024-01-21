-- require 'winapi'

local function reg_getdata(regkey, value)
  local data, data_type = regkey:get_value(value)
  if data_type == winapi.REG_DWORD then
    data = data | 0 -- to integer
  end
  return data, data_type
end

function reg_read(key, values)
  local res = {}
  local data, data_type = nil, 0

  local k, err = winapi.open_reg_key(key, false)
  if not k then return nil end

  if not (type(values) == 'table') then
    data, data_type = reg_getdata(k, values)
    k:close()
    return data, data_type
  end

  for i, v in ipairs(values) do
    data = reg_getdata(k, v)
    res[v] = data
    res[i] = data
  end
  k:close()

  return res
end

function reg_write(key, name, value, data_type)
  local k, err = winapi.open_reg_key(key, true)
  if not k then -- create the key if the key isn't exists
    winapi.create_reg_key(key)
    k,err = winapi.open_reg_key(key, true) -- open again
    if not k then return nil end
  end
  if data_type == nil then data_type = winapi.REG_SZ end
  k:set_value(name, value, data_type)
  k:close()
  return 1
end

function reg_delete(key, name)
  local subkey, k, err
  if name == nil then
    name = key:match("[^\\]+$")
    key = key:match(".+\\")
    subkey = key
  end

  k,err = winapi.open_reg_key(key, true)
  if not k then return nil end
  if subkey ~= nil then
    k:delete(name)
  else
    k:delete_value(name)
  end
  k:close()
  return 0
end

-- Export Reg Class

Reg = {}


REG_NONE = 0
REG_SZ  = 1
REG_EXPAND_SZ = 2
REG_BINARY = 3
REG_DWORD = 4

REG_LINK = 6
REG_MULTI_SZ = 7
REG_QWORD = 11

function Reg:Read(...) return reg_read(...) end
function Reg:Write(...) return reg_write(...) end
function Reg:Delete(...) return reg_delete(...) end

function Reg:GetSubKeys(key)
  local k, err
  k, err = winapi.open_reg_key(key, true)
  if not k then
    return nil
  end
  local keys = k:get_keys()
  k:close()
  if #keys >= 1 then
    table.remove(keys, 1) -- remove self
    return keys
  end
  return nil
end

