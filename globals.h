#pragma once

/*
 * Copyright 2003, 2004 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


//
// Explorer clone
//
// globals.h
//
// Martin Fuchs, 23.07.2003
//

// #define USE_DUILIB
// #define HANDLE_PROTOCOL_URL

#include "luaengine/LuaAppEngine.h"
#include "features/reshelper.h"

/// management of file types
struct FileTypeInfo {
    String  _classname;
    String  _displayname;
    bool    _neverShowExt;
};

struct FileTypeManager : public map<String, FileTypeInfo> {
    typedef map<String, FileTypeInfo> super;

    const FileTypeInfo &operator[](String ext);

    static bool is_exe_file(LPCTSTR ext);

    LPCTSTR set_type(struct Entry *entry, bool dont_hide_ext = false);
};


typedef pair<HWND, DWORD> MinimizeStruct;

struct Desktop {
    set<HWND> _windows;
    WindowHandle _hwndForeground;
    list<MinimizeStruct> _minimized;
    void ToggleMinimize();
};
typedef Desktop DesktopRef;

/// structure containing global variables of Explorer
extern struct ExplorerGlobals {
    ExplorerGlobals();

    void    Init(HINSTANCE hInstance, LPTSTR lpCmdLine);

    void    ReadPersistent();
    void    WritePersistent();

    void InitLog();
    void Log(TCHAR * msg);
    void CloseLog();


    void    getModulePath();
    void    getSystemInfo();
    void    getLuaAppEngine();
    void    getUIFolder();
    void    loadConfig();

    HINSTANCE   _hInstance;
    UINT        _cfStrFName;

    String      _cmdline;
    String      _winver;
    WORD        _winvers[4];
    BOOL        _isDebug;
    BOOL        _isShell;
    BOOL        _isWinPE;
    BOOL        _isNT5;
    String      _langID;
    String      _locale;

    int         _exitcode;
#ifndef ROSSHELL
    ATOM        _hframeClass;
    HWND        _hMainWnd;
    bool        _desktop_mode;
    bool        _prescan_nodes;
#endif

    FILE       *_log;
    HANDLE       _log_file;

    DWORD(STDAPICALLTYPE *_SHRestricted)(RESTRICTIONS rest);
    VOID(STDAPICALLTYPE *_SHSettingsChanged)(UINT Ignored, LPCWSTR lpCategory);

    FileTypeManager _ftype_mgr;
    IconCache   _icon_cache;

    HFONT       _hDefaultFont;
    HWND        _hwndDesktopBar;
    HWND        _hwndShellView;
    HWND        _hwndDesktop;
    HWND        _hwndDaemon;

    Desktop     _desktop;

    String      _cfg_dir;
    String      _cfg_path;

    String      _uifolder;
    LuaAppEngine    *_lua;

    TCHAR _varClockTextBuffer[64];
} g_Globals;

extern HINSTANCE g_hInst;

extern void gLuaCall(const char *funcname, LPTSTR p1 = TEXT(""), LPTSTR p2 = TEXT(""));
extern int gLuaClick(LPTSTR item);

