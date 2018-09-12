#pragma once

#include <windows.h>
#include <CommCtrl.h>

#include <map>

#include "../utility/utility.h"
#include "../utility/shellclasses.h"
#include "../shell/entries.h"

enum ICON_TYPE {
    IT_STATIC,
    IT_CACHED,
    IT_DYNAMIC,
    IT_SYSCACHE
};

enum ICON_ID {
    ICID_UNKNOWN,
    ICID_NONE,

    ICID_FOLDER,
    //ICID_DOCUMENT,
    ICID_APP,
    ICID_EXPLORER,

    ICID_CONFIG,
    ICID_DOCUMENTS,
    ICID_FAVORITES,
    ICID_INFO,
    ICID_APPS,
    ICID_SEARCH,
    ICID_ACTION,
    ICID_SEARCH_DOC,
    ICID_PRINTER,
    ICID_NETWORK,
    ICID_COMPUTER,
    ICID_LOGOFF,
    ICID_SHUTDOWN,
    ICID_RESTART,
    ICID_TERMINATE,
    ICID_BOOKMARK,
    ICID_MINIMIZE,
    ICID_CONTROLPAN,
    ICID_DESKSETTING,
    ICID_NETCONNS,
    ICID_ADMIN,
    ICID_RECENT,

    ICID_TRAY_SND_NONE,
    ICID_TRAY_SND_MUTE,
    ICID_TRAY_SND_SMALL,
    ICID_TRAY_SND_MIDDLE,
    ICID_TRAY_SND_LARGE,
    ICID_TRAY_NET_WIRED_DIS,
    ICID_TRAY_NET_WIRED_LAN,
    ICID_TRAY_NET_WIRED_INTERNET,
    ICID_TRAY_NET_WIRELESS_DIS,
    ICID_TRAY_NET_WIRELESS_NOCONN,
    ICID_TRAY_NET_WIRELESS_LAN,
    //    ICID_TRAY_NET_WIRELESS_INTERNET,
    ICID_TRAY_NET_SIGNAL_NONE,
    ICID_TRAY_NET_SIGNAL_QUARTER1,
    ICID_TRAY_NET_SIGNAL_QUARTER2,
    ICID_TRAY_NET_SIGNAL_QUARTER3,
    ICID_TRAY_NET_SIGNAL_QUARTER4,

    ICID_DYNAMIC
};



#define ICON_SIZE_SMALL     16  // GetSystemMetrics(SM_CXSMICON)
#define ICON_SIZE_MIDDLE    24  // special size for start menu root icons
#define ICON_SIZE_LARGE     32  // GetSystemMetrics(SM_CXICON)

#define ICON_SIZE_FROM_ICF(flags)   (flags&ICF_LARGE? ICON_SIZE_LARGE: flags&ICF_MIDDLE? ICON_SIZE_MIDDLE: ICON_SIZE_SMALL)
#define ICF_FROM_ICON_SIZE(size)    (size>=ICON_SIZE_LARGE? ICF_LARGE: size>=ICON_SIZE_MIDDLE? ICF_MIDDLE: ICF_SMALL)

struct Icon {
    Icon();
    Icon(ICON_ID id, UINT nid);
    Icon(ICON_ID id, UINT nid, int icon_size);
    Icon(ICON_TYPE itype, int id, HICON hIcon);
    Icon(ICON_TYPE itype, int id, int sys_idx);

    operator ICON_ID() const { return _id; }

    void    draw(HDC hdc, int x, int y, int cx, int cy, COLORREF bk_color, HBRUSH bk_brush) const;
    HBITMAP create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd, int icon_size = ICON_SIZE_SMALL) const;
    HBITMAP create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd, int icon_size, RECT rect) const;
    int     add_to_imagelist(HIMAGELIST himl, HDC hdc_wnd, COLORREF bk_color = GetSysColor(COLOR_WINDOW), HBRUSH bk_brush = GetSysColorBrush(COLOR_WINDOW)) const;

    int     get_sysiml_idx() const { return _itype == IT_SYSCACHE ? _sys_idx : -1; }
    HICON   get_hicon() const { return _itype != IT_SYSCACHE ? _hicon : 0; }
    ICON_TYPE get_icontype() const { return _itype; }

    bool    destroy() { if (_itype == IT_DYNAMIC) { DestroyIcon(_hicon); return true; } else return false; }

