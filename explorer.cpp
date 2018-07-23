/*
 * Copyright 2003, 2004, 2005, 2006 Martin Fuchs
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
// explorer.cpp
//
// Martin Fuchs, 23.07.2003
//
// Credits: Thanks to Leon Finker for his explorer cabinet window example
//


#include "precomp.h"

#include "resource.h"

#include <locale.h>    // for setlocale()

#include <wincon.h>

#ifndef _WIN32_WINNT_WINBLUE
#define _WIN32_WINNT_WINBLUE                0x0603
#endif

#include <VersionHelpers.h>

#ifndef __WINE__
#include <io.h>        // for dup2()
#include <fcntl.h>    // for _O_RDONLY
#endif

//#include "dialogs/settings.h"    // for MdiSdiDlg

#include "services/shellservices.h"
#include "jconfig/jcfg.h"

#include "DUI\UIManager.h"
#include "utility/LuaAppEngine.h"

DynamicLoadLibFct<void(__stdcall *)(BOOL)> g_SHDOCVW_ShellDDEInit(TEXT("SHDOCVW"), 118);

boolean DebugMode = FALSE;

ExplorerGlobals g_Globals;
boolean SelectOpt = FALSE;

void UIProcess(HINSTANCE hInst, String cmdline);

ExplorerGlobals::ExplorerGlobals()
{
    _hInstance = 0;
    _cfStrFName = 0;

    _cmdline = _T("");
#ifndef ROSSHELL
    _hframeClass = 0;
    _hMainWnd = 0;
    _desktop_mode = false;
    _prescan_nodes = false;
#endif

    _log = NULL;
    _SHRestricted = 0;
    _hwndDesktopBar = 0;
    _hwndShellView = 0;
    _hwndDesktop = 0;

    _isWinPE = FALSE;
    _lua = NULL;
}


void ExplorerGlobals::init(HINSTANCE hInstance)
{
    _hInstance = hInstance;

    _SHRestricted = (DWORD(STDAPICALLTYPE *)(RESTRICTIONS)) GetProcAddress(GetModuleHandle(TEXT("SHELL32")), "SHRestricted");

    _icon_cache.init();
}

void ExplorerGlobals::get_modulepath()
{
    TCHAR szFile[MAX_PATH + 1] = { 0 };
    String strPath = TEXT("");
    String strFileName = TEXT("");
    JVAR("JVAR_MODULEFILENAME") = TEXT("");
    DWORD dwRet = GetModuleFileName(NULL, szFile, COUNTOF(szFile));
    if (dwRet != 0) {
        strPath = szFile;
        JVAR("JVAR_MODULEFILENAME") = strPath;
        size_t nPos = strPath.rfind(TEXT('\\'));
        if (nPos != -1) {
            strFileName = strPath.substr(nPos + 1);
            strPath = strPath.substr(0, nPos);
        }
    }
    JVAR("JVAR_MODULEPATH") = strPath;
    JVAR("JVAR_MODULENAME") = strFileName;
}

void ExplorerGlobals::get_uifolder()
{
    TCHAR uifolder[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("WINXSHELL_UIFOLDER"), uifolder, MAX_PATH);
    if (dw == 0) {
        g_Globals._uifolder = TEXT("wxsUI");
    } else {
        g_Globals._uifolder = uifolder;
    }
}

void ExplorerGlobals::load_config()
{
    get_uifolder();

    String jcfgfile = TEXT("WinXShell.jcfg");
#ifndef _DEBUG
    TCHAR buff[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("WINXSHELL_JCFGFILE"), buff, MAX_PATH);
    if (dw == 0) {
        jcfgfile = JVAR("JVAR_MODULEPATH").ToString() + TEXT("\\WinXShell.jcfg");
    } else {
        jcfgfile = JVAR("JVAR_MODULEPATH").ToString() + TEXT("\\") + buff;
    }
#endif
    Load_JCfg(jcfgfile);
}

void ExplorerGlobals::get_systeminfo()
{
    g_Globals._isNT5 = !IsWindowsVistaOrGreater();

    Value v = JCFG2("JS_SYSTEMINFO", "langid");
    String langID = v.ToString();
    g_Globals._langID = langID;
    if (langID == TEXT("0")) {
        g_Globals._langID.printf(TEXT("%d"), GetSystemDefaultLangID());
    }
}

void ExplorerGlobals::read_persistent()
{
    // read configuration file
}

void ExplorerGlobals::write_persistent()
{
    // write configuration file
    //RecursiveCreateDirectory(_cfg_dir);
}

void _log_(LPCTSTR txt)
{
    FmtString msg(TEXT("%s\n"), txt);

    if (g_Globals._log)
        _fputts(msg, g_Globals._log);

#ifdef _DEBUG
    OutputDebugString(msg);
#endif
}

void _logA_(LPCSTR txt)
{
     FmtStringA msg("%s\n", txt);

    if (g_Globals._log)
        fputs(msg.c_str(), g_Globals._log);

#ifdef _DEBUG
    OutputDebugStringA(msg.c_str());
#endif
}


bool FileTypeManager::is_exe_file(LPCTSTR ext)
{
    static const LPCTSTR s_executable_extensions[] = {
        TEXT("COM"),
        TEXT("EXE"),
        TEXT("BAT"),
        TEXT("CMD"),
        TEXT("CMM"),
        TEXT("BTM"),
        TEXT("AWK"),
        0
    };

    TCHAR ext_buffer[_MAX_EXT];
    const LPCTSTR *p;
    LPCTSTR s;
    LPTSTR d;

    for (s = ext + 1, d = ext_buffer; (*d = toupper(*s)); s++)
        ++d;

    for (p = s_executable_extensions; *p; p++)
        if (!lstrcmp(ext_buffer, *p))
            return true;

    return false;
}


const FileTypeInfo &FileTypeManager::operator[](String ext)
{
    ext.toLower();

    iterator found = find(ext);
    if (found != end())
        return found->second;

    FileTypeInfo &ftype = super::operator[](ext);

    ftype._neverShowExt = false;

    HKEY hkey;
    TCHAR value[MAX_PATH], display_name[MAX_PATH];
    LONG valuelen = sizeof(value);

    if (!RegQueryValue(HKEY_CLASSES_ROOT, ext, value, &valuelen)) {
        ftype._classname = value;

        valuelen = sizeof(display_name);
        if (!RegQueryValue(HKEY_CLASSES_ROOT, ftype._classname, display_name, &valuelen))
            ftype._displayname = display_name;

        if (!RegOpenKey(HKEY_CLASSES_ROOT, ftype._classname, &hkey)) {
            if (!RegQueryValueEx(hkey, TEXT("NeverShowExt"), 0, NULL, NULL, NULL))
                ftype._neverShowExt = true;

            RegCloseKey(hkey);
        }
    }

    return ftype;
}

LPCTSTR FileTypeManager::set_type(Entry *entry, bool dont_hide_ext)
{
    LPCTSTR ext = _tcsrchr(entry->_data.cFileName, TEXT('.'));

    if (ext) {
        const FileTypeInfo &type = (*this)[ext];

        if (!type._displayname.empty())
            entry->_type_name = _tcsdup(type._displayname);

        // hide some file extensions
        if (type._neverShowExt && !dont_hide_ext) {
            intptr_t len = ext - entry->_data.cFileName;

            if (entry->_display_name != entry->_data.cFileName)
                free(entry->_display_name);

            entry->_display_name = (LPTSTR) malloc((len + 1) * sizeof(TCHAR));
            lstrcpyn(entry->_display_name, entry->_data.cFileName, (int)len + 1);
        }

        if (is_exe_file(ext))
            entry->_data.dwFileAttributes |= ATTRIBUTE_EXECUTABLE;
    }

    return ext;
}


Icon::Icon()
    : _id(ICID_UNKNOWN),
      _itype(IT_STATIC),
      _hicon(0)
{
}

Icon::Icon(ICON_ID id, UINT nid)    //, int cx, int cy
    :    _id(id),
         _itype(IT_STATIC),
         _hicon(ResIcon(nid))    // ResIconEx(nid, cx, cy)
{
}

Icon::Icon(ICON_ID id, UINT nid, int icon_size)
    :    _id(id),
         _itype(IT_STATIC),
         _hicon(ResIconEx(nid, icon_size, icon_size))
{
}

Icon::Icon(ICON_TYPE itype, int id, HICON hIcon)
    :    _id((ICON_ID)id),
         _itype(itype),
         _hicon(hIcon)
{
}

Icon::Icon(ICON_TYPE itype, int id, int sys_idx)
    :    _id((ICON_ID)id),
         _itype(itype),
         _sys_idx(sys_idx)
{
}

void Icon::draw(HDC hdc, int x, int y, int cx, int cy, COLORREF bk_color, HBRUSH bk_brush) const
{
    if (_itype == IT_SYSCACHE)
        ImageList_DrawEx(g_Globals._icon_cache.get_sys_imagelist(), _sys_idx, hdc, x, y, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
    else
        DrawIconEx(hdc, x, y, _hicon, cx, cy, 0, bk_brush, DI_NORMAL);
}

HBITMAP    Icon::create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd, int icon_size) const
{
    if (_itype == IT_SYSCACHE) {
        HIMAGELIST himl = g_Globals._icon_cache.get_sys_imagelist();

        int cx, cy;
        ImageList_GetIconSize(himl, &cx, &cy);

        HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);
        HDC hdc = CreateCompatibleDC(hdc_wnd);
        HBITMAP hbmp_old = SelectBitmap(hdc, hbmp);
        ImageList_DrawEx(himl, _sys_idx, hdc, 0, 0, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
        SelectBitmap(hdc, hbmp_old);
        DeleteDC(hdc);

        return hbmp;
    } else
        return create_bitmap_from_icon(_hicon, hbrBkgnd, hdc_wnd, icon_size);
}


int Icon::add_to_imagelist(HIMAGELIST himl, HDC hdc_wnd, COLORREF bk_color, HBRUSH bk_brush) const
{
    int ret;

    if (_itype == IT_SYSCACHE) {
        HIMAGELIST himl = g_Globals._icon_cache.get_sys_imagelist();

        int cx, cy;
        ImageList_GetIconSize(himl, &cx, &cy);

        HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);
        HDC hdc = CreateCompatibleDC(hdc_wnd);
        HBITMAP hbmp_old = SelectBitmap(hdc, hbmp);
        ImageList_DrawEx(himl, _sys_idx, hdc, 0, 0, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
        SelectBitmap(hdc, hbmp_old);
        DeleteDC(hdc);

        ret = ImageList_Add(himl, hbmp, 0);

        DeleteObject(hbmp);
    } else
        ret = ImageList_AddAlphaIcon(himl, _hicon, bk_brush, hdc_wnd);

    return ret;
}

HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd, int icon_size)
{
    int cx = icon_size;
    int cy = icon_size;
    HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);

    MemCanvas canvas;
    BitmapSelection sel(canvas, hbmp);

    RECT rect = {0, 0, cx, cy};
    FillRect(canvas, &rect, hbrush_bkgnd);

    DrawIconEx(canvas, 0, 0, hIcon, cx, cy, 0, hbrush_bkgnd, DI_NORMAL);

    return hbmp;
}

HBITMAP create_small_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
{
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);
    HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);

    MemCanvas canvas;
    BitmapSelection sel(canvas, hbmp);

    RECT rect = {0, 0, cx, cy};
    FillRect(canvas, &rect, hbrush_bkgnd);

    DrawIconEx(canvas, 0, 0, hIcon, cx, cy, 0, hbrush_bkgnd, DI_NORMAL);

    return hbmp;
}

int ImageList_AddAlphaIcon(HIMAGELIST himl, HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
{
    HBITMAP hbmp = create_bitmap_from_icon(hIcon, hbrush_bkgnd, hdc_wnd);

    int ret = ImageList_Add(himl, hbmp, 0);

    DeleteObject(hbmp);

    return ret;
}


int IconCache::s_next_id = ICID_DYNAMIC;


void IconCache::init()
{
    int icon_size = STARTMENUROOT_ICON_SIZE;

    _icons[ICID_NONE]        = Icon(IT_STATIC, ICID_NONE, (HICON)0);

    _icons[ICID_FOLDER]        = Icon(ICID_FOLDER,        IDI_FOLDER);
    //_icons[ICID_DOCUMENT]    = Icon(ICID_DOCUMENT,    IDI_DOCUMENT);
    _icons[ICID_EXPLORER]    = Icon(ICID_EXPLORER,    IDI_EXPLORER);
    //_icons[ICID_APP]        = Icon(ICID_APP,        IDI_APPICON);

    _icons[ICID_CONFIG]        = Icon(ICID_CONFIG,        IDI_CONFIG,        icon_size);
    _icons[ICID_DOCUMENTS]    = Icon(ICID_DOCUMENTS,    IDI_DOCUMENTS,    icon_size);
    _icons[ICID_FAVORITES]    = Icon(ICID_FAVORITES,    IDI_FAVORITES,    icon_size);
    _icons[ICID_INFO]        = Icon(ICID_INFO,        IDI_INFO,        icon_size);
    _icons[ICID_APPS]        = Icon(ICID_APPS,        IDI_APPS,        icon_size);
    _icons[ICID_SEARCH]     = Icon(ICID_SEARCH,     IDI_SEARCH,        icon_size);
    _icons[ICID_ACTION]     = Icon(ICID_ACTION,     IDI_ACTION,        icon_size);
    _icons[ICID_SEARCH_DOC] = Icon(ICID_SEARCH_DOC, IDI_SEARCH_DOC,    icon_size);
    _icons[ICID_PRINTER]    = Icon(ICID_PRINTER,    IDI_PRINTER,    icon_size);
    _icons[ICID_NETWORK]    = Icon(ICID_NETWORK,    IDI_NETWORK,    icon_size);
    _icons[ICID_COMPUTER]    = Icon(ICID_COMPUTER,    IDI_COMPUTER,    icon_size);
    _icons[ICID_LOGOFF]     = Icon(ICID_LOGOFF,     IDI_LOGOFF,        icon_size);
    _icons[ICID_SHUTDOWN]    = Icon(ICID_SHUTDOWN,    IDI_SHUTDOWN,    icon_size);
    _icons[ICID_TERMINATE] = Icon(ICID_TERMINATE, IDI_TERMINATE, icon_size);
    _icons[ICID_RESTART]    = Icon(ICID_RESTART,    IDI_RESTART,    icon_size);
    _icons[ICID_BOOKMARK]    = Icon(ICID_BOOKMARK,    IDI_DOT_TRANS,    icon_size);
    _icons[ICID_MINIMIZE]    = Icon(ICID_MINIMIZE,    IDI_MINIMIZE,    icon_size);
    _icons[ICID_CONTROLPAN] = Icon(ICID_CONTROLPAN, IDI_CONTROLPAN,    icon_size);
    _icons[ICID_DESKSETTING] = Icon(ICID_DESKSETTING, IDI_DESKSETTING, icon_size);
    _icons[ICID_NETCONNS]    = Icon(ICID_NETCONNS,    IDI_NETCONNS,    icon_size);
    _icons[ICID_ADMIN]        = Icon(ICID_ADMIN,        IDI_ADMIN,        icon_size);
    _icons[ICID_RECENT]     = Icon(ICID_RECENT,     IDI_RECENT,        icon_size);

    _icons[ICID_TRAY_SND_NONE] = Icon(IT_STATIC, ICID_TRAY_SND_NONE, SmallIcon(IDI_TRAY_SND_NONE));
    _icons[ICID_TRAY_SND_MUTE] = Icon(IT_STATIC, ICID_TRAY_SND_MUTE, SmallIcon(IDI_TRAY_SND_MUTE));
    _icons[ICID_TRAY_SND_SMALL] = Icon(IT_STATIC, ICID_TRAY_SND_SMALL, SmallIcon(IDI_TRAY_SND_SMALL));
    _icons[ICID_TRAY_SND_MIDDLE] = Icon(IT_STATIC, ICID_TRAY_SND_MIDDLE, SmallIcon(IDI_TRAY_SND_MIDDLE));
    _icons[ICID_TRAY_SND_LARGE] = Icon(IT_STATIC, ICID_TRAY_SND_LARGE, SmallIcon(IDI_TRAY_SND_LARGE));
    _icons[ICID_TRAY_NET_WIRED_DIS] = Icon(IT_STATIC, ICID_TRAY_NET_WIRED_DIS, SmallIcon(IDI_TRAY_NET_WIRED_DIS));
    _icons[ICID_TRAY_NET_WIRED_LAN] = Icon(IT_STATIC, ICID_TRAY_NET_WIRED_LAN, SmallIcon(IDI_TRAY_NET_WIRED_LAN));
    _icons[ICID_TRAY_NET_WIRED_INTERNET] = Icon(IT_STATIC, ICID_TRAY_NET_WIRED_INTERNET, SmallIcon(IDI_TRAY_NET_WIRED_INTERNET));
    _icons[ICID_TRAY_NET_WIRELESS_DIS] = Icon(IT_STATIC, ICID_TRAY_NET_WIRELESS_DIS, SmallIcon(IDI_TRAY_NET_WIRELESS_DIS));
    _icons[ICID_TRAY_NET_WIRELESS_NOCONN] = Icon(IT_STATIC, ICID_TRAY_NET_WIRELESS_NOCONN, SmallIcon(IDI_TRAY_NET_WIRELESS_NOCONN));
    _icons[ICID_TRAY_NET_WIRELESS_LAN] = Icon(IT_STATIC, ICID_TRAY_NET_WIRELESS_LAN, SmallIcon(IDI_TRAY_NET_WIRELESS_LAN));
//    _icons[ICID_TRAY_NET_WIRELESS_INTERNET] = Icon(IT_STATIC, ICID_TRAY_NET_WIRELESS_INTERNET, SmallIcon(IDI_TRAY_NET_WIRELESS_INTERNET));
    _icons[ICID_TRAY_NET_SIGNAL_NONE] = Icon(IT_STATIC, ICID_TRAY_NET_SIGNAL_NONE, SmallIcon(IDI_TRAY_NET_SIGNAL_NONE));
    _icons[ICID_TRAY_NET_SIGNAL_QUARTER1] = Icon(IT_STATIC, ICID_TRAY_NET_SIGNAL_QUARTER1, SmallIcon(IDI_TRAY_NET_SIGNAL_QUARTER1));
    _icons[ICID_TRAY_NET_SIGNAL_QUARTER2] = Icon(IT_STATIC, ICID_TRAY_NET_SIGNAL_QUARTER2, SmallIcon(IDI_TRAY_NET_SIGNAL_QUARTER2));
    _icons[ICID_TRAY_NET_SIGNAL_QUARTER3] = Icon(IT_STATIC, ICID_TRAY_NET_SIGNAL_QUARTER3, SmallIcon(IDI_TRAY_NET_SIGNAL_QUARTER3));
    _icons[ICID_TRAY_NET_SIGNAL_QUARTER4] = Icon(IT_STATIC, ICID_TRAY_NET_SIGNAL_QUARTER4, SmallIcon(IDI_TRAY_NET_SIGNAL_QUARTER4));
}


const Icon &IconCache::extract(LPCTSTR path, UINT flags)
{
    // search for matching icon with unchanged flags in the cache
    CacheKey mapkey(path, flags);
    PathCacheMap::iterator found = _pathCache.find(mapkey);

    if (found != _pathCache.end())
        return _icons[found->second];

    // search for matching icon with handle
    CacheKey mapkey_hicon(path, flags | ICF_HICON);
    if (flags != mapkey_hicon.second) {
        found = _pathCache.find(mapkey_hicon);

        if (found != _pathCache.end())
            return _icons[found->second];
    }

    // search for matching icon in the system image list cache
    CacheKey mapkey_syscache(path, flags | ICF_SYSCACHE);
    if (flags != mapkey_syscache.second) {
        found = _pathCache.find(mapkey_syscache);

        if (found != _pathCache.end())
            return _icons[found->second];
    }

    SHFILEINFO sfi;

    int shgfi_flags = 0;

    if (flags & ICF_NOLINKOVERLAY) {
        shgfi_flags = SHGFI_SYSICONINDEX;
        if (flags & ICF_LARGE) {
            shgfi_flags |= SHGFI_LARGEICON;
        } else {
            shgfi_flags |= SHGFI_SMALLICON;
        }
        HIMAGELIST himl = (HIMAGELIST)SHGetFileInfo(path, 0, &sfi, sizeof(sfi), shgfi_flags);
        if (himl) {
            HICON hicon = ImageList_GetIcon(himl, sfi.iIcon, ILD_NORMAL);
            const Icon &icon = add(hicon, IT_CACHED);

            ///@todo limit cache size
            _pathCache[mapkey_hicon] = icon;

            return icon;
        }
    }

    shgfi_flags = 0;

    if (flags & ICF_OPEN)
        shgfi_flags |= SHGFI_OPENICON;

    if ((flags & (ICF_LARGE | ICF_MIDDLE | ICF_OVERLAYS | ICF_HICON)) && !(flags & ICF_SYSCACHE)) {
        shgfi_flags |= SHGFI_ICON;

        if (!(flags & (ICF_LARGE | ICF_MIDDLE)))
            shgfi_flags |= SHGFI_SMALLICON;

        if (flags & ICF_OVERLAYS)
            shgfi_flags |= SHGFI_ADDOVERLAYS;

        // get small/big icons with/without overlays
        if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), shgfi_flags)) {
            const Icon &icon = add(sfi.hIcon, IT_CACHED);

            ///@todo limit cache size
            _pathCache[mapkey_hicon] = icon;

            return icon;
        }
    } else {
        assert(!(flags & ICF_OVERLAYS));

        shgfi_flags |= SHGFI_SYSICONINDEX | SHGFI_SMALLICON;

        // use system image list - the "search program dialog" needs it
        HIMAGELIST himlSys_small = (HIMAGELIST) SHGetFileInfo(path, 0, &sfi, sizeof(sfi), shgfi_flags);

        if (himlSys_small) {
            _himlSys_small = himlSys_small;

            const Icon &icon = add(sfi.iIcon/*, IT_SYSCACHE*/);

            ///@todo limit cache size
            _pathCache[mapkey_syscache] = icon;

            return icon;
        }
    }

    return _icons[ICID_NONE];
}

