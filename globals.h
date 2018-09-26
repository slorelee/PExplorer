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

#include "utility/LuaAppEngine.h"
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

    void    init(HINSTANCE hInstance);

    void    read_persistent();
    void    write_persistent();

    void    load_config();
    void    get_modulepath();
    void    get_systeminfo();
    void    get_uifolder();

    HINSTANCE   _hInstance;
    UINT        _cfStrFName;

    String      _cmdline;
    String      _winver;
    BOOL        _isWinPE;
    BOOL        _isNT5;
    String      _langID;
    String      _locale;
#ifndef ROSSHELL
    ATOM        _hframeClass;
    HWND        _hMainWnd;
    bool        _desktop_mode;
    bool        _prescan_nodes;
#endif

    FILE       *_log;

    DWORD(STDAPICALLTYPE *_SHRestricted)(RESTRICTIONS);

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
} g_Globals;