protected:
    ICON_ID _id;
    ICON_TYPE _itype;
    HICON   _hicon;
    int     _sys_idx;
};

struct SysCacheIcon : public Icon {
    SysCacheIcon(int id, int sys_idx)
        : Icon(IT_SYSCACHE, id, sys_idx)
    {
    }
};

struct IconCache {
    IconCache() : _himlSys_small(0) {}

    virtual ~IconCache();
    void    init();

    const Icon &extract(LPCTSTR path, UINT flags = ICF_NORMAL);
    const Icon &extract(LPCTSTR path, int icon_idx, UINT flags = ICF_HICON);
    const Icon &extract(IExtractIcon *pExtract, LPCTSTR path, int icon_idx, UINT flags = ICF_HICON);
    const Icon &extract(LPCITEMIDLIST pidl, UINT flags = ICF_NORMAL);

    const Icon &add(HICON hIcon, ICON_TYPE type = IT_DYNAMIC);
    const Icon &add(int sys_idx/*, ICON_TYPE type=IT_SYSCACHE*/);

    const Icon &get_icon(int icon_id);

    HIMAGELIST get_sys_imagelist() const { return _himlSys_small; }

    void    free_icon(int icon_id);

protected:
    static int s_next_id;

    typedef map<int, Icon> IconMap;
    IconMap _icons;

    typedef pair<String, int/*ICONCACHE_FLAGS*/> CacheKey;
    typedef map<CacheKey, ICON_ID> PathCacheMap;
    PathCacheMap _pathCache;

    typedef pair<String, pair<int, int/*ICONCACHE_FLAGS*/> > IdxCacheKey;
    typedef map<IdxCacheKey, ICON_ID> IdxCacheMap;
    IdxCacheMap _idxCache;

    typedef pair<ShellPath, int/*ICONCACHE_FLAGS*/> PidlCacheKey;
    typedef map<PidlCacheKey, ICON_ID> PidlCacheMap;
    PidlCacheMap _pidlcache;

    HIMAGELIST _himlSys_small;
};

/// create a bitmap from an icon
extern HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd, int icon_size = ICON_SIZE_SMALL);
extern HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd, int icon_size, RECT rect);

/// add icon with alpha channel to imagelist using the specified background color
extern int ImageList_AddAlphaIcon(HIMAGELIST himl, HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd);

/// retrieve icon from window
extern HICON get_window_icon_small(HWND hwnd);
extern HICON get_window_icon_big(HWND hwnd, bool allow_from_class = true);

/// convenient loading of string resources
struct ResString : public String {
    ResString(UINT nid);
};

/// convenient loading of standard (32x32) icon resources
struct ResIcon {
    ResIcon(UINT nid);

    operator HICON() const { return _hicon; }

protected:
    HICON   _hicon;
};

/// convenient loading of small (16x16) icon resources
struct SmallIcon {
    SmallIcon(UINT nid);

    operator HICON() const { return _hicon; }

protected:
    HICON   _hicon;
};


struct SizeIcon {
    SizeIcon(UINT nid, int size);

    operator HICON() const { return _hicon; }

protected:
    HICON   _hicon;
};

/// convenient loading of icon resources with specified sizes
struct ResIconEx {
    ResIconEx(UINT nid, int w, int h);

    operator HICON() const { return _hicon; }

protected:
    HICON   _hicon;
};

/// set big and small icons out of the resources for a window
extern void SetWindowIcon(HWND hwnd, UINT nid);

/// convenient loading of bitmap resources
struct ResBitmap {
    ResBitmap(UINT nid);
    ~ResBitmap() { DeleteObject(_hBmp); }

    operator HBITMAP() const { return _hBmp; }

protected:
    HBITMAP _hBmp;
};

