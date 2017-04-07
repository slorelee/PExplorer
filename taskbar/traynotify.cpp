/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
// traynotify.cpp
//
// Martin Fuchs, 22.08.2003
//


#include <precomp.h>
#include <time.h>
#include <ShellAPI.h>

#include "../resource.h"

#include "traynotify.h"

#ifdef USE_NOTIFYHOOK
#include "../notifyhook/notifyhook.h"

NotifyHook::NotifyHook()
    :  WM_GETMODULEPATH(InstallNotifyHook())
{
}

NotifyHook::~NotifyHook()
{
    DeinstallNotifyHook();
}

void NotifyHook::GetModulePath(HWND hwnd, HWND hwndCallback)
{
    PostMessage(hwnd, WM_GETMODULEPATH, (WPARAM)hwndCallback, 0);
}

bool NotifyHook::ModulePathCopyData(LPARAM lparam, HWND *phwnd, String &path)
{
    char buffer[MAX_PATH];

    int l = GetWindowModulePathCopyData(lparam, phwnd, buffer, COUNTOF(buffer));

    if (l) {
        path.assign(buffer, l);
        return true;
    } else
        return false;
}
#endif //USE_NOTIFYHOOK

const int PF_NOTIFYICONDATAA_V1_SIZE = FIELD_OFFSET(X86_NOTIFYICONDATAA, szTip[64]);
const int PF_NOTIFYICONDATAW_V1_SIZE = FIELD_OFFSET(X86_NOTIFYICONDATAW, szTip[64]);

const int PF_NOTIFYICONDATAA_V2_SIZE = FIELD_OFFSET(X86_NOTIFYICONDATAA, guidItem);
const int PF_NOTIFYICONDATAW_V2_SIZE = FIELD_OFFSET(X86_NOTIFYICONDATAW, guidItem);

const int PF_NOTIFYICONDATAA_V3_SIZE = FIELD_OFFSET(X86_NOTIFYICONDATAA, hBalloonIcon);
const int PF_NOTIFYICONDATAW_V3_SIZE = FIELD_OFFSET(X86_NOTIFYICONDATAW, hBalloonIcon);

const int PF_NOTIFYICONDATAA_V4_SIZE = sizeof(X86_NOTIFYICONDATAA);
const int PF_NOTIFYICONDATAW_V4_SIZE = sizeof(X86_NOTIFYICONDATAW);

// x86 UNICODE versions von NOTIFYICONDATA
#define IS_NID_UNICODE_SIZE_X86(data) (((data) == (PF_NOTIFYICONDATAW_V1_SIZE)) ||\
                                   ((data) == (PF_NOTIFYICONDATAW_V2_SIZE)) ||\
                                   ((data) == (PF_NOTIFYICONDATAW_V3_SIZE)) ||\
                                   ((data) == (PF_NOTIFYICONDATAW_V4_SIZE)))

// x86 ANSI versions von NOTIFYICONDATA
#define IS_NID_ANSI_SIZE_X86(data) (((data) == (PF_NOTIFYICONDATAA_V1_SIZE)) ||\
                                ((data) == (PF_NOTIFYICONDATAA_V2_SIZE)) ||\
                                ((data) == (PF_NOTIFYICONDATAA_V3_SIZE)) ||\
                                ((data) == (PF_NOTIFYICONDATAA_V4_SIZE)))


// x64 UNICODE versions von NOTIFYICONDATA
#define IS_NID_UNICODE_SIZE_X64(data) (((data) == ((PF_NOTIFYICONDATAW_V1_SIZE) + 16)) ||\
                                   ((data) == ((PF_NOTIFYICONDATAW_V2_SIZE) + 16)) ||\
                                   ((data) == ((PF_NOTIFYICONDATAW_V3_SIZE) + 16)) ||\
                                   ((data) == ((PF_NOTIFYICONDATAW_V4_SIZE) + 20)))

// x64 ANSI versions von NOTIFYICONDATA
#define IS_NID_ANSI_SIZE_X64(data) (((data) == ((PF_NOTIFYICONDATAA_V1_SIZE) + 16)) ||\
                                ((data) == ((PF_NOTIFYICONDATAA_V2_SIZE) + 16)) ||\
                                ((data) == ((PF_NOTIFYICONDATAA_V3_SIZE) + 16)) ||\
                                ((data) == ((PF_NOTIFYICONDATAA_V4_SIZE) + 20)))

#define IS_NID_UNICODE_SIZE(data) ((IS_NID_UNICODE_SIZE_X86(data)) || (IS_NID_UNICODE_SIZE_X64(data)))
#define IS_NID_ANSI_SIZE(data) ((IS_NID_ANSI_SIZE_X86(data)) || (IS_NID_ANSI_SIZE_X64(data)))


static UINT GET_NID_uID(NOTIFYICONDATA *pnid)
{
    UINT32 uid = 0;
    DWORD size = ((NOTIFYICONDATA *)pnid)->cbSize;
    if (IS_NID_ANSI_SIZE_X86(size)) {
        uid = ((X86_NOTIFYICONDATAA *)pnid)->uID;
    } else if (IS_NID_UNICODE_SIZE_X86(size)) {
        uid = ((X86_NOTIFYICONDATAW *)pnid)->uID;
    } else if (IS_NID_ANSI_SIZE_X64(size)) {
        uid = ((X64_NOTIFYICONDATAA *)pnid)->uID;
    } else if (IS_NID_UNICODE_SIZE_X64(size)) {
        uid = ((X64_NOTIFYICONDATAW *)pnid)->uID;
    }
    return (UINT)uid;
}

static UINT GET_NID_uVersion(NOTIFYICONDATA *pnid)
{
    UINT32 uversion = 0;
    DWORD size = ((NOTIFYICONDATA *)pnid)->cbSize;
    if (IS_NID_ANSI_SIZE_X86(size)) {
        uversion = ((X86_NOTIFYICONDATAA *)pnid)->UNION_MEMBER(uVersion);
    } else if (IS_NID_UNICODE_SIZE_X86(size)) {
        uversion = ((X86_NOTIFYICONDATAW *)pnid)->UNION_MEMBER(uVersion);
    } else if (IS_NID_ANSI_SIZE_X64(size)) {
        uversion = ((X64_NOTIFYICONDATAA *)pnid)->UNION_MEMBER(uVersion);
    } else if (IS_NID_UNICODE_SIZE_X64(size)) {
        uversion = ((X64_NOTIFYICONDATAW *)pnid)->UNION_MEMBER(uVersion);
    }
    return (UINT)uversion;
}

