

App.info = App.Info
App.call = App.Call
App.print = App.Print

app = App


function get_option(...)
  return App:GetOption(...)
end

function has_option(...)
  return App:HasOption(...)
end

File.exists = File.Exists
Folder.exists = Folder.Exists
