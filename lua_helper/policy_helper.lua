ProductPolicy = {}

function ProductPolicy:Load(key, item)
  App:Call('ProductPolicy::Load', key, item)
end

function ProductPolicy:Get(name)
  App:Call('ProductPolicy::Get', name)
end

function ProductPolicy:Set(name, value)
  App:Call('ProductPolicy::Set', name, value)
end

function ProductPolicy:Save(name, value)
  if name ~= nil then
    App:Call('ProductPolicy::Set', name, value)
  end
  App:Call('ProductPolicy::Save')
end