NotifyIconIndex::NotifyIconIndex(NOTIFYICONDATA *pnid)
{
    ULONG32 size = ((NOTIFYICONDATA *)pnid)->cbSize;
    if (IS_NID_ANSI_SIZE_X86(size)) {
        _hWnd = (HWND)((X86_NOTIFYICONDATAA *)pnid)->hWnd;
        _uID = ((X86_NOTIFYICONDATAA *)pnid)->uID;
    } else if (IS_NID_UNICODE_SIZE_X86(size)) {
        _hWnd = (HWND)((X86_NOTIFYICONDATAW *)pnid)->hWnd;
        _uID = ((X86_NOTIFYICONDATAW *)pnid)->uID;
    } else if (IS_NID_ANSI_SIZE_X64(size)) {
        _hWnd = (HWND)((X64_NOTIFYICONDATAA *)pnid)->hWnd;
        _uID = ((X64_NOTIFYICONDATAA *)pnid)->uID;
    } else if (IS_NID_UNICODE_SIZE_X64(size)) {
        _hWnd = (HWND)((X64_NOTIFYICONDATAW *)pnid)->hWnd;
        _uID = ((X64_NOTIFYICONDATAW *)pnid)->uID;
    }

    // special handling for windows task manager
    //if ((INT32)_uID < 0) {
    //    _uID = 0;
    //}
}

NotifyIconIndex::NotifyIconIndex()
{
    _hWnd = 0;
    _uID = 0;
}

NotifyInfo::NotifyInfo()
{
    _idx = -1;
    _hIcon = 0;
    _dwState = 0;
    _uCallbackMessage = 0;
    _version = 0;

    _mode = NIM_SHOW;
    _lastChange = GetTickCount();
}

bool NotifyInfo::modify(void *pnid)
{
    bool changes = false;
    ULONG32 size = ((NOTIFYICONDATA *)pnid)->cbSize;
    if (IS_NID_ANSI_SIZE_X86(size)) {
        changes = _modify((X86_NOTIFYICONDATAA *)pnid);
    } else if (IS_NID_UNICODE_SIZE_X86(size)) {
        changes = _modify((X86_NOTIFYICONDATAW *)pnid);
    } else if (IS_NID_ANSI_SIZE_X64(size)) {
        changes = _modify((X64_NOTIFYICONDATAA *)pnid);
    } else if (IS_NID_UNICODE_SIZE_X64(size)) {
        changes = _modify((X64_NOTIFYICONDATAW *)pnid);
    }

    return set_title() || changes;

}

template<typename NID_T>
bool NotifyInfo::_modify(NID_T *pnid)
{
    bool changes = false;

    if (_hWnd != (HWND)pnid->hWnd || _uID != pnid->uID) {
        _hWnd = (HWND)pnid->hWnd;
        _uID = pnid->uID;

        changes = true;
    }

    if (pnid->uFlags & NIF_MESSAGE) {
        if (_uCallbackMessage != pnid->uCallbackMessage) {
            _uCallbackMessage = pnid->uCallbackMessage;
            changes = true;
        }
    }

    if (pnid->uFlags & NIF_ICON) {
        // Some applications destroy the icon immediatelly after completing the
        // NIM_ADD/MODIFY message, so we have to make a copy of it.
        if (_hIcon)
            DestroyIcon(_hIcon);

        _hIcon = (HICON) CopyImage((HICON)pnid->hIcon, IMAGE_ICON, NOTIFYICON_SIZE, NOTIFYICON_SIZE, 0);

        changes = true; ///@todo compare icon
    }

#ifdef NIF_STATE    // as of 21.08.2003 missing in MinGW headers
    if (pnid->uFlags & NIF_STATE) {
        DWORD new_state = (_dwState & ~pnid->dwStateMask) | (pnid->dwState & pnid->dwStateMask);

        if (_dwState != new_state) {
            _dwState = new_state;
            changes = true;
        }
    }
#endif

    // store tool tip text
    if (pnid->uFlags & NIF_TIP) {
        int max_len = 128;
        String new_text(pnid->szTip, max_len);

        //new_text.assign(txt, l);

        if (new_text != _tipText) {
            _tipText = new_text;
            changes = true;
        }
    }

    return changes;
}

bool NotifyInfo::set_title()
{
    bool changes = false;
    TCHAR title[MAX_PATH];

    DWORD pid;
    GetWindowThreadProcessId(_hWnd, &pid);

    // avoid to send WM_GETTEXT messages to the own process
    if (pid != GetCurrentProcessId()) {
        if (GetWindowText(_hWnd, title, COUNTOF(title))) {
            if (_windowTitle != title) {
                _windowTitle = title;
                changes = true;
            }
        }

        if (changes) {
            create_name();
            _lastChange = GetTickCount();
        }
    }

    return changes;
}

NotifyArea::NotifyArea(HWND hwnd)
    :  super(hwnd),
       _tooltip(hwnd)
{
    _next_idx = 0;
    _clock_width = 0;
    _last_icon_count = 0;
    _show_hidden = false;
    _hide_inactive = true;
    _show_button = true;
}

NotifyArea::~NotifyArea()
{
    KillTimer(_hwnd, 0);

    write_config();
}

static bool get_hide_clock_from_registry()
{
    HKEY hkeyStuckRects = 0;
    DWORD buffer[10];
    DWORD len = sizeof(buffer);

    bool hide_clock = false;

    // check if the clock should be hidden
    if (!RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects2"), &hkeyStuckRects) &&
        !RegQueryValueEx(hkeyStuckRects, TEXT("Settings"), 0, NULL, (LPBYTE)buffer, &len) &&
        len == sizeof(buffer) && buffer[0] == sizeof(buffer))
        hide_clock = buffer[2] & 0x08 ? true : false;

    if (hkeyStuckRects)
        RegCloseKey(hkeyStuckRects);

    return hide_clock;
}

void NotifyArea::read_config()
{
    bool clock_visible = true;
    show_clock(clock_visible);
}

void NotifyArea::write_config()
{
}

void NotifyArea::show_clock(bool flag)
{
    bool vis = _hwndClock != 0;

    if (vis != flag) {
        if (flag) {
            // create clock window
            _hwndClock = ClockWindow::Create(_hwnd);

            if (_hwndClock) {
                ClientRect clock_size(_hwndClock);
                _clock_width = clock_size.right;
            }
        } else {
            DestroyWindow(_hwndClock);
            _hwndClock = 0;
            _clock_width = 0;
        }

        SendMessage(GetParent(_hwnd), PM_RESIZE_CHILDREN, 0, 0);
    }
}

LRESULT NotifyArea::Init(LPCREATESTRUCT pcs)
{
    if (super::Init(pcs))
        return 1;

    read_config();

    SetTimer(_hwnd, 0, 1000, NULL);

    return 0;
}

