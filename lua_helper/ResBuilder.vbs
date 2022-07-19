Const ForReading = 1, ForWriting = 2, ForAppending = 8
Dim fs, f
Set fs = CreateObject("Scripting.FileSystemObject")
set f =  fs.CreateTextFile("APP_HELPERS.lua", True)
f.Close

Set f = fs.OpenTextFile("APP_HELPERS.lua", ForAppending, TristateFalse)

Require("app_helper")
Require("debug_helper")
Require("io_helper")
Require("reg_helper")
Require("os_helper")
Require("cmd_helper")
Require("proc_helper")
Require("win_helper")
Require("shell_helper")
Require("wxs_helper")
Require("loader_helper")
Require("i18n")

f.Close

Function ReadTextFile(file)
    Dim f
    Set f = fs.OpenTextFile(file, ForReading, TristateFalse)
    ReadTextFile = f.ReadAll()
    f.Close
End Function

Function Require(file)
    Dim code
    f.WriteLine("-- " & file & ".lua")
    code = ReadTextFile(file & ".lua")
    f.WriteLine(code)
End Function