const Icon &IconCache::extract(LPCTSTR path, int icon_idx, UINT flags)
{
    IdxCacheKey key(path, make_pair(icon_idx, (flags | ICF_HICON) & ~ICF_SYSCACHE));

    key.first.toLower();

    IdxCacheMap::iterator found = _idxCache.find(key);

    if (found != _idxCache.end())
        return _icons[found->second];

    HICON hIcon;

    if ((int)ExtractIconEx(path, icon_idx, NULL, &hIcon, 1) > 0) {
        const Icon &icon = add(hIcon, IT_CACHED);

        _idxCache[key] = icon;

        return icon;
    } else {

        ///@todo retreive "http://.../favicon.ico" format icons

        return _icons[ICID_NONE];
    }
}

const Icon &IconCache::extract(IExtractIcon *pExtract, LPCTSTR path, int icon_idx, UINT flags)
{
    HICON hIconLarge = 0;
    HICON hIcon;

    int icon_size = ICON_SIZE_FROM_ICF(flags);
    HRESULT hr = pExtract->Extract(path, icon_idx, &hIconLarge, &hIcon, MAKELONG(GetSystemMetrics(SM_CXICON), icon_size));

    if (hr == NOERROR) {    //@@ oder SUCCEEDED(hr) ?
        if (icon_size > ICON_SIZE_SMALL) {    //@@ OK?
            if (hIcon)
                DestroyIcon(hIcon);

            hIcon = hIconLarge;
        } else {
            if (hIconLarge)
                DestroyIcon(hIconLarge);
        }

        if (hIcon)
            return add(hIcon);    //@@ When do we want not to free this icons?
    }

    return _icons[ICID_NONE];
}