#ifdef _DEBUG
static TCHAR *getmsgstr(UINT msgid)
{
    TCHAR *msg = TEXT("");
    switch (msgid) {
    case NIN_SELECT:msg = TEXT("NIN_SELECT"); break;
    case NIN_KEYSELECT:msg = TEXT("NIN_KEYSELECT"); break;
    case WM_CONTEXTMENU: msg = TEXT("WM_CONTEXTMENU"); break;
    case WM_MOUSEMOVE: msg = TEXT("WM_MOUSEMOVE"); break;
    case WM_LBUTTONDOWN: msg = TEXT("WM_LBUTTONDOWN"); break;
    case WM_LBUTTONUP: msg = TEXT("WM_LBUTTONUP"); break;
    case WM_LBUTTONDBLCLK: msg = TEXT("WM_LBUTTONDBLCLK"); break;
    case WM_RBUTTONDOWN: msg = TEXT("WM_RBUTTONDOWN"); break;
    case WM_RBUTTONUP: msg = TEXT("WM_RBUTTONUP"); break;
    case WM_RBUTTONDBLCLK: msg = TEXT("WM_RBUTTONDBLCLK"); break;
    case WM_MBUTTONDOWN: msg = TEXT("WM_MBUTTONDOWN"); break;
    case WM_MBUTTONUP: msg = TEXT("WM_MBUTTONUP"); break;
    case WM_MBUTTONDBLCLK: msg = TEXT("WM_MBUTTONDBLCLK"); break;
    default: _log_(FmtString(TEXT("TRAYICON MSG = %d"), msgid));
    }
    return msg;
}
#endif

static BOOL TrayNotifyMessage(HWND hwnd, const NotifyInfo &entry, LPARAM lparam, POINT pt)
{
#ifdef _DEBUG
    _log_(FmtString(TEXT("TRAYICON MSG = %s"), getmsgstr(lparam)));
#endif
    if (entry._version == NOTIFYICON_VERSION_4)
    {
        POINT messagePt = pt;
        ClientToScreen(hwnd, &messagePt);
        //if (lparam == NIN_SELECT) lparam = NIN_KEYSELECT;
        WPARAM wparam = MAKEWPARAM(messagePt.x, messagePt.y);
        return PostMessage(entry._hWnd, entry._uCallbackMessage, wparam, MAKELPARAM(lparam, entry._uID)) == S_OK;
    }

    return PostMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, lparam) == S_OK;
}

HWND NotifyArea::Create(HWND hwndParent)
{
    static BtnWindowClass wcTrayNotify(CLASSNAME_TRAYNOTIFY, CS_DBLCLKS);
    wcTrayNotify.hbrBackground = TASKBAR_BRUSH();
    ClientRect clnt(hwndParent);

    return Window::Create(WINDOW_CREATOR(NotifyArea), 0,
                          wcTrayNotify, TITLE_TRAYNOTIFY, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                          clnt.right - (NOTIFYAREA_WIDTH_DEF + 1), 1, NOTIFYAREA_WIDTH_DEF, clnt.bottom - 2, hwndParent);
}

