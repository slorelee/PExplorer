@echo off && goto :MAIN
----------------------------------------------------------------------------------------
Lua
    Lua is a powerful, efficient, lightweight, embeddable scripting language.

    https://www.lua.org/home.html

Lua CJSON
    The Lua CJSON module provides JSON support for Lua.

    https://github.com/mpx/lua-cjson
    https://kyne.au/~mark/software/lua-cjson.php
----------------------------------------------------------------------------------------

:MAIN
set "MSBUILD=D:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"

set LUA=lua-5.3.6
set LUA_CJSON=lua-cjson-2.1.0


cd /d "%~dp0"
if "x%PROCESSOR_ARCHITECTURE%"=="xAMD64" (
    set "PATH=%~dp0vsbuild\7za\x64;%PATH%"
) else (
    set "PATH=%~dp0vsbuild\7za\x86;%PATH%"
)

if not exist vsbuild\7za vsbuild\unzip.exe vsbuild\bin.zip -d vsbuild\

if exist lua goto :CJSON

7z x %LUA%.tar.gz
7z x %LUA%.tar
del /f /q %LUA%.tar
ren %LUA% lua
copy vsbuild\wmain.c lua\src\
if exist include (del /f /q include\*.*) else mkdir include
if exist lib (del /f /q lib\*.*) else mkdir lib

rem lua.hpp,lua.h,lualib.h,lauxlib.h
move lua\src\lua.h* include\
move lua\src\*lib.h include\

rem lua-cjson
move lua\src\luaconf.h include\

:CJSON
if exist lua-cjson goto :BUILD
7z x %LUA_CJSON%.zip
ren %LUA_CJSON% lua-cjson
copy /y vsbuild\patch\lua-cjson\dtoa.c lua-cjson\dtoa.c

:BUILD
cd vsbuild
"%MSBUILD%" lua.sln /p:Configuration=Release /p:Platform=x64
"%MSBUILD%" lua.sln /p:Configuration=Debug /p:Platform=x64
"%MSBUILD%" lua.sln /p:Configuration=Release /p:Platform=Win32
"%MSBUILD%" lua.sln /p:Configuration=Debug /p:Platform=Win32

copy x64\Debug\lua.lib ..\lib\lua_d_x64.lib
copy x64\Release\lua.lib ..\lib\lua_x64.lib
copy Debug\lua.lib ..\lib\lua_d.lib
copy Release\lua.lib ..\lib\lua.lib

copy x64\Debug\lua-cjson.lib ..\lib\lua-cjson_d_x64.lib
copy x64\Release\lua-cjson.lib ..\lib\lua-cjson_x64.lib
copy Debug\lua-cjson.lib ..\lib\lua-cjson_d.lib
copy Release\lua-cjson.lib ..\lib\lua-cjson.lib
pause