const Icon &IconCache::extract(LPCITEMIDLIST pidl, UINT flags)
{
    // search for matching icon with unchanged flags in the cache
    PidlCacheKey mapkey(pidl, flags);
    PidlCacheMap::iterator found = _pidlcache.find(mapkey);

    if (found != _pidlcache.end())
        return _icons[found->second];

    // search for matching icon with handle
    PidlCacheKey mapkey_hicon(pidl, flags | ICF_HICON);
    if (flags != mapkey_hicon.second) {
        found = _pidlcache.find(mapkey_hicon);

        if (found != _pidlcache.end())
            return _icons[found->second];
    }

    // search for matching icon in the system image list cache
    PidlCacheKey mapkey_syscache(pidl, flags | ICF_SYSCACHE);
    if (flags != mapkey_syscache.second) {
        found = _pidlcache.find(mapkey_syscache);

        if (found != _pidlcache.end())
            return _icons[found->second];
    }

    SHFILEINFO sfi;

    int shgfi_flags = SHGFI_PIDL;

    if (!(flags & (ICF_LARGE | ICF_MIDDLE)))
        shgfi_flags |= SHGFI_SMALLICON;

    if (flags & ICF_OPEN)
        shgfi_flags |= SHGFI_OPENICON;

    if (flags & ICF_SYSCACHE) {
        assert(!(flags & ICF_OVERLAYS));

        HIMAGELIST himlSys = (HIMAGELIST) SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | shgfi_flags);
        if (himlSys) {
            const Icon &icon = add(sfi.iIcon/*, IT_SYSCACHE*/);

            ///@todo limit cache size
            _pidlcache[mapkey_syscache] = icon;

            return icon;
        }
    } else {
        if (flags & ICF_OVERLAYS)
            shgfi_flags |= SHGFI_ADDOVERLAYS;

        if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | shgfi_flags)) {
            const Icon &icon = add(sfi.hIcon, IT_CACHED);

            ///@todo limit cache size
            _pidlcache[mapkey_hicon] = icon;

            return icon;
        }
    }

    return _icons[ICID_NONE];
}