LRESULT NotifyArea::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch (nmsg) {
    case WM_PAINT:
        Paint();
        break;

    case WM_TIMER: {
        Refresh();

        ClockWindow *clock_window = GET_WINDOW(ClockWindow, _hwndClock);

        if (clock_window)
            clock_window->TimerTick();
        break;
    }

    case PM_REFRESH:
        Refresh(true);
        break;

    case WM_SIZE: {
        int cx = LOWORD(lparam);
        SetWindowPos(_hwndClock, 0, cx - _clock_width, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    case PM_GET_WIDTH: {
        size_t w = _sorted_icons.size() * NOTIFYICON_DIST + NOTIFYAREA_SPACE + _clock_width;
        if (_show_button)
            w += NOTIFYICON_DIST;
        return w;
    }

    case PM_REFRESH_CONFIG:
        read_config();
        break;

    case WM_CONTEXTMENU: {
        Point pt(lparam);
        POINTS p;
        p.x = (SHORT) pt.x;
        p.y = (SHORT) pt.y;
        ScreenToClient(_hwnd, &pt);

        if (IconHitTest(pt) == _sorted_icons.end()) { // display menu only when no icon clicked
            PopupMenu menu(IDM_NOTIFYAREA);
            SetMenuDefaultItem(menu, 0, MF_BYPOSITION);
            if (GetKeyState(VK_SHIFT) < 0) {
                menu.Append(0, NULL, MF_SEPARATOR);
                menu.Append(ID_ABOUT_EXPLORER, ResString(IDS_ABOUT_EXPLORER));
            }
            CheckMenuItem(menu, ID_SHOW_HIDDEN_ICONS, MF_BYCOMMAND | (_show_hidden ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(menu, ID_SHOW_ICON_BUTTON, MF_BYCOMMAND | (_show_button ? MF_CHECKED : MF_UNCHECKED));
            menu.TrackPopupMenu(_hwnd, p);
        }
        break;
    }

#ifdef USE_NOTIFYHOOK
    case WM_COPYDATA: {   // receive NotifyHook answers
        String path;
        HWND hwnd;

        if (_hook.ModulePathCopyData(lparam, &hwnd, path))
            _window_modules[hwnd] = path;
        break;
    }
#endif

    default:
        if (nmsg >= WM_MOUSEFIRST && nmsg <= WM_MOUSELAST) {
            // close startup menu and other popup menus
            // This functionality is missing in MS Windows.
            if (nmsg == WM_LBUTTONDOWN || nmsg == WM_MBUTTONDOWN || nmsg == WM_RBUTTONDOWN
#ifdef WM_XBUTTONDOWN
                || nmsg == WM_XBUTTONDOWN
#endif
               )

                CancelModes();

            Point pt(lparam);
            NotifyIconSet::const_iterator found = IconHitTest(pt);

            if (found != _sorted_icons.end()) {
                const NotifyInfo &entry = const_cast<NotifyInfo &>(*found); // Why does GCC 3.3 need this additional const_cast ?!
                // allow SetForegroundWindow() in client process
                DWORD pid;

                if (GetWindowThreadProcessId(entry._hWnd, &pid)) {
                    // bind dynamically to AllowSetForegroundWindow() to be compatible to WIN98
                    static DynamicFct<BOOL(WINAPI *)(DWORD)> AllowSetForegroundWindow(TEXT("USER32"), "AllowSetForegroundWindow");

                    if (AllowSetForegroundWindow)
                        (*AllowSetForegroundWindow)(pid);
                }

                // set activation time stamp
                if (nmsg == WM_LBUTTONDOWN ||
                    nmsg == WM_MBUTTONDOWN ||
#ifdef WM_XBUTTONDOWN
                    nmsg == WM_XBUTTONDOWN ||
#endif
                    nmsg == WM_RBUTTONDOWN) {
                    _icon_map[entry]._lastChange = GetTickCount();
                }

                // Notify the message if the owner is still alive
                if (IsWindow(entry._hWnd)) {
                    if (//nmsg == WM_MOUSEMOVE ||
                        nmsg == WM_LBUTTONDOWN || nmsg == WM_LBUTTONUP || nmsg == WM_LBUTTONDBLCLK ||
                        nmsg == WM_MBUTTONDOWN || nmsg == WM_MBUTTONUP || nmsg == WM_MBUTTONDBLCLK ||
#ifdef WM_XBUTTONDOWN
                        nmsg == WM_XBUTTONDOWN || nmsg == WM_XBUTTONUP || nmsg == WM_XBUTTONDBLCLK ||
#endif
                        nmsg == WM_RBUTTONDOWN || nmsg == WM_RBUTTONUP || nmsg == WM_RBUTTONDBLCLK) {
                        if (nmsg == WM_LBUTTONUP) {
                            TrayNotifyMessage(_hwnd, entry, NIN_SELECT, pt);
                            return TrayNotifyMessage(_hwnd, entry, WM_LBUTTONUP, pt);
                        } else if (nmsg == WM_RBUTTONUP) {
                            TrayNotifyMessage(_hwnd, entry, WM_RBUTTONUP, pt);
                            return TrayNotifyMessage(_hwnd, entry, WM_CONTEXTMENU, pt);
                        } else {
                            return TrayNotifyMessage(_hwnd, entry, nmsg, pt);
                        }
                    }
                } else if (_icon_map.erase(entry))  // delete icons without valid owner window
                    UpdateIcons();
            } else
                // handle clicks on notification area button "show hidden icons"
                if (_show_button)
                    if (nmsg == WM_LBUTTONDOWN)
                        if (pt.x >= NOTIFYICON_X && pt.x < NOTIFYICON_X + NOTIFYICON_SIZE &&
                            pt.y >= NOTIFYICON_Y && pt.y < NOTIFYICON_Y + NOTIFY_HINT_Y)
                            PostMessage(_hwnd, WM_COMMAND, MAKEWPARAM(ID_SHOW_HIDDEN_ICONS, 0), 0);
        }

        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

int NotifyArea::Command(int id, int code)
{
    switch (id) {
    case ID_SHOW_HIDDEN_ICONS:
        _show_hidden = !_show_hidden;
        UpdateIcons();
        break;

    case ID_SHOW_ICON_BUTTON:
        _show_button = !_show_button;
        UpdateIcons();
        break;

    case ID_CONFIG_NOTIFYAREA:
        Dialog::DoModal(IDD_NOTIFYAREA, WINDOW_CREATOR(TrayNotifyDlg), GetParent(_hwnd));
        break;

    case ID_CONFIG_TIME:
        launch_cpanel(_hwnd, TEXT("timedate.cpl"));
        break;

    default:
        SendParent(WM_COMMAND, MAKELONG(id, code), 0);
    }

    return 0;
}

int NotifyArea::Notify(int id, NMHDR *pnmh)
{
    if (pnmh->code == TTN_GETDISPINFO) {
        LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

        Point pt(GetMessagePos());
        ScreenToClient(_hwnd, &pt);

        if (_show_button &&
            pt.x >= NOTIFYICON_X && pt.x < NOTIFYICON_X + NOTIFYICON_DIST &&
            pt.y >= NOTIFYICON_Y && pt.y < NOTIFYICON_Y + NOTIFY_HINT_Y) {
            static ResString sShowIcons(IDS_SHOW_HIDDEN_ICONS);
            static ResString sHideIcons(IDS_HIDE_ICONS);

            pdi->lpszText = (_show_hidden ? sHideIcons : sShowIcons).str();
        } else {
            NotifyIconSet::iterator found = IconHitTest(pt);

            if (found != _sorted_icons.end()) {
                NotifyInfo &entry = const_cast<NotifyInfo &>(*found);   // Why does GCC 3.3 need this additional const_cast ?!

                // enable multiline tooltips (break at CR/LF and for very long one-line strings)
                SendMessage(pnmh->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 400);

                pdi->lpszText = entry._tipText.str();
            }
        }
    }

    return 0;
}

void NotifyArea::CancelModes()
{
    PostMessage(HWND_BROADCAST, WM_CANCELMODE, 0, 0);

    for (NotifyIconSet::const_iterator it = _sorted_icons.begin(); it != _sorted_icons.end(); ++it)
        PostMessage(it->_hWnd, WM_CANCELMODE, 0, 0);
}

LRESULT NotifyArea::ProcessTrayNotification(int notify_code, NOTIFYICONDATA *pnid)
{
    bool changes = false;

    switch (notify_code) {
    case NIM_ADD:
    case NIM_MODIFY: {
        // special handling for windows Task Manager
        UINT uid = GET_NID_uID(pnid);
        if ((INT32)uid < 0) { // handle Task Manager's uIDs - 0xffffffff~0xfffffff4
            //OutputDebugString(FmtString("Task - %d %d 0x%x\r\n", (INT16)uid,(INT32)uid, uid));
            return TRUE;
        }

        NotifyInfo &entry = _icon_map[pnid];

        // a new entry?
        if (entry._idx == -1)
            entry._idx = ++_next_idx;

        bool changes = entry.modify((void *)pnid);

#if NOTIFYICON_VERSION>=3   // as of 21.08.2003 missing in MinGW headers
        if (DetermineHideState(entry) && entry._mode == NIM_HIDE) {
            entry._dwState |= NIS_HIDDEN;
            changes = true;
        }
#endif

        if (changes)
            UpdateIcons();  ///@todo call only if really changes occurred

        return TRUE;
    }
    break;
    case NIM_DELETE: {
        NotifyIconMap::iterator found = _icon_map.find(pnid);
        if (found != _icon_map.end()) {
            if (found->second._hIcon)
                DestroyIcon(found->second._hIcon);
            _icon_map.erase(found);
            UpdateIcons();
            return TRUE;
        }
        break;
    }

#if NOTIFYICON_VERSION>=3   // as of 21.08.2003 missing in MinGW headers
    case NIM_SETFOCUS:
        SetForegroundWindow(_hwnd);
        return TRUE;

    case NIM_SETVERSION:
        NotifyIconMap::iterator found = _icon_map.find(pnid);

        if (found != _icon_map.end()) {
            found->second._version = GET_NID_uVersion(pnid);
            return TRUE;
        } else
            return FALSE;
#endif
    }

    return FALSE;
}

void NotifyArea::UpdateIcons()
{
    _sorted_icons.clear();

    // sort icon infos by display index
    for (NotifyIconMap::const_iterator it = _icon_map.begin(); it != _icon_map.end(); ++it) {
        const NotifyInfo &entry = it->second;

#ifdef NIF_STATE    // as of 21.08.2003 missing in MinGW headers
        if (_show_hidden || !(entry._dwState & NIS_HIDDEN))
#endif
            _sorted_icons.insert(entry);
    }

    // sync tooltip areas to current icon number
    size_t current_icon_count = _sorted_icons.size();
    if (_show_button) current_icon_count++;

    if (current_icon_count != _last_icon_count) {
        RECT rect = {NOTIFYICON_X, NOTIFYICON_Y, NOTIFYICON_X + NOTIFYICON_DIST - 1, NOTIFYICON_Y + NOTIFY_HINT_Y - 1};

        UINT tt_idx = 0;

        if (_show_button) {
            _tooltip.add(_hwnd, tt_idx++, rect);

            rect.left += NOTIFYICON_DIST;
            rect.right += NOTIFYICON_DIST;
        }

        while (tt_idx < current_icon_count) {
            _tooltip.add(_hwnd, tt_idx++, rect);

            rect.left += NOTIFYICON_DIST;
            rect.right += NOTIFYICON_DIST;
        }

        while (tt_idx < _last_icon_count)
            _tooltip.remove(_hwnd, tt_idx++);

        _last_icon_count = current_icon_count;
    }

    SendMessage(GetParent(_hwnd), PM_RESIZE_CHILDREN, 0, 0);

    InvalidateRect(_hwnd, NULL, FALSE); // refresh icon display
    UpdateWindow(_hwnd);
}

#ifndef _NO_ALPHABLEND
#ifdef _MSC_VER
#pragma comment(lib, "msimg32") // for AlphaBlend()
#endif
#endif

void NotifyArea::Paint()
{
    BufferedPaintCanvas canvas(_hwnd);

    // first fill with the background color
    FillRect(canvas, &canvas.rcPaint, TASKBAR_BRUSH());

    // draw icons
    int x = NOTIFYICON_X;
    int y = NOTIFYICON_Y;

    if (_show_button) {
        static int initIcon = 0;
        static SizeIcon leftArrowIcon(IDI_NOTIFY_L_B, NOTIFYICON_SIZE);
        static SizeIcon rightArrowIcon(IDI_NOTIFY_R_B, NOTIFYICON_SIZE);
        if (initIcon == 0) {
            initIcon = 1;
            if (JCFG2("JS_TASKBAR", "theme").ToString().compare(TEXT("dark")) == 0) {
                leftArrowIcon = SizeIcon(IDI_NOTIFY_L_W, NOTIFYICON_SIZE);
                rightArrowIcon = SizeIcon(IDI_NOTIFY_R_W, NOTIFYICON_SIZE);
            }
        }
        DrawIconEx(canvas, x + (NOTIFYICON_DIST - NOTIFYICON_SIZE) / 2, y + ((DESKTOPBARBAR_HEIGHT - NOTIFYICON_Y * 2) - NOTIFYICON_SIZE) / 2, _show_hidden ? rightArrowIcon : leftArrowIcon, NOTIFYICON_SIZE, NOTIFYICON_SIZE, 0, 0, DI_NORMAL);
        x += NOTIFYICON_DIST;
    }

#ifndef _NO_ALPHABLEND
    MemCanvas mem_dc;
    SelectedBitmap bmp(mem_dc, CreateCompatibleBitmap(canvas, NOTIFYICON_SIZE, NOTIFYICON_SIZE));
    RECT rect = {0, 0, NOTIFYICON_SIZE, NOTIFYICON_SIZE};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 128, 0}; // 50 % visible
#endif

    for (NotifyIconSet::const_iterator it = _sorted_icons.begin(); it != _sorted_icons.end(); ++it) {
#ifndef _NO_ALPHABLEND
        if (it->_dwState & NIS_HIDDEN) {
            FillRect(mem_dc, &rect, GetSysColorBrush(COLOR_BTNFACE));
            DrawIconEx(mem_dc, 0, 0, it->_hIcon, NOTIFYICON_SIZE, NOTIFYICON_SIZE, 0, 0, DI_NORMAL);
            AlphaBlend(canvas, x, y, NOTIFYICON_SIZE, NOTIFYICON_SIZE, mem_dc, 0, 0, NOTIFYICON_SIZE, NOTIFYICON_SIZE, blend);
        }
        else
#endif
        {
            int left = x + (NOTIFYICON_DIST - NOTIFYICON_SIZE) / 2;
            int top = y + ((DESKTOPBARBAR_HEIGHT - NOTIFYICON_Y * 2) - NOTIFYICON_SIZE) / 2;
            DrawIconEx(canvas, left, top, it->_hIcon, NOTIFYICON_SIZE, NOTIFYICON_SIZE, 0, 0, DI_NORMAL);
        }
        x += NOTIFYICON_DIST;
    }
}

void NotifyArea::Refresh(bool update)
{
    // Look for task icons without valid owner window.
    // This is an extended feature missing in MS Windows.
    for (NotifyIconSet::const_iterator it = _sorted_icons.begin(); it != _sorted_icons.end(); ++it) {
        const NotifyInfo &entry = *it;

        if (!IsWindow(entry._hWnd))
            if (_icon_map.erase(entry)) // delete icons without valid owner window
                ++update;
    }

    DWORD now = GetTickCount();

    // handle icon hiding
    for (NotifyIconMap::iterator it = _icon_map.begin(); it != _icon_map.end(); ++it) {
        NotifyInfo &entry = it->second;

        DetermineHideState(entry);

        switch (entry._mode) {
        case NIM_HIDE:
            if (!(entry._dwState & NIS_HIDDEN)) {
                entry._dwState |= NIS_HIDDEN;
                ++update;
            }
            break;

        case NIM_SHOW:
            if (entry._dwState & NIS_HIDDEN) {
                entry._dwState &= ~NIS_HIDDEN;
                ++update;
            }
            break;

        case NIM_AUTO:
            // automatically hide icons after long periods of inactivity
            if (_hide_inactive)
                if (!(entry._dwState & NIS_HIDDEN))
                    if (now - entry._lastChange > ICON_AUTOHIDE_SECONDS * 1000) {
                        entry._dwState |= NIS_HIDDEN;
                        ++update;
                    }
            break;
        }
    }

    if (update)
        UpdateIcons();
}

/// search for a icon at a given client coordinate position
NotifyIconSet::iterator NotifyArea::IconHitTest(const POINT &pos)
{
    if (pos.y < NOTIFYICON_Y || pos.y >= NOTIFYICON_Y + NOTIFY_HINT_Y)
        return _sorted_icons.end();

    NotifyIconSet::iterator it = _sorted_icons.begin();

    int x = NOTIFYICON_X;

    if (_show_button)
        x += NOTIFYICON_DIST;

    for (; it != _sorted_icons.end(); ++it) {
        //NotifyInfo& entry = const_cast<NotifyInfo&>(*it); // Why does GCC 3.3 need this additional const_cast ?!

        if (pos.x >= x && pos.x < x + NOTIFYICON_DIST)
            break;

        x += NOTIFYICON_DIST;
    }

    return it;
}


void NotifyIconConfig::create_name()
{
    _name = FmtString(TEXT("'%s' - '%s' - '%s'"), _tipText.c_str(), _windowTitle.c_str(), _modulePath.c_str());
}


#if NOTIFYICON_VERSION>=3   // as of 21.08.2003 missing in MinGW headers

bool NotifyIconConfig::match(const NotifyIconConfig &props) const
{
    if (!_tipText.empty() && !props._tipText.empty())
        if (props._tipText == _tipText)
            return true;

    if (!_windowTitle.empty() && !props._windowTitle.empty())
        if (_tcsstr(props._windowTitle, _windowTitle))
            return true;

    if (!_modulePath.empty() && !props._modulePath.empty())
        if (!_tcsicmp(props._modulePath, _modulePath))
            return true;

    return false;
}

bool NotifyArea::DetermineHideState(NotifyInfo &entry)
{
    if (entry._modulePath.empty()) {
        const String &modulePath = _window_modules[entry._hWnd];

        // request module path for new windows (We will get an asynchronous answer by a WM_COPYDATA message.)
        if (!modulePath.empty())
            entry._modulePath = modulePath;
#ifdef USE_NOTIFYHOOK
        else
            _hook.GetModulePath(entry._hWnd, _hwnd);
#endif
    }

    for (NotifyIconCfgList::const_iterator it = _cfg.begin(); it != _cfg.end(); ++it) {
        const NotifyIconConfig &cfg = *it;

        if (cfg.match(entry)) {
            entry._mode = cfg._mode;
            return true;
        }
    }

    return false;
}

#endif


String string_from_mode(NOTIFYICONMODE mode)
{
    switch (mode) {
    case NIM_SHOW:
        return ResString(IDS_NOTIFY_SHOW);

    case NIM_HIDE:
        return ResString(IDS_NOTIFY_HIDE);

    default:  //case NIM_AUTO
        return ResString(IDS_NOTIFY_AUTOHIDE);
    }
}


TrayNotifyDlg::TrayNotifyDlg(HWND hwnd)
    :  super(hwnd),
       _tree_ctrl(GetDlgItem(hwnd, IDC_NOTIFY_ICONS)),
       _himl(ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR24, 3, 0)),
       _pNotifyArea(static_cast<NotifyArea *>(Window::get_window((HWND)SendMessage(g_Globals._hwndDesktopBar, PM_GET_NOTIFYAREA, 0, 0))))
{
    _selectedItem = 0;

    if (_pNotifyArea) {
        // save original icon states and configuration data
        for (NotifyIconMap::const_iterator it = _pNotifyArea->_icon_map.begin(); it != _pNotifyArea->_icon_map.end(); ++it)
            _icon_states_org[it->first] = IconStatePair(it->second._mode, it->second._dwState);

        _cfg_org = _pNotifyArea->_cfg;
        _show_hidden_org = _pNotifyArea->_show_hidden;
    }

    SetWindowIcon(hwnd, IDI_WINXSHELL);

    _haccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_TRAYNOTIFY));

    {
        WindowCanvas canvas(_hwnd);
        HBRUSH hbkgnd = GetStockBrush(WHITE_BRUSH);

        ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT), hbkgnd, canvas);
        ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT_TRANS), hbkgnd, canvas);
        ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT_RED), hbkgnd, canvas);
    }

    (void)TreeView_SetImageList(_tree_ctrl, _himl, TVSIL_NORMAL);

    _resize_mgr.Add(IDC_NOTIFY_ICONS,   RESIZE);
    _resize_mgr.Add(IDC_LABEL1,         MOVE_Y);
    _resize_mgr.Add(IDC_NOTIFY_TOOLTIP, RESIZE_X | MOVE_Y);
    _resize_mgr.Add(IDC_LABEL2,         MOVE_Y);
    _resize_mgr.Add(IDC_NOTIFY_TITLE,   RESIZE_X | MOVE_Y);
    _resize_mgr.Add(IDC_LABEL3,         MOVE_Y);
    _resize_mgr.Add(IDC_NOTIFY_MODULE,  RESIZE_X | MOVE_Y);

    _resize_mgr.Add(IDC_LABEL4,         MOVE_Y);
    _resize_mgr.Add(IDC_NOTIFY_SHOW,    MOVE_Y);
    _resize_mgr.Add(IDC_NOTIFY_HIDE,    MOVE_Y);
    _resize_mgr.Add(IDC_NOTIFY_AUTOHIDE, MOVE_Y);

    _resize_mgr.Add(IDC_PICTURE,        MOVE);
    _resize_mgr.Add(ID_SHOW_HIDDEN_ICONS, MOVE_Y);

    _resize_mgr.Add(IDC_LABEL6,         MOVE_Y);
    _resize_mgr.Add(IDC_LAST_CHANGE,    MOVE_Y);

    _resize_mgr.Add(IDOK,               MOVE);
    _resize_mgr.Add(IDCANCEL,           MOVE);

    _resize_mgr.Resize(+150, +200);

    Refresh();

    SetTimer(_hwnd, 0, 3000, NULL);
    register_pretranslate(hwnd);
}

