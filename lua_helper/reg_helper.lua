require 'winapi'

function reg_read(key, values)
  local res = {}
  local data = nil
  k,err = winapi.open_reg_key(key, true)
  if not k then return nil end

  if not (type(values) == 'table') then
    data = k:get_value(values)
    k:close()
    return data
  end

  for i,v in ipairs(values) do
    data = k:get_value(v)
    res[v] = data
  end

  k:close()
  return res
end