const Icon &IconCache::add(HICON hIcon, ICON_TYPE type)
{
    int id = ++s_next_id;

    return _icons[id] = Icon(type, id, hIcon);
}

const Icon    &IconCache::add(int sys_idx/*, ICON_TYPE type=IT_SYSCACHE*/)
{
    int id = ++s_next_id;

    return _icons[id] = SysCacheIcon(id, sys_idx);
}

const Icon &IconCache::get_icon(int id)
{
    return _icons[id];
}

IconCache::~IconCache()
{
    /* We don't need to free cached resources - they are automatically freed at process termination
        for (int index = s_next_id; index >= 0; index--) {
            IconMap::iterator found = _icons.find(index);

            if (found != _icons.end()) {
                Icon& icon = found->second;

                if ((icon.get_icontype() == IT_DYNAMIC) ||
                    (icon.get_icontype() == IT_CACHED))
                {
                    DestroyIcon(icon.get_hicon());
                    _icons.erase(found);
                }
            }
        }
    */
}

void IconCache::free_icon(int icon_id)
{
    IconMap::iterator found = _icons.find(icon_id);

    if (found != _icons.end()) {
        Icon &icon = found->second;

        if (icon.destroy())
            _icons.erase(found);
    }
}


ResString::ResString(UINT nid)
{
    TCHAR buffer[BUFFER_LEN];

    int len = LoadString(g_Globals._hInstance, nid, buffer, sizeof(buffer) / sizeof(TCHAR));

    super::assign(buffer, len);
}


ResIcon::ResIcon(UINT nid)
{
    _hicon = LoadIcon(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}

SmallIcon::SmallIcon(UINT nid)
{
    _hicon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}

SizeIcon::SizeIcon(UINT nid, int size)
{
    _hicon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, size, size, LR_SHARED);
}

ResIconEx::ResIconEx(UINT nid, int w, int h)
{
    _hicon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, w, h, LR_SHARED);
}


void SetWindowIcon(HWND hwnd, UINT nid)
{
    HICON hIcon = ResIcon(nid);
    (void)Window_SetIcon(hwnd, ICON_BIG, hIcon);

    HICON hIconSmall = SmallIcon(nid);
    (void)Window_SetIcon(hwnd, ICON_SMALL, hIconSmall);
}


ResBitmap::ResBitmap(UINT nid)
{
    _hBmp = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}


#ifndef ROSSHELL

bool ExplorerCmd::ParseCmdLine(LPCTSTR lpCmdLine)
{
    bool ok = true;

    LPCTSTR b = lpCmdLine;
    LPCTSTR p = b;

    while (*b) {
        // remove leading space
        while (_istspace((unsigned)*b))
            ++b;

        p = b;

        bool quote = false;

        // options are separated by ','
        for (; *p; ++p) {
            if (*p == '"')    // Quote characters may appear at any position in the command line.
                quote = !quote;
            else if (*p == ',' && !quote)
                break;
        }

        if (p > b) {
            intptr_t l = p - b;

            // remove trailing space
            while (l > 0 && _istspace((unsigned)b[l - 1]))
                --l;

            if (!EvaluateOption(String(b, (int)l)))
                ok = false;

            if (*p)
                ++p;

            b = p;
        }
    }

    return ok;
}