TrayNotifyDlg::~TrayNotifyDlg()
{
    KillTimer(_hwnd, 0);
    unregister_pretranslate(_hwnd);
    ImageList_Destroy(_himl);
}

void TrayNotifyDlg::Refresh()
{
    ///@todo refresh incrementally

    HiddenWindow hide(_tree_ctrl);

    TreeView_DeleteAllItems(_tree_ctrl);

    TV_INSERTSTRUCT tvi;

    tvi.hParent = 0;
    tvi.hInsertAfter = TVI_LAST;

    TV_ITEM &tv = tvi.item;
    tv.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

    ResString str_cur(IDS_ITEMS_CUR);
    tv.pszText = str_cur.str();
    tv.iSelectedImage = tv.iImage = 0;  // IDI_DOT
    _hitemCurrent = TreeView_InsertItem(_tree_ctrl, &tvi);

    ResString str_conf(IDS_ITEMS_CONFIGURED);
    tv.pszText = str_conf.str();
    tv.iSelectedImage = tv.iImage = 2;  // IDI_DOT_RED
    _hitemConfig = TreeView_InsertItem(_tree_ctrl, &tvi);

    tvi.hParent = _hitemCurrent;

    ResString str_visible(IDS_ITEMS_VISIBLE);
    tv.pszText = str_visible.str();
    tv.iSelectedImage = tv.iImage = 0;  // IDI_DOT
    _hitemCurrent_visible = TreeView_InsertItem(_tree_ctrl, &tvi);

    ResString str_hidden(IDS_ITEMS_HIDDEN);
    tv.pszText = str_hidden.str();
    tv.iSelectedImage = tv.iImage = 1;  // IDI_DOT_TRANS
    _hitemCurrent_hidden = TreeView_InsertItem(_tree_ctrl, &tvi);

    if (_pNotifyArea) {
        _info.clear();

        tv.mask |= TVIF_PARAM;

        WindowCanvas canvas(_hwnd);

        // insert current (visible and hidden) items
        for (NotifyIconMap::const_iterator it = _pNotifyArea->_icon_map.begin(); it != _pNotifyArea->_icon_map.end(); ++it) {
            const NotifyInfo &entry = it->second;

            InsertItem(entry._dwState & NIS_HIDDEN ? _hitemCurrent_hidden : _hitemCurrent_visible, TVI_LAST, entry, canvas);
        }

        // insert configured items in tree view
        const NotifyIconCfgList &cfg = _pNotifyArea->_cfg;
        for (NotifyIconCfgList::const_iterator it = cfg.begin(); it != cfg.end(); ++it) {
            const NotifyIconConfig &cfg_entry = *it;

            HICON hicon = 0;

            if (!cfg_entry._modulePath.empty()) {
                if ((int)ExtractIconEx(cfg_entry._modulePath, 0, NULL, &hicon, 1) <= 0)
                    hicon = 0;

                if (!hicon) {
                    SHFILEINFO sfi;

                    if (SHGetFileInfo(cfg_entry._modulePath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON))
                        hicon = sfi.hIcon;
                }
            }

            InsertItem(_hitemConfig, TVI_SORT, cfg_entry, canvas, hicon, cfg_entry._mode);

            if (hicon)
                DestroyIcon(hicon);
        }

        CheckDlgButton(_hwnd, ID_SHOW_HIDDEN_ICONS, _pNotifyArea->_show_hidden ? BST_CHECKED : BST_UNCHECKED);
    }

    TreeView_Expand(_tree_ctrl, _hitemCurrent_visible, TVE_EXPAND);
    TreeView_Expand(_tree_ctrl, _hitemCurrent_hidden, TVE_EXPAND);
    TreeView_Expand(_tree_ctrl, _hitemCurrent, TVE_EXPAND);
    TreeView_Expand(_tree_ctrl, _hitemConfig, TVE_EXPAND);

    TreeView_EnsureVisible(_tree_ctrl, _hitemCurrent_visible);
}

void TrayNotifyDlg::InsertItem(HTREEITEM hparent, HTREEITEM after, const NotifyInfo &entry, HDC hdc)
{
    InsertItem(hparent, after, entry, hdc, entry._hIcon, entry._mode);
}

void TrayNotifyDlg::InsertItem(HTREEITEM hparent, HTREEITEM after, const NotifyIconDlgInfo &entry,
                               HDC hdc, HICON hicon, NOTIFYICONMODE mode)
{
    UINT32 idx = (UINT32)_info.size() + 1;
    _info[idx] = entry;

    String mode_str = string_from_mode(mode);

    switch (mode) {
    case NIM_SHOW:    mode_str = ResString(IDS_NOTIFY_SHOW);      break;
    case NIM_HIDE:    mode_str = ResString(IDS_NOTIFY_HIDE);      break;
    case NIM_AUTO:    mode_str = ResString(IDS_NOTIFY_AUTOHIDE);
    }

    FmtString txt(TEXT("%s  -  %s  [%s]"), entry._tipText.c_str(), entry._windowTitle.c_str(), mode_str.c_str());

    TV_INSERTSTRUCT tvi;

    tvi.hParent = hparent;
    tvi.hInsertAfter = after;

    TV_ITEM &tv = tvi.item;
    tv.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

    tv.lParam = (LPARAM)idx;
    tv.pszText = txt.str();
    tv.iSelectedImage = tv.iImage = ImageList_AddAlphaIcon(_himl, hicon, GetStockBrush(WHITE_BRUSH), hdc);
    (void)TreeView_InsertItem(_tree_ctrl, &tvi);
}