bool ExplorerCmd::EvaluateOption(LPCTSTR option)
{
    String opt_str;

    // Remove quote characters, as they are evaluated at this point.
    for (; *option; ++option)
        if (*option != '"')
            opt_str += *option;

    option = opt_str;

    if (option[0] == '/') {
        ++option;

        // option /e for windows in explorer mode
        if (!_tcsicmp(option, TEXT("e")))
            _flags |= OWM_EXPLORE;
        // option /root for rooted explorer windows
        else if (!_tcsicmp(option, TEXT("root")))
            _flags |= OWM_ROOTED;
        // non-standard options: /mdi, /sdi
        else if (!_tcsicmp(option, TEXT("mdi")))
            _mdi = true;
        else if (!_tcsicmp(option, TEXT("sdi")))
            _mdi = false;
        else if (!_tcsicmp(option, TEXT("n"))) {
            // Do nothing
        } else if (!_tcsicmp(option, TEXT("select"))) {
            SelectOpt = TRUE;
        } else
            return false;

    } else {
        if (!_path.empty())
            return false;

        if ((SelectOpt == TRUE) && (PathFileExists(option))) {
            TCHAR szDir[MAX_PATH];

            _tsplitpath(option, szPath, szDir, NULL, NULL);
            _tcscat(szPath, szDir);
            PathRemoveBackslash(szPath);
            _path = szPath;
            SelectOpt = FALSE;
        } else
            _path = opt_str;
    }

    return true;
}

bool ExplorerCmd::IsValidPath() const
{
    if (!_path.empty()) {
        DWORD attribs = GetFileAttributes(_path);

        if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY))
            return true;    // file system path
        else if (*_path == ':' && _path.at(1) == ':')
            return true;    // text encoded IDL
    }

    return false;
}

#else

void explorer_show_frame(int cmdShow, LPTSTR lpCmdLine)
{
    if (!lpCmdLine)
        lpCmdLine = TEXT("explorer.exe");

    launch_file(GetDesktopWindow(), lpCmdLine, cmdShow);
}

#endif


PopupMenu::PopupMenu(UINT nid)
{
    HMENU hMenu = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(nid));
    _hmenu = GetSubMenu(hMenu, 0);
    RemoveMenu(hMenu, 0, MF_BYPOSITION);
    DestroyMenu(hMenu);
}


/// "About Explorer" Dialog
struct ExplorerAboutDlg : public
    CtlColorParent <
    OwnerDrawParent<Dialog>
    > {
    typedef CtlColorParent <
    OwnerDrawParent<Dialog>
    > super;

    ExplorerAboutDlg(HWND hwnd)
        :    super(hwnd)
    {
        SetWindowIcon(hwnd, IDI_WINXSHELL);

        new FlatButton(hwnd, IDOK);

        _hfont = CreateFont(20, 0, 0, 0, FW_BOLD, TRUE, 0, 0, 0, 0, 0, 0, 0, TEXT("Sans Serif"));
        new ColorStatic(hwnd, IDC_PE_EXPLORER, RGB(32, 32, 128), 0, _hfont);

        new HyperlinkCtrl(hwnd, IDC_WWW);

        FmtString ver_txt(ResString(IDS_EXPLORER_VERSION_STR), (LPCTSTR)ResString(IDS_VERSION_STR));
        SetWindowText(GetDlgItem(hwnd, IDC_VERSION_TXT), ver_txt);

        HWND hwnd_winver = GetDlgItem(hwnd, IDC_WIN_VERSION);
        SetWindowText(hwnd_winver, get_windows_version_str());
        SetWindowFont(hwnd_winver, GetStockFont(DEFAULT_GUI_FONT), FALSE);

        CenterWindow(hwnd);
    }

    ~ExplorerAboutDlg()
    {
        DeleteObject(_hfont);
    }

    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
    {
        switch (nmsg) {
        case WM_PAINT:
            Paint();
            break;

        default:
            return super::WndProc(nmsg, wparam, lparam);
        }

        return 0;
    }

    void Paint()
    {
        PaintCanvas canvas(_hwnd);

        HICON hicon = (HICON) LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(IDI_WINXSHELL_BIG), IMAGE_ICON, 0, 0, LR_SHARED);

        DrawIconEx(canvas, 20, 10, hicon, 0, 0, 0, 0, DI_NORMAL);
    }

protected:
    HFONT    _hfont;
};

void explorer_about(HWND hwndParent)
{
    Dialog::DoModal(IDD_ABOUT_EXPLORER, WINDOW_CREATOR(ExplorerAboutDlg), hwndParent);
}


static void InitInstance(HINSTANCE hInstance)
{
    CONTEXT("InitInstance");

    setlocale(LC_COLLATE, "");    // set collating rules to local settings for compareName

#ifndef ROSSHELL
    // register frame window class
    g_Globals._hframeClass = IconWindowClass(CLASSNAME_FRAME, IDI_EXPLORER);

    // register child window class
    WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC | CS_DBLCLKS).Register();

    // register tree window class
    WindowClass(CLASSNAME_WINEFILETREE, CS_CLASSDC | CS_DBLCLKS).Register();
#endif

    g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}


int explorer_main(HINSTANCE hInstance, LPTSTR lpCmdLine, int cmdShow)
{
    CONTEXT("explorer_main");
    int rc = 0;
    // initialize Common Controls library
    CommonControlInit usingCmnCtrl;

    try {
        InitInstance(hInstance);
    } catch (COMException &e) {
        HandleException(e, GetDesktopWindow());
        return -1;
    }

#ifndef ROSSHELL
    if (cmdShow != SW_HIDE) {
        /*    // don't maximize if being called from the ROS desktop
                if (cmdShow == SW_SHOWNORMAL)
                        ///@todo read window placement from registry
                    cmdShow = SW_MAXIMIZE;
        */

        rc = explorer_open_frame(cmdShow, lpCmdLine, EXPLORER_OPEN_NORMAL);
    }
#endif
    if (g_Globals._desktop_mode || rc == 1) {
        Window::MessageLoop();
    }
    return 1;
}

static bool SetShellReadyEvent(LPCTSTR evtName)
{
    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, evtName);
    if (!hEvent)
        return false;

    SetEvent(hEvent);
    CloseHandle(hEvent);

    return true;
}

static void CloseShellProcess()
{
    HWND shellWindow = GetShellWindow();

    if (shellWindow) {
        DWORD pid;

        // terminate shell process for NT like systems
        GetWindowThreadProcessId(shellWindow, &pid);
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

        // On Win 9x it's sufficient to destroy the shell window.
        DestroyWindow(shellWindow);

        if (TerminateProcess(hProcess, 0))
            WaitForSingleObject(hProcess, 10000); //INFINITE

        CloseHandle(hProcess);
    }
}

static void ChangeUserProfileEnv()
{
    //HKLM\Software\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-18\ProfileImagePath
    TCHAR userprofile[MAX_PATH + 1] = { 0 };
    if (g_Globals._isWinPE) {
        GetEnvironmentVariable(TEXT("USERPROFILE"), userprofile, MAX_PATH);
        if (_tcsicmp(userprofile, TEXT("X:\\windows\\system32\\config\\systemprofile")) == 0) {
            _tcscpy(userprofile, TEXT("X:\\Users\\Default"));
            SetEnvironmentVariable(TEXT("USERPROFILE"), userprofile);
        }
    }
}

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* for MessageHook */
typedef BOOL(*pSetHook)(DWORD, int);
typedef BOOL(*pRemoveHook)(void);

pSetHook SetHook = NULL;
pRemoveHook RemoveHook = NULL;
UINT *pUWM_HOOKMESSAGE = 0;

}

extern void InstallHook(HWND hwnd, int reHook);
void InitHook(HWND hwnd)
{
#ifdef _WIN64
    TCHAR DllPath[] = TEXT("wxsStub.dll");
#else
    TCHAR DllPath[] = TEXT("wxsStub32.dll");
#endif
    HINSTANCE Hook_Dll = LoadLibrary(DllPath);
    if (Hook_Dll) {
        SetHook = (pSetHook)GetProcAddress(Hook_Dll, "SetHook");
        RemoveHook = (pRemoveHook)GetProcAddress(Hook_Dll, "RemoveHook");
        pUWM_HOOKMESSAGE = (UINT *)GetProcAddress(Hook_Dll, "UWM_HOOKMESSAGE");

        InstallHook(hwnd, 0);
    } else {
        MessageBox(NULL, TEXT("LoadLibrary Error"), TEXT("Error"), MB_ICONERROR);
    }
}