LRESULT TrayNotifyDlg::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch (nmsg) {
    case PM_TRANSLATE_MSG: {
        MSG *pmsg = (MSG *) lparam;

        if (TranslateAccelerator(_hwnd, _haccel, pmsg))
            return TRUE;

        return FALSE;
    }

    case WM_TIMER:
        Refresh();
        break;

    default:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

int TrayNotifyDlg::Command(int id, int code)
{
    if (code == BN_CLICKED) {
        switch (id) {
        case ID_REFRESH:
            Refresh();
            break;

        case IDC_NOTIFY_SHOW:
            SetIconMode(NIM_SHOW);
            break;

        case IDC_NOTIFY_HIDE:
            SetIconMode(NIM_HIDE);
            break;

        case IDC_NOTIFY_AUTOHIDE:
            SetIconMode(NIM_AUTO);
            break;

        case ID_SHOW_HIDDEN_ICONS:
            if (_pNotifyArea)
                SendMessage(*_pNotifyArea, WM_COMMAND, MAKEWPARAM(id, code), 0);
            break;

        case IDOK:
            EndDialog(_hwnd, id);
            break;

        case IDCANCEL:
            // rollback changes
            if (_pNotifyArea) {
                // restore original icon states and configuration data
                _pNotifyArea->_cfg = _cfg_org;
                _pNotifyArea->_show_hidden = _show_hidden_org;

                for (IconStateMap::const_iterator it = _icon_states_org.begin(); it != _icon_states_org.end(); ++it) {
                    NotifyInfo &info = _pNotifyArea->_icon_map[it->first];

                    info._mode = it->second.first;
                    info._dwState = it->second.second;
                }

                SendMessage(*_pNotifyArea, PM_REFRESH, 0, 0);
            }

            EndDialog(_hwnd, id);
            break;
        }

        return 0;
    }

    return 1;
}

int TrayNotifyDlg::Notify(int id, NMHDR *pnmh)
{
    switch (pnmh->code) {
    case TVN_SELCHANGED: {
        NMTREEVIEW *pnmtv = (NMTREEVIEW *)pnmh;
        INT32 idx = (INT32)pnmtv->itemNew.lParam;

        if (idx) {
            RefreshProperties(_info[idx]);
            _selectedItem = pnmtv->itemNew.hItem;
        } else {
            /*
            SetDlgItemText(_hwnd, IDC_NOTIFY_TOOLTIP, NULL);
            SetDlgItemText(_hwnd, IDC_NOTIFY_TITLE, NULL);
            SetDlgItemText(_hwnd, IDC_NOTIFY_MODULE, NULL);
            */
            CheckRadioButton(_hwnd, IDC_NOTIFY_SHOW, IDC_NOTIFY_AUTOHIDE, 0);
        }
        break;
    }
    }

    return 0;
}

void TrayNotifyDlg::RefreshProperties(const NotifyIconDlgInfo &entry)
{
    SetDlgItemText(_hwnd, IDC_NOTIFY_TOOLTIP, entry._tipText);
    SetDlgItemText(_hwnd, IDC_NOTIFY_TITLE, entry._windowTitle);
    SetDlgItemText(_hwnd, IDC_NOTIFY_MODULE, entry._modulePath);

    CheckRadioButton(_hwnd, IDC_NOTIFY_SHOW, IDC_NOTIFY_AUTOHIDE, IDC_NOTIFY_SHOW + entry._mode);

    String change_str;
    if (entry._lastChange)
        change_str.printf(TEXT("before %d s"), (GetTickCount() - entry._lastChange + 500) / 1000);
    SetDlgItemText(_hwnd, IDC_LAST_CHANGE, change_str);

    HICON hicon = 0; //get_window_icon_big(entry._hWnd, false);

    // If we could not find an icon associated with the owner window, try to load one from the owning module.
    if (!hicon && !entry._modulePath.empty()) {
        hicon = ExtractIcon(g_Globals._hInstance, entry._modulePath, 0);

        if (!hicon) {
            SHFILEINFO sfi;

            if (SHGetFileInfo(entry._modulePath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
                hicon = sfi.hIcon;
        }
    }

    if (hicon) {
        SendMessage(GetDlgItem(_hwnd, IDC_PICTURE), STM_SETICON, (LPARAM)hicon, 0);
        DestroyIcon(hicon);
    } else
        SendMessage(GetDlgItem(_hwnd, IDC_PICTURE), STM_SETICON, 0, 0);
}

void TrayNotifyDlg::SetIconMode(NOTIFYICONMODE mode)
{
    UINT32 idx = (UINT32)TreeView_GetItemData(_tree_ctrl, _selectedItem);

    if (!idx)
        return;

    NotifyIconConfig &entry = _info[idx];

    if (entry._mode != mode) {
        entry._mode = mode;

        // trigger refresh in notify area and this dialog
        if (_pNotifyArea)
            SendMessage(*_pNotifyArea, PM_REFRESH, 0, 0);
    }

    if (_pNotifyArea) {
        bool found = false;

        NotifyIconCfgList &cfg = _pNotifyArea->_cfg;
        for (NotifyIconCfgList::iterator it = cfg.begin(); it != cfg.end(); ++it) {
            NotifyIconConfig &cfg_entry = *it;

            if (cfg_entry.match(entry)) {
                cfg_entry._mode = mode;
                ++found;
                break;
            }
        }

        if (!found) {
            // insert new configuration entry
            NotifyIconConfig cfg_entry = entry;

            cfg_entry._mode = mode;

            _pNotifyArea->_cfg.push_back(cfg_entry);
        }
    }

    Refresh();
    ///@todo select treeview item at new position in tree view -> refresh HTREEITEM in _selectedItem
}


ClockWindow::ClockWindow(HWND hwnd)
    :  super(hwnd),
       _tooltip(hwnd)
{
    *_time = TEXT('\0');
    FormatTime();

    _tooltip.add(_hwnd, _hwnd);
}

HWND ClockWindow::Create(HWND hwndParent)
{
    static BtnWindowClass wcClock(CLASSNAME_CLOCKWINDOW, CS_DBLCLKS);
    wcClock.hbrBackground = TASKBAR_BRUSH();
    ClientRect clnt(hwndParent);

    WindowCanvas canvas(hwndParent);
    FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

    RECT rect = {0, 0, 0, 0};
    TCHAR buffer[32];
    // Arbitrary high time so that the created clock window is big enough
    SYSTEMTIME st = { 1601, 1, 0, 1, 23, 59, 59, 999 };

    if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buffer, sizeof(buffer) / sizeof(TCHAR)))
        _tcscpy(buffer, TEXT("00:00\r\n2015/08/15"));
    else {
        _tcscat(buffer, TEXT("\r\n2015/08/15"));
    }

    // Calculate the rectangle needed to draw the time (without actually drawing it)
    DrawText(canvas, buffer, -1, &rect, DT_NOPREFIX | DT_CALCRECT);
    int clockwindowWidth = rect.right - rect.left + 20;

    return Window::Create(WINDOW_CREATOR(ClockWindow), 0,
                          wcClock, NULL, WS_CHILD | WS_VISIBLE,
                          clnt.right - clockwindowWidth, clnt.top + 1, clockwindowWidth, clnt.bottom - 2, hwndParent);
}

LRESULT ClockWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch (nmsg) {
    case WM_PAINT:
        Paint();
        break;

    case WM_LBUTTONDBLCLK:
        launch_cpanel(_hwnd, TEXT("timedate.cpl"));
        break;

    default:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

int ClockWindow::Notify(int id, NMHDR *pnmh)
{
    if (pnmh->code == TTN_GETDISPINFO) {
        LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

        SYSTEMTIME systime;
        TCHAR buffer[64];

        GetLocalTime(&systime);

        if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime, NULL, buffer, 64))
            _tcscpy(pdi->szText, buffer);
        else
            pdi->szText[0] = '\0';
    }

    return 0;
}

void ClockWindow::TimerTick()
{
    if (FormatTime())
        InvalidateRect(_hwnd, NULL, TRUE);  // refresh displayed time
}

bool ClockWindow::FormatTime()
{
    TCHAR buffer[64] = TEXT("");
    TCHAR time_buff[16];
    TCHAR date_buffer[64];

    if (!(GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL,
                        time_buff, sizeof(time_buff) / sizeof(TCHAR)))) return false;

    _tcscat(buffer, time_buff);

    if (!(GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL,
                        date_buffer, sizeof(date_buffer) / sizeof(TCHAR)))) return false;

    _tcscat(buffer, TEXT("\r\n"));
    _tcscat(buffer, date_buffer);

    if (_tcscmp(buffer, _time)) {
        _tcscpy(_time, buffer);
        return true;    // The text to display has changed.
    }
    return false; //no change
}

void ClockWindow::Paint()
{
    static bool inited = false;
    static RECT rc;

    PaintCanvas canvas(_hwnd);

    FillRect(canvas, &canvas.rcPaint, TASKBAR_BRUSH());

    BkMode bkmode(canvas, TRANSPARENT);
    FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));
    SetTextColor(canvas, CLOCK_TEXT_COLOR());
    if (!inited) {
        inited = true;
        rc = ClientRect(_hwnd);
        RECT rc_text = { 0, 0 };
        DrawText(canvas, _time, -1, &rc_text, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
        rc_text.right = DPI_SX(rc_text.right);
        rc_text.bottom = DPI_SY(rc_text.bottom);
        rc.top += (rc.bottom - rc_text.bottom) / 2;
    }

    DrawText(canvas, _time, -1, &rc, DT_CENTER | DT_NOPREFIX);
}