void InstallHook(HWND hwnd, int reHook)
{
    BOOL rc = FALSE;
    HWND hObjWnd = NULL;
    DWORD dwObjThreadId = 0;

    if (!SetHook) return;
    //Progman Shell_TrayWnd TrayClockWClass
    hObjWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (!hObjWnd) return;
    hObjWnd = FindWindowEx(hObjWnd, 0, TEXT("TrayNotifyWnd"), NULL);
    if (!hObjWnd) return;
    hObjWnd = FindWindowEx(hObjWnd, 0, TEXT("TrayClockWClass"), NULL);
    if (!hObjWnd) return;
    dwObjThreadId = GetWindowThreadProcessId(hObjWnd, NULL);

    if (dwObjThreadId != 0) {
        rc = SetHook(dwObjThreadId, reHook);
    }

    if (rc) {
        /*if (dwObjThreadId)
        MessageBox(NULL, "Thread Hook", "Success", MB_OK);
        else
        MessageBox(NULL, "System Hook", "Success", MB_OK);*/
        PostMessage(hObjWnd, *pUWM_HOOKMESSAGE, (WPARAM)hwnd, (LPARAM)hObjWnd);
    }
}

struct WinXShell_DaemonWindow : public Window {
    typedef Window super;
    WinXShell_DaemonWindow(HWND hwnd);
    ~WinXShell_DaemonWindow();
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
    static HWND Create();
protected:
    const UINT WM_TASKBARCREATED;
};


WinXShell_DaemonWindow::WinXShell_DaemonWindow(HWND hwnd)
    : super(hwnd), WM_TASKBARCREATED(RegisterWindowMessage(WINMSG_TASKBARCREATED))
{
}

WinXShell_DaemonWindow::~WinXShell_DaemonWindow()
{
    RemoveHook();
}


HWND WinXShell_DaemonWindow::Create()
{
    static WindowClass wcDaemonWindow(TEXT("WINXSHELL_DAEMONWINDOW"));
    HWND hwnd = Window::Create(WINDOW_CREATOR(WinXShell_DaemonWindow),
        WS_EX_NOACTIVATE, wcDaemonWindow, TEXT("WINXSHELL_DAEMONWINDOW"), WS_POPUP,
        0, 0, 0, 0, 0);
    return hwnd;
}

#define HM_CLOCKAREA_CLICKED 1
#define CLOCKAREA_CLICK_TIMER 1001

static void ClockArea_OnClick(HWND hwnd, int isDbClick)
{
    if (hwnd) KillTimer(hwnd, CLOCKAREA_CLICK_TIMER);
    if (isDbClick) {
        CommandHook(hwnd, TEXT("clockarea_dbclick"), TEXT("JS_DAEMON"));
    } else {
        CommandHook(hwnd, TEXT("clockarea_click"), TEXT("JS_DAEMON"));
    }
}

LRESULT WinXShell_DaemonWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    static int isDbClick = 0;
    if (nmsg == WM_TASKBARCREATED) {
        InstallHook(_hwnd, 1);
    } else if (pUWM_HOOKMESSAGE && nmsg == *pUWM_HOOKMESSAGE) {
#ifdef _DEBUG
        PrintMessage(0, wparam, 0, 0);
#endif
        if (wparam == HM_CLOCKAREA_CLICKED) {
            //MessageBox(NULL, TEXT("HM_CLOCKAREA_CLICKED"), TEXT(""), 0);
            SetTimer(_hwnd, CLOCKAREA_CLICK_TIMER, 500, NULL);
            isDbClick++;
        }
        return S_OK;
    } else if (nmsg == WM_TIMER) {
        if (wparam == CLOCKAREA_CLICK_TIMER) {
            ClockArea_OnClick(_hwnd, (isDbClick>1) ? 1 : 0);
            isDbClick = 0;
            return S_OK;
        }
    }
    return super::WndProc(nmsg, wparam, lparam);
}

class CReg {
public:
    HKEY m_hkey;
    CReg::CReg(HKEY hKey, LPCTSTR lpSubKey) {
        m_Res = 0; m_hkey = NULL;
        m_Res = RegOpenKey(hKey, lpSubKey, &m_hkey);
    }
    CReg::~CReg() { if (m_hkey) RegCloseKey(m_hkey); }
    LSTATUS CReg::Write(TCHAR *value, TCHAR *data) {
        String buff = data;
        if (m_Res) return m_Res;
        return RegSetValueEx(m_hkey, value, 0, REG_SZ, (LPBYTE)buff.c_str(), buff.length() * sizeof(TCHAR));
    }
    LSTATUS CReg::Write(TCHAR *value, DWORD data) {
        if (m_Res) return m_Res;
        return RegSetValueEx(m_hkey, value, 0, REG_DWORD, (LPBYTE)(&data), sizeof(DWORD));
    }
private:
    LONG m_Res;
};

/*
;override system properties
[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties]
"Position"="Bottom"
@="@shell32.dll,-33555"
;@="&Property" I don't found out the resource with shortcut for every language now, use 4177 instead.

[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties\command]
@="WinXShell.exe -ui -jcfg UI_SystemInfo\\main.jcfg"
*/
static void update_property_handler()
{
    if (JCFG2_DEF("JS_DAEMON", "update_properties_name", true).ToBool() == FALSE) {
        return;
    }
    int mid = JCFG2_DEF("JS_DAEMON", "properties_menu", 220).ToInt();
    CReg reg_prop(HKEY_CLASSES_ROOT, TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shell\\properties"));
    if (!reg_prop.m_hkey) return;
    TCHAR namebuffer[MAX_PATH];
    HINSTANCE res = LoadLibrary(TEXT("shell32.dll"));
    if (!res) return;
    HMENU menu = LoadMenu(res, MAKEINTRESOURCE(mid));
    if (!menu) return;
    if (!GetMenuString(menu, 0, namebuffer, MAX_PATH, MF_BYCOMMAND)) {
        FreeLibrary(res);
        return;
    }
    FreeLibrary(res);
    reg_prop.Write(NULL, namebuffer);

    /*reg_prop.Write(TEXT("Position"), TEXT("Bottom"));

    CReg reg_prop_cmd(HKEY_CLASSES_ROOT, TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shell\\Property\\command"));
    reg_prop_cmd.Write(NULL, TEXT("WinXShell.exe -ui -jcfg UI_SystemInfo\\main.jcfg"));

    CReg reg_no_default_prop(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
    reg_no_default_prop.Write(TEXT("NoPropertiesMyComputer"), 1);*/
}

static void ocf(const TCHAR *szPath)
{
    LPITEMIDLIST  pidl;
    LPCITEMIDLIST cpidl_dir;
    LPCITEMIDLIST cpidl_file;
    LPSHELLFOLDER pDesktopFolder;
    ULONG         chEaten;
    ULONG         dwAttributes;
    HRESULT       hr;
    TCHAR          szDirPath[MAX_PATH];

    // 
    // Get a pointer to the Desktop's IShellFolder interface.
    // 
    if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder))) {
        StrCpy(szDirPath, szPath);
        PathRemoveFileSpec(szDirPath);

        hr = pDesktopFolder->ParseDisplayName(NULL, 0, szDirPath, &chEaten, &pidl, &dwAttributes);
        if (FAILED(hr)) {
            pDesktopFolder->Release();
            return;
            // Handle error.
        }
        cpidl_dir = pidl;

        // 
        // Convert the path to an ITEMIDLIST.
        // 
        hr = pDesktopFolder->ParseDisplayName(NULL, 0, (LPWSTR)szPath, &chEaten, &pidl, &dwAttributes);
        if (FAILED(hr)) {
            pDesktopFolder->Release();
            return;
            // Handle error.
        }
        cpidl_file = pidl;
        HRESULT RE = CoInitialize(NULL);
        int re = SHOpenFolderAndSelectItems(cpidl_dir, 1, &cpidl_file, NULL);

        //
        // pidl now contains a pointer to an ITEMIDLIST.
        // This ITEMIDLIST needs to be freed using the IMalloc allocator
        // returned from SHGetMalloc().
        //
        //release the desktop folder object
        pDesktopFolder->Release();
    }
}

static void OpenContainingFolder(LPTSTR pszCmdline)
{
    String cmdline = pszCmdline;
    String lnkfile;
    size_t pos = cmdline.find(_T("-ocf"));
    lnkfile = cmdline.substr(pos + 5);
    if (lnkfile[0U] == TEXT('\"')) lnkfile = lnkfile.substr(1, lnkfile.length() - 2);
    TCHAR path[MAX_PATH];
    GetShortcutPath(lnkfile.c_str(), path, MAX_PATH);

    String strPath = path;
    if (g_Globals._lua) {
        if (g_Globals._lua->hasfunc("do_ocf")) {
            g_Globals._lua->call("do_ocf", lnkfile, strPath);
            return;
        }
    }

    //if (!PathIsDirectory(path)) {
        //size_t nPos = strPath.rfind(TEXT('\\'));
        //if (nPos == String::npos) return;
        //strPath = strPath.substr(0, nPos);
    //}
    if (cmdline.find(_T("-explorer")) != String::npos) {
        ocf(strPath.c_str());
        return;
    }
    strPath = TEXT("/select,") + strPath;
    explorer_show_frame(SW_SHOWNORMAL, (LPTSTR)(strPath.c_str()));
    Window::MessageLoop();
    return;

}

extern string_t GetParameter(string_t cmdline, string_t key, BOOL hasValue = TRUE);

static DWORD GetColorRef(String color)
{
    DWORD clrColor = 0;
    LPCTSTR pstrValue = color.c_str();
    LPTSTR pstr = NULL;
    if (color != TEXT("") && color.length() >= 8) {
        pstrValue = ::CharNext(pstrValue);
        pstrValue = ::CharNext(pstrValue);
        TCHAR buff[10] = { 0 };
        buff[0] = pstrValue[4];buff[1] = pstrValue[5];
        buff[2] = pstrValue[2];buff[3] = pstrValue[3];
        buff[4] = pstrValue[0];buff[5] = pstrValue[1];
        clrColor = _tcstoul(buff, &pstr, 16);
        return clrColor;
    }
    return 0;
}

static void UpdateSysColor(LPTSTR pszCmdline)
{
    String cmdline = pszCmdline;
    cmdline += TEXT(" ");
    String hl = GetParameter(cmdline, TEXT("color_highlight"), TRUE);
    INT elements[2] = { COLOR_HIGHLIGHT, COLOR_HOTLIGHT };
    if (hl != TEXT("")) {
        DWORD clrColor = GetColorRef(hl);
        SetSysColors(1, elements, &clrColor);
    }

    hl = GetParameter(cmdline, TEXT("color_selection"), TRUE);
    if (hl != TEXT("")) {
        DWORD clrColor = GetColorRef(hl);
        SetSysColors(1, elements + 1, &clrColor);
    }
}


EXTERN_C {
    extern int ShellHasBeenRun();
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
    CONTEXT("WinMain()");

    BOOL any_desktop_running = IsAnyDesktopRunning();

    BOOL startup_desktop = FALSE;

    // strip extended options from the front of the command line
    String ext_options;

    LPTSTR lpCmdLineOrg = lpCmdLine;
    while (*lpCmdLine == '-') {
        while (*lpCmdLine && !_istspace((unsigned)*lpCmdLine))
            ext_options += *lpCmdLine++;

        while (_istspace((unsigned)*lpCmdLine))
            ++lpCmdLine;
    }

    if (_tcsstr(ext_options, TEXT("-winpe"))) {
        g_Globals._isWinPE = TRUE;
        CloseShellProcess();
        ChangeUserProfileEnv();
        any_desktop_running = FALSE;
    } else if (_tcsstr(ext_options, TEXT("-wes"))) {
        CloseShellProcess();
        any_desktop_running = FALSE;
    }

    // command line option "-install" to replace previous shell application with WinXShell
    if (_tcsstr(ext_options, TEXT("-install"))) {
        // install WinXShell into the registry
        TCHAR path[MAX_PATH];

        int l = GetModuleFileName(0, path, COUNTOF(path));
        if (l) {
            HKEY hkey;

            if (!RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hkey)) {

                ///@todo save previous shell application in config file

                RegSetValueEx(hkey, TEXT("Shell"), 0, REG_SZ, (LPBYTE)path, l * sizeof(TCHAR));
                RegCloseKey(hkey);
            }

            if (!RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hkey)) {

                ///@todo save previous shell application in config file

                RegSetValueEx(hkey, TEXT("Shell"), 0, REG_SZ, (LPBYTE)TEXT(""), l * sizeof(TCHAR));
                RegCloseKey(hkey);
            }
        }

        CloseShellProcess();

        startup_desktop = TRUE;
    } else {
        // create desktop window and task bar only, if there is no other shell and we are
        // the first explorer instance
        // MS Explorer looks additionally into the registry entry HKCU\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\shell,
        // to decide wether it is currently configured as shell application.
        startup_desktop = !any_desktop_running;
    }


    bool autostart = !any_desktop_running;

    // disable autostart if the SHIFT key is pressed
    if (GetAsyncKeyState(VK_SHIFT) < 0)
        autostart = false;

#ifdef _DEBUG    //MF: disabled for debugging
    autostart = false;
#endif

    _tsetlocale(LC_ALL, TEXT("")); //set locale for support multibyte character

    g_Globals.init(hInstance); /* init icon_cache for UI process */

    g_Globals.read_persistent();
    g_Globals.get_modulepath();
    g_Globals.load_config();
    g_Globals.get_systeminfo();
    g_Globals._cmdline = lpCmdLineOrg;

    string_t file(_T("WinXShell.lua"));
    TCHAR luascript[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("WINXSHELL_LUASCRIPT"), luascript, MAX_PATH);
    if (dw != 0) file = luascript;

#ifdef _DEBUG
    SetEnvironmentVariable(TEXT("WINXSHELL_DEBUG"), TEXT("1"));
#endif

    String mpath = JVAR("JVAR_MODULEPATH").ToString();
    SetEnvironmentVariable(TEXT("WINXSHELL_MODULEPATH"), mpath);
    if (_tcsstr(ext_options, TEXT("-ui")) == 0) {
#ifndef _DEBUG
        file = mpath + TEXT("\\") + file;
#endif
        if (PathFileExists(file.c_str())) {
            g_Globals._lua = new LuaAppEngine(file);
        }
    }

#ifndef __WINE__
    if (_tcsstr(ext_options, TEXT("-console"))) {
        AllocConsole();

        _dup2(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_RDONLY), 0);
        _dup2(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 0), 1);
        _dup2(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), 0), 2);

        g_Globals._log = _tfdopen(1, TEXT("w"));
        setvbuf(g_Globals._log, 0, _IONBF, 0);

        LOG(TEXT("starting winxshell debug log\n"));
    }
#endif

    TCHAR locale_buf[LOCALE_NAME_MAX_LENGTH];
    g_Globals._locale = _T("en-US");
    if (GetUserDefaultLocaleName(locale_buf, LOCALE_NAME_MAX_LENGTH) > 0) {
        g_Globals._locale = locale_buf;
    }
    SetEnvironmentVariable(TEXT("USERDEFAULT_LOCALENAME"), g_Globals._locale);

    CUIManager *pUIManager = NULL;
    if (_tcsstr(ext_options, TEXT("-uimgr"))) {
        HWND hwnd = CUIManager::GetUIManager();
        if (hwnd == NULL) {
            pUIManager = new CUIManager(hInstance, TRUE);
       }
    }

   if (g_Globals._lua) g_Globals._lua->onLoad();

    if (_tcsstr(ext_options, TEXT("-ui"))) {
#ifndef _DEBUG
        SetCurrentDirectory(JVAR("JVAR_MODULEPATH").ToString().c_str());
#endif
        g_Globals.get_uifolder();
        UIProcess(hInstance, lpCmdLineOrg);
        if (pUIManager) {
            Window::MessageLoop();
        }
        return 0;
    }

    // If there is given the command line option "-desktop", create desktop window anyways
    if (_tcsstr(ext_options, TEXT("-desktop")))
        startup_desktop = TRUE;
#ifndef ROSSHELL
    else if (_tcsstr(ext_options, TEXT("-nodesktop")))
        startup_desktop = FALSE;

    // Don't display cabinet window in desktop mode
    if (startup_desktop && !_tcsstr(ext_options, TEXT("-explorer")))
        nShowCmd = SW_HIDE;
#endif

    if (_tcsstr(ext_options, TEXT("-noautostart")))
        autostart = false;
    else if (_tcsstr(ext_options, TEXT("-autostart")))
        autostart = true;

    if (startup_desktop) {
        if (IsWindowsVistaOrGreater()) {
            // for Vista later
            if (!SetShellReadyEvent(TEXT("ShellDesktopSwitchEvent")))
                SetShellReadyEvent(TEXT("Global\\ShellDesktopSwitchEvent"));
        } else {
            // hide the XP login screen (Credit to Nicolas Escuder)
            // another undocumented event: "Global\\msgina: ReturnToWelcome"
            if (!SetShellReadyEvent(TEXT("msgina: ShellReadyEvent")))
                SetShellReadyEvent(TEXT("Global\\msgina: ShellReadyEvent"));
        }
    }
#ifdef ROSSHELL
    else
        return 0;    // no shell to launch, so exit immediatelly
#endif


    if (!any_desktop_running) {
        // launch the shell DDE server
        if (g_SHDOCVW_ShellDDEInit)
            (*g_SHDOCVW_ShellDDEInit)(TRUE);
    }

    if (_tcsstr(ext_options, TEXT("-debug"))) {
        DebugMode = true;
        Sleep(10000);
    }

    if (_tcsstr(ext_options, TEXT("-break"))) {
        LOG(TEXT("debugger breakpoint"));
#ifdef _MSC_VER
        DebugBreak();
#else
        asm("int3");
#endif
    }

    // initialize COM and OLE before creating the desktop window
    OleInit usingCOM;

    // init common controls library
    CommonControlInit usingCmnCtrl;

    // for loading UI Resources
#ifndef _DEBUG
    SetCurrentDirectory(JVAR("JVAR_MODULEPATH").ToString().c_str());
#endif

    if (_tcsstr(ext_options, TEXT("-color"))) {
        UpdateSysColor(lpCmdLineOrg);
    }

    if (_tcsstr(ext_options, TEXT("-settings"))) {
        return 0;
    }

    if (_tcsstr(ext_options, TEXT("-ocf"))) {
        OpenContainingFolder(lpCmdLineOrg);
        return 0;
    }

    if (_tcsstr(ext_options, TEXT("-daemon"))) {
        HWND daemon = WinXShell_DaemonWindow::Create();
        if (g_Globals._lua) g_Globals._lua->call("ondaemon");
        //hijack clockarea click event
        if (JCFG2_DEF("JS_DAEMON", "handle_clockarea_click", false).ToBool() != FALSE) {
            InitHook(daemon);
        }
        update_property_handler();
        Window::MessageLoop();
        return 0;
    }

    if (startup_desktop) {
        WaitCursor wait;

        WinXShell_DaemonWindow::Create();
        //create a ApplicationManager_DesktopShellWindow window for ClassicShell startmenu
        AM_DesktopShellWindow::Create();
        g_Globals._hwndDesktop = DesktopWindow::Create();

        if (g_Globals._lua) g_Globals._lua->onShell();
        update_property_handler();

#ifdef _USE_HDESK
        g_Globals._desktops.get_current_Desktop()->_hwndDesktop = g_Globals._hwndDesktop;
#endif
    }

    if (_tcsstr(ext_options, TEXT("-?"))) {
        MessageBoxA(g_Globals._hwndDesktop,
                    "\r\n"
                    "-?        display command line options\r\n"
                    "\r\n"
                    "-desktop        start in desktop mode regardless of an already running shell\r\n"
                    "-nodesktop    disable desktop mode\r\n"
                    "-explorer        display cabinet window regardless of enabled desktop mode\r\n"
                    "\r\n"
                    "-install        replace previous shell application with WinXShell\r\n"
                    "\r\n"
                    "-noautostart    disable autostarts\r\n"
                    "-autostart    enable autostarts regardless of debug build\r\n"
                    "\r\n"
                    "-console        open debug console\r\n"
                    "\r\n"
                    "-debug        activate GDB remote debugging stub\r\n"
                    "-break        activate debugger breakpoint\r\n",
                    "WinXShell - command line options", MB_OK);
    }

    Thread *pSSOThread = NULL;

    if (startup_desktop && JCFG_TB(2, "notaskbar").ToBool() == FALSE) {
        // launch SSO thread to allow message processing independent from the explorer main thread
        pSSOThread = new SSOThread;
        pSSOThread->Start();
    }


    /**TODO launching autostart programs can be moved into a background thread. */
    if (autostart) {
        const TCHAR *argv[] = {TEXT(""), TEXT("s")};    // call startup routine in SESSION_START mode
        startup(2, argv);
    }


    if (g_Globals._hwndDesktop) {
        g_Globals._desktop_mode = true;
        bool isfirstrun = (ShellHasBeenRun() == 0);
        if (isfirstrun && g_Globals._lua) g_Globals._lua->onFirstRun();
    }

    /* UIManager Process */
    if (!startup_desktop && pUIManager) {
         Window::MessageLoop();
         return 0;
    }

    int ret = explorer_main(hInstance, lpCmdLine, nShowCmd);


    // write configuration file
    //g_Globals.write_persistent();

    if (pSSOThread) {
        pSSOThread->Stop();
        delete pSSOThread;
    }

    FileExplorerWindow::ReleaseHook();

    if (!any_desktop_running) {
        // shutdown the shell DDE server
        if (g_SHDOCVW_ShellDDEInit)
            (*g_SHDOCVW_ShellDDEInit)(FALSE);
    }

    return ret;
}

void UIProcess(HINSTANCE hInst, String cmdline) {

    HWND hwnd = CUIManager::GetUIManager();
    if (hwnd == NULL) {
        CUIManager::CreateUI(hInst, cmdline);
    } else {
        SendMessage(hwnd, WM_UICREATE, 0, (LPARAM)(cmdline.c_str()));
    }
    return;
}
