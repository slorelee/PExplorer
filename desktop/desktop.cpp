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
// desktop.cpp
//
// Martin Fuchs, 09.08.2003
//


#include <precomp.h>
#include <olectl.h>
#include <ole2.h>
#include "../resource.h"

#include "../taskbar/desktopbar.h"
#include "../taskbar/taskbar.h" // for PM_GET_LAST_ACTIVE

enum WallPaperStyle {
    STYLE_WP_STRETCH = 0,
    STYLE_WP_TILE,
    STYLE_WP_CENTER
};


static BOOL (WINAPI *SetShellWindow)(HWND);
static BOOL (WINAPI *SetShellWindowEx)(HWND, HWND);



static BOOL CALLBACK SwitchDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
    WindowSet &windows = *(WindowSet *)lparam;

    if (hwnd != g_Globals._hwndDesktopBar && hwnd != g_Globals._hwndDesktop)
        if (IsWindowVisible(hwnd))
            windows.insert(hwnd);

    return TRUE;
}

//default ingore contextmenu,tooltips and Windows 10's Metro Window
#define DEF_IGNORE_WINDOW_TITLES TEXT("")
#define DEF_IGNORE_WINDOW_CLASSES TEXT(";#32768;tooltips_class32;ApplicationFrameWindow;Windows.UI.Core.CoreWindow")

static BOOL IsIgnoredWindow(HWND hwnd)
{
    static String ignore_window_titles = JCFG2_DEF("JS_HOTKEY", "IGNORE_WINDOW_TITLES", TEXT("")).ToString();
    static String ignore_window_classes = JCFG2_DEF("JS_HOTKEY", "IGNORE_WINDOW_CLASSES", TEXT("")).ToString();
    ignore_window_classes.append(DEF_IGNORE_WINDOW_CLASSES);
    TCHAR strbuffer[BUFFER_LEN] = { 0 };
    if (!GetWindowText(hwnd, strbuffer, BUFFER_LEN))
        strbuffer[0] = '\0';
    if (!ignore_window_titles.empty() && ignore_window_titles.find(strbuffer) != string_t::npos) {
        return TRUE;
    }
    LOG(strbuffer);
    if (!GetClassName(hwnd, strbuffer, BUFFER_LEN))
        strbuffer[0] = '\0';
    if (!ignore_window_classes.empty() && ignore_window_classes.find(strbuffer) != string_t::npos) {
        return TRUE;
    }
    LOG(strbuffer);
    return FALSE;
}

static BOOL CALLBACK MinimizeDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
    BOOL rc = FALSE;
    list<MinimizeStruct> &minimized = *(list<MinimizeStruct> *)lparam;

    if (hwnd == g_Globals._hwndDesktopBar || hwnd == g_Globals._hwndDesktop) return TRUE;
    if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
        if (IsIgnoredWindow(hwnd)) return TRUE;
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            if (rect.right > 0 && rect.bottom > 0 &&
                rect.right > rect.left && rect.bottom > rect.top) {
                minimized.push_back(MinimizeStruct(hwnd, GetWindowStyle(hwnd)));
                rc = ShowWindowAsync(hwnd, SW_SHOWMINIMIZED);
                if (rc == FALSE) {
                    PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);  //Some apps like TaskManager can't be minimized by ShowWindowAsync()
                }
            }
        }
    }

    return TRUE;
}

static BOOL CALLBACK UnminimizedDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
    BOOL rc = FALSE;
    list<MinimizeStruct> &unminimized = *(list<MinimizeStruct> *)lparam;

    if (hwnd == g_Globals._hwndDesktopBar || hwnd == g_Globals._hwndDesktop) return TRUE;
    if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
        if (IsIgnoredWindow(hwnd)) return TRUE;
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            if (rect.right > 0 && rect.bottom > 0 &&
                rect.right > rect.left && rect.bottom > rect.top) {
                unminimized.push_back(MinimizeStruct(hwnd, GetWindowStyle(hwnd)));
            }
        }
    }
    return TRUE;
}

// flag = SW_SHOWMINIMIZED or SW_SHOWNORMAL
static void ToggleWindows(list<MinimizeStruct> *windows, int flag, list<MinimizeStruct> *minimized = NULL)
{
    BOOL rc = FALSE;
    for (list<MinimizeStruct>::const_reverse_iterator it = windows->rbegin();
    it != windows->rend(); ++it) {
        if (flag == SW_SHOWMINIMIZED) {
            minimized->push_back(MinimizeStruct(it->first, GetWindowStyle(it->first)));
            rc = ShowWindowAsync(it->first, SW_SHOWMINIMIZED);
            if (rc == FALSE) {
                PostMessage(it->first, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            }
        } else {
            rc = ShowWindowAsync(it->first, it->second & WS_MAXIMIZE ? SW_MAXIMIZE : SW_RESTORE);
            if (rc == FALSE) {
                PostMessage(it->first, WM_SYSCOMMAND, it->second & WS_MAXIMIZE ? SC_MAXIMIZE : SC_RESTORE, 0);
            }
        }
        Sleep(20);
    }
    windows->clear();
}

// minimize/restore all windows on the desktop
void Desktop::ToggleMinimize()
{
    BOOL rc = FALSE;
    list<MinimizeStruct> &minimized = _minimized;
    if (minimized.empty()) {
        EnumWindows(MinimizeDesktopEnumFct, (LPARAM)&minimized);
    } else {
        list<MinimizeStruct> unminimized;
        EnumWindows(UnminimizedDesktopEnumFct, (LPARAM)&unminimized);
        if (!unminimized.empty()) {
            minimized.clear();
            ToggleWindows(&unminimized, SW_SHOWMINIMIZED, &minimized);
        } else {
            ToggleWindows(&minimized, SW_SHOWNORMAL);
        }
    }
}


BOOL IsAnyDesktopRunning()
{
    HINSTANCE hUser32 = GetModuleHandle(TEXT("user32"));

    SetShellWindow = (BOOL(WINAPI *)(HWND)) GetProcAddress(hUser32, "SetShellWindow");
    SetShellWindowEx = (BOOL(WINAPI *)(HWND, HWND)) GetProcAddress(hUser32, "SetShellWindowEx");

    return GetShellWindow() != 0;
}

/*
BackgroundWindow::BackgroundWindow(HWND hwnd)
 :  super(hwnd)
{
     // set background brush for the short moment of displaying the
     // background color while moving foreground windows
#ifdef _WIN64
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, COLOR_BACKGROUND+1);
#else
    SetClassLongPtr(hwnd, GCL_HBRBACKGROUND, COLOR_BACKGROUND+1);
#endif

    _display_version = RegGetDWORDValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), TEXT("PaintDesktopVersion"), 1);
}


LRESULT BackgroundWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch(nmsg) {
      case WM_ERASEBKGND:
        DrawDesktopBkgnd((HDC)wparam);
        return TRUE;

      case WM_MBUTTONDBLCLK:
        // Imagelist icons are missing if MainFrame::Create() is called directly from here!
        // explorer_show_frame(SW_SHOWNORMAL);
        PostMessage(g_Globals._hwndDesktop, nmsg, wparam, lparam);
        break;

      case PM_DISPLAY_VERSION:
        if (lparam || wparam) {
            DWORD or_mask = wparam;
            DWORD reset_mask = LOWORD(lparam);
            DWORD xor_mask = HIWORD(lparam);
            _display_version = ((_display_version&~reset_mask) | or_mask) ^ xor_mask;
            RegSetDWORDValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), TEXT("PaintDesktopVersion"), _display_version);
            ///@todo Changing the PaintDesktopVersion-Flag needs a restart of the shell -> display a message box
            InvalidateRect(_hwnd, NULL, TRUE);
        }
        return _display_version;

      default:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

void BackgroundWindow::DrawDesktopBkgnd(HDC hdc)
{
    PaintDesktop(hdc);
    return;

    //special solid background
    HBRUSH bkgndBrush = CreateSolidBrush(RGB(0,32,160));    // dark blue
    FillRect(hdc, &rect, bkgndBrush);
    DeleteBrush(bkgndBrush);

}
*/

DesktopWindow::DesktopWindow(HWND hwnd)
    :  super(hwnd)
{
    _pShellView = NULL;
    _hAccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_DESKTOP));
}

DesktopWindow::~DesktopWindow()
{
    RegisterHotkeys(TRUE);
    if (_pShellView)
        _pShellView->Release();
}


HWND DesktopWindow::Create()
{
    static IconWindowClass wcDesktop(TEXT("Progman"), IDI_PEXLORER, CS_DBLCLKS);
    /* (disabled because of small ugly temporary artefacts when hiding start menu)
    wcDesktop.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1); */
    wcDesktop.hbrBackground = CreateSolidBrush(DESKTOP_BKCOLOR());

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HWND hwndDesktop = Window::Create(WINDOW_CREATOR(DesktopWindow),
                                      WS_EX_TOOLWINDOW, wcDesktop, TEXT("Program Manager"), WS_POPUP | WS_VISIBLE, //|WS_CLIPCHILDREN for SDI frames
                                      0, 0, width, height, 0);

    // work around to display desktop bar in Wine
    ShowWindow(GET_WINDOW(DesktopWindow, hwndDesktop)->_desktopBar, SW_SHOW);

    // work around for Windows NT, Win 98, ...
    // Without this the desktop has mysteriously only a size of 800x600 pixels.
    MoveWindow(hwndDesktop, 0, 0, width, height, TRUE);

    return hwndDesktop;
}


LRESULT DesktopWindow::Init(LPCREATESTRUCT pcs)
{
    if (super::Init(pcs))
        return 1;

    HRESULT hr = GetDesktopFolder()->CreateViewObject(_hwnd, IID_IShellView, (void **)&_pShellView);
    /* also possible:
        SFV_CREATE sfv_create;

        sfv_create.cbSize = sizeof(SFV_CREATE);
        sfv_create.pshf = GetDesktopFolder();
        sfv_create.psvOuter = NULL;
        sfv_create.psfvcb = NULL;

        HRESULT hr = SHCreateShellFolderView(&sfv_create, &_pShellView);
    */
    HWND hWndView = 0;

    if (SUCCEEDED(hr)) {
        FOLDERSETTINGS fs;

        fs.ViewMode = FVM_ICON;
        fs.fFlags = FWF_DESKTOP | FWF_ALIGNLEFT | FWF_NOCLIENTEDGE | FWF_NOSCROLL | FWF_BESTFITWINDOW | FWF_SNAPTOGRID; //|FWF_AUTOARRANGE;
        /* PositionIcons() need remove FWF_SNAPTOGRID flag, but set the flag after the
           view be created need use IFolderView2 interface in windows vista or later. */
        ClientRect rect(_hwnd);

        hr = _pShellView->CreateViewWindow(NULL, &fs, this, &rect, &hWndView);

        ///@todo use IShellBrowser::GetViewStateStream() to restore previous view state -> see SHOpenRegStream()

        if (SUCCEEDED(hr)) {
            g_Globals._hwndShellView = hWndView;

            // subclass shellview window
            new DesktopShellView(hWndView, _pShellView);

            _pShellView->UIActivate(SVUIA_ACTIVATE_FOCUS);

            /*
                IShellView2* pShellView2;

                hr = _pShellView->QueryInterface(IID_IShellView2, (void**)&pShellView2);

                SV2CVW2_PARAMS params;
                params.cbSize = sizeof(SV2CVW2_PARAMS);
                params.psvPrev = _pShellView;
                params.pfs = &fs;
                params.psbOwner = this;
                params.prcView = &rect;
                params.pvid = params.pvid;//@@

                hr = pShellView2->CreateViewWindow2(&params);
                params.pvid;
            */

            /*
                IFolderView* pFolderView;

                hr = _pShellView->QueryInterface(IID_IFolderView, (void**)&pFolderView);

                if (SUCCEEDED(hr)) {
                    hr = pFolderView->GetAutoArrange();
                    hr = pFolderView->SetCurrentViewMode(FVM_DETAILS);
                }
            */
        }
    }

    if (JCFG2_DEF("JS_DESKTOP", "shellwindow", true).ToBool() != FALSE) {
        if (hWndView && SetShellWindowEx)
            SetShellWindowEx(_hwnd, hWndView);
        else if (SetShellWindow)
            SetShellWindow(_hwnd);
    }

    // create the explorer bar
    if (JCFG_TB(2, "notaskbar").ToBool() == FALSE) {
        _desktopBar = DesktopBar::Create();
        g_Globals._hwndDesktopBar = _desktopBar;
    }

    RegisterHotkeys();
    return 0;
}


#define AUTOREGISTERHOTKEY(unreg, hwnd, id,fsModifiers, vk)\
  if (!unreg) RegisterHotKey(hwnd, id,fsModifiers, vk);\
  else UnregisterHotKey(hwnd, id);

void DesktopWindow::RegisterHotkeys(BOOL unreg)
{
    // register hotkey WIN+E opening explorer
    AUTOREGISTERHOTKEY(unreg, _hwnd, IDHK_EXPLORER, MOD_WIN, 'E');
    AUTOREGISTERHOTKEY(unreg, _hwnd, IDHK_RUN, MOD_WIN, 'R');
    AUTOREGISTERHOTKEY(unreg, _hwnd, IDHK_LOGOFF, MOD_WIN, 'L');
    if (g_Globals._hwndDesktopBar == (HWND)0) {
        AUTOREGISTERHOTKEY(unreg, _hwnd, IDHK_DESKTOP, MOD_WIN, 'D');
    }
    ///@todo register all common hotkeys
}

void DesktopWindow::ProcessHotKey(int id_hotkey)
{
    switch (id_hotkey) {
    case IDHK_EXPLORER:
        explorer_open_frame(SW_SHOWNORMAL);
        break;

    case IDHK_RUN:
        ShowLaunchDialog(_hwnd);
        break;

    case IDHK_LOGOFF:
         ShowLogoffDialog(_hwnd);
        break;

    case IDHK_DESKTOP:
         g_Globals._desktop.ToggleMinimize();
        break;
    //@todo implement all common hotkeys
    }
}

int VK_WIN_HOOK()
{
    static String winhotkey = JCFG2_DEF("JS_HOTKEY", "WIN", TEXT("")).ToString();
    if (winhotkey.empty()) return 0;

    winhotkey.toUpper();
    String key1, key2;
    size_t pos = winhotkey.find(TEXT("+"));
    if (pos == string_t::npos) return -1;
    key1 = winhotkey.substr(0, pos);
    key2 = winhotkey.substr(pos + 1, winhotkey.length() - pos - 1);

    BYTE bVkey1 = 0;
    if (key1 == TEXT("WIN")) {
        bVkey1 = VK_LWIN;
    } else if (key1 == TEXT("SHIFT")) {
        bVkey1 = VK_SHIFT;
    } else if (key1 == TEXT("CTRL")) {
        bVkey1 = VK_CONTROL;
    } else if (key1 == TEXT("ALT")) {
        bVkey1 = VK_MENU;
    }
    BYTE bVkey2 = 0;
    if (key2.length() >= 2 && key2.at(0) == TEXT('F')) {
        bVkey2 = VK_F1 + _wtoi(key2.substr(1).c_str());
        if (bVkey2 < VK_F1 || bVkey2 > VK_F12) bVkey2 = 0;
    } else {
        bVkey2 = (BYTE)(key2.at(0));
        if (bVkey2 < TEXT('A') || bVkey2 > TEXT('Z')) bVkey2 = 0;
    }
    if (bVkey1 == 0 || bVkey2 == 0) return -1;

    keybd_event(bVkey1, 0, 0, 0);
    keybd_event(bVkey2, 0, 0, 0);
    keybd_event(bVkey2, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(bVkey1, 0, KEYEVENTF_KEYUP, 0);
    return 1;
}

LRESULT DesktopWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch (nmsg) {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        explorer_show_frame(SW_SHOWNORMAL);
        break;

    case WM_HOTKEY:
        ProcessHotKey((int)wparam);
        break;

    case WM_DISPLAYCHANGE:
        MoveWindow(_hwnd, 0, 0, LOWORD(lparam), HIWORD(lparam), TRUE);
        MoveWindow(g_Globals._hwndShellView, 0, 0, LOWORD(lparam), HIWORD(lparam), TRUE);
        MoveWindow(_desktopBar, 0, HIWORD(lparam) - DESKTOPBARBAR_HEIGHT, LOWORD(lparam), DESKTOPBARBAR_HEIGHT, TRUE);
        SendMessage(g_Globals._hwndShellView, WM_DISPLAYCHANGE, LOWORD(lparam), HIWORD(lparam));
        break;

    case WM_GETISHELLBROWSER:
        return (LRESULT)static_cast<IShellBrowser *>(this);

    case WM_DESTROY:

        ///@todo use IShellBrowser::GetViewStateStream() and _pShellView->SaveViewState() to store view state

        if (SetShellWindow)
            SetShellWindow(0);
        break;

    case WM_CLOSE:
        ShowExitWindowsDialog(_hwnd);
        break;

    case WM_SYSCOMMAND:
        if (wparam == SC_TASKLIST) {
            if (VK_WIN_HOOK() == 0 && _desktopBar) {
                SendMessage(_desktopBar, nmsg, wparam, lparam);
            }
        }
        goto def;

    case WM_SYSCOLORCHANGE:
        // redraw background window - it's done by system
        //InvalidateRect(g_Globals._hwndShellView, NULL, TRUE);

        // forward message to common controls
        SendMessage(g_Globals._hwndShellView, WM_SYSCOLORCHANGE, 0, 0);
        SendMessage(_desktopBar, WM_SYSCOLORCHANGE, 0, 0);
        break;

    case WM_SETTINGCHANGE:
        SendMessage(g_Globals._hwndShellView, nmsg, wparam, lparam);
        break;

    case PM_TRANSLATE_MSG: {
        /* TranslateAccelerator is called for all explorer windows that are open
           so we have to decide if this is the correct recipient */
        LPMSG lpmsg = (LPMSG)lparam;
        HWND hwnd = lpmsg->hwnd;

        while (hwnd) {
            if (hwnd == _hwnd)
                break;

            hwnd = GetParent(hwnd);
        }

        if (hwnd) {
            int rc = TranslateAccelerator(g_Globals._hwndShellView, _hAccel, lpmsg);
            if (rc != 0) return true;
            return _pShellView->TranslateAccelerator(lpmsg) == S_OK;
        }
        return false;
    }

default: def:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}


HRESULT DesktopWindow::OnDefaultCommand(LPIDA pida)
{
#ifndef ROSSHELL    // in shell-only-mode fall through and let shell32 handle the command
    if (MainFrameBase::OpenShellFolders(pida, 0))
        return S_OK;
#endif

    return E_NOTIMPL;
}

#define ICON_ALGORITHM_DEF 0
#define ID_TIMER_ADJUST_ICONPOSITION 815

DesktopShellView::DesktopShellView(HWND hwnd, IShellView *pShellView)
    :  super(hwnd),
       _pShellView(pShellView)
{
    _hwndListView = GetNextWindow(hwnd, GW_CHILD);
    ShowWindow(_hwndListView, SW_HIDE);
    // work around for Windows NT, Win 98, ...
    // Without this the desktop has mysteriously only a size of 800x600 pixels.
    ClientRect rect(hwnd);
    MoveWindow(_hwndListView, 0, 0, rect.right, rect.bottom, TRUE);

    // subclass background window
    //new BackgroundWindow(_hwndListView);


    //refresh();
    InitDragDrop();

    _hbmWallp = NULL;
    _hbrWallp = NULL;
    SetRect(&_rcWp, 0, 0, 0, 0);
    SetRect(&_rcBitmapWp, 0, 0, 0, 0);

    LoadWallpaper(TRUE);
    _icon_algo = ICON_ALGORITHM_DEF;    // default icon arrangement (top/left)
    //SetTimer(_hwnd, ID_TIMER_ADJUST_ICONPOSITION, 1000, NULL);

}


DesktopShellView::~DesktopShellView()
{
    if (_hbmWallp) {
        DeleteObject(_hbmWallp);
        _hbmWallp = NULL;
    }

    if (_hbrWallp) {
        DeleteObject(_hbrWallp);
        _hbrWallp = NULL;
    }
}


bool DesktopShellView::InitDragDrop()
{
    CONTEXT("DesktopShellView::InitDragDrop()");
    IDropTarget *dt;
    HRESULT hr = _pShellView->QueryInterface(IID_IDropTarget, (void **)&dt);
    if (SUCCEEDED(hr)) {
        RevokeDragDrop(_hwndListView); // just in case
        RegisterDragDrop(_hwndListView, dt);
        return true;
    }
    return false;
}


void DesktopShellView::Refresh()
{
    _pShellView->Refresh();
}

// Function LoadAnImage: accepts a file name(JPG/GIF/BMP) and returns a HBITMAP.
// On error, it returns 0.

static HBITMAP LoadAnImage(LPCTSTR szFileName)
{
    // Use IPicture stuff to use JPG / GIF files
    IPicture *pIPic = NULL;
    IStream *pIStream = NULL;
    HGLOBAL hGB;
    HANDLE hFile;
    DWORD dwFileSize, dwRead;

    // Read file in memory
    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    dwFileSize = GetFileSize(hFile, NULL);
    hGB = GlobalAlloc(GPTR, dwFileSize);
    if (!hGB) {
        CloseHandle(hFile);
        return NULL;
    }
    ReadFile(hFile, (LPVOID)hGB, dwFileSize, &dwRead, NULL);
    CloseHandle(hFile);

    CreateStreamOnHGlobal(hGB, false, &pIStream);
    if (!pIStream) {
        GlobalFree(hGB);
        return NULL;
    }

    OleLoadPicture(pIStream, 0, false, IID_IPicture, (void**)&pIPic);

    if (!pIPic) {
        pIStream->Release();
        GlobalFree(hGB);
        return NULL;
    }
    pIStream->Release();
    GlobalFree(hGB);

    HBITMAP hbmp = 0;
    pIPic->get_Handle((unsigned int*)&hbmp);

    // Copy the image. Necessary, because upon pIPic's release,
    // the handle is destroyed.
    HBITMAP hretbmp = (HBITMAP)CopyImage(hbmp, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
    pIPic->Release();
    return hretbmp;
}

HBITMAP
DesktopShellView::SHLoadDIBitmap(LPCTSTR szFileName, int *pnWidth, int *pnHeight)
{
    HBITMAP hbmp = LoadAnImage(szFileName);

    if (hbmp){
        BITMAP bmp = {0};
        GetObject(hbmp, sizeof(BITMAP), (LPBYTE)&bmp);
        *pnWidth = bmp.bmWidth;
        *pnHeight = bmp.bmHeight;
    }
    return hbmp;
}


static
HBITMAP GetSrcBit(HDC hDC, DWORD dstBitWidth, DWORD dstBitHeight, DWORD srcBitWidth, DWORD srcBitHeight)
{
    HBITMAP hbmp = CreateCompatibleBitmap(hDC, dstBitWidth, dstBitHeight);
    MemCanvas dstCanvas(hDC);
    BitmapSelection bmpSel(dstCanvas, hbmp);
    SetStretchBltMode(dstCanvas, HALFTONE);
    SetBrushOrgEx(dstCanvas, 0, 0, NULL);
    BOOL ret = StretchBlt(dstCanvas, 0, 0, dstBitWidth, dstBitHeight, hDC, 0, 0, srcBitWidth, srcBitHeight, SRCCOPY);
    return hbmp;
}

HBITMAP
DesktopShellView::StretchWallpaper()
{
    if (!_hbmWallp) return _hbmWallp;

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    if (_fStyleWallp == STYLE_WP_CENTER) {
        _rcWp.left = max(0, (width - _rcBitmapWp.right) / 2);
        _rcWp.top = max(0, (height - _rcBitmapWp.bottom) / 2);
        _rcWp.right = _rcWp.left + _rcBitmapWp.right;
        _rcWp.bottom = _rcWp.top + _rcBitmapWp.bottom;
        return _hbmWallp;
    }

    if (_fStyleWallp != STYLE_WP_STRETCH) {
        return _hbmWallp;
    }

    MemCanvas srcCanvas;
    BitmapSelection srcBmp(srcCanvas, _hbmWallp);

    HBITMAP hbmp = GetSrcBit((HDC)srcCanvas, width, height, _rcBitmapWp.right, _rcBitmapWp.bottom);
    if (hbmp) {
        return hbmp;
    }
    return _hbmWallp;
}

LRESULT DesktopShellView::LoadWallpaper(BOOL fInitial)
{
    if (fInitial) {
        if (_hbmWallp) {
            DeleteObject(_hbmWallp);
            _hbmWallp = NULL;
        }
        if (_hbrWallp) {
            DeleteObject(_hbrWallp);
            _hbrWallp = NULL;
        }
        SetRect(&_rcWp, 0, 0, 0, 0);
        SetRect(&_rcBitmapWp, 0, 0, 0, 0);

        String wallpaper_path = JCFG2("JS_DESKTOP", "wallpaper").ToString();
        _fStyleWallp = JCFG2("JS_DESKTOP", "wallpaperstyle").ToInt();
        ExpandEnvironmentStrings(wallpaper_path, _szBMPName, MAX_PATH);
        int x, y;
        _hbmWallp = SHLoadDIBitmap(_szBMPName, &x, &y);
        if (_hbmWallp) {
            _rcBitmapWp.right = x;
            _rcBitmapWp.bottom = y;
        }
    }

    // need to repaint whole thing, so invalidate entire desktop window
    //InvalidateRect(_hwnd, NULL, TRUE);

    if (_hbmWallp) {
        HBITMAP hbmp = StretchWallpaper();
        if (_hbrWallp) DeleteObject(_hbrWallp);
        _hbrWallp = CreatePatternBrush(hbmp);
        if (hbmp != _hbmWallp) DeleteObject(hbmp);
    }

    return TRUE;
}

void DesktopShellView::DrawDesktopBkgnd(HDC hdc)
{
    RECT rc;
    GetClipBox(hdc, &rc);

    if (!_hbrWallp || _fStyleWallp == STYLE_WP_CENTER) {
        HBRUSH hBkBrush = CreateSolidBrush(DESKTOP_BKCOLOR());
        FillRect(hdc, &rc, hBkBrush);
        DeleteObject(hBkBrush);
    }

    if (_hbrWallp) {
        if (_fStyleWallp == STYLE_WP_CENTER) {
            if ((rc.right >= _rcWp.left && rc.bottom >= _rcWp.top) &&
                (rc.left <= _rcWp.right && rc.top <= _rcWp.bottom)) {
                SetBrushOrgEx(hdc, _rcWp.left - rc.left, _rcWp.top - rc.top, NULL);
                FillRect(hdc, &_rcWp, _hbrWallp);
            } 
        } else {
            SetBrushOrgEx(hdc, 0 - rc.left, 0 - rc.top, NULL);
            FillRect(hdc, &rc, _hbrWallp);
        }
    }

}

LRESULT DesktopShellView::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch (nmsg) {
    case WM_TIMER: {
        UINT tid = (UINT)wparam;
        if (tid == ID_TIMER_ADJUST_ICONPOSITION) {
            KillTimer(_hwnd, tid);
            if (_hwndListView) {
                PositionIcons();
                return 0;
            }
        }
        return super::WndProc(nmsg, wparam, lparam);
    }
    case WM_SETTINGCHANGE: {
        RECT work_area;
        //UINT nWorkArea;
        //ListView_GetNumberOfWorkAreas(_hwndListView, &nWorkArea);
        SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
        ListView_SetWorkAreas(_hwndListView, 1, &work_area);
        break;
    }
    case WM_DISPLAYCHANGE:
        LoadWallpaper(FALSE);
        break;
    case WM_ERASEBKGND:
        DrawDesktopBkgnd((HDC)wparam);
        break;
    case WM_CONTEXTMENU:
        if (!DoContextMenu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
            DoDesktopContextMenu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        break;

    case PM_SET_ICON_ALGORITHM:
        _icon_algo = (int)wparam;
        PositionIcons();
        break;

    case PM_GET_ICON_ALGORITHM:
        return _icon_algo;

    case WM_MBUTTONDBLCLK:
        /* Imagelist icons are missing if MainFrame::Create() is called directly from here!
        explorer_show_frame(SW_SHOWNORMAL); */
        PostMessage(g_Globals._hwndDesktop, nmsg, wparam, lparam);
        break;
    default:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

int DesktopShellView::Command(int id, int code)
{
    switch (id) {
    case ID_REFRESH:
        Refresh();
        break;
    default:
        return super::Command(id, code);
    }

    return 0;
}

int DesktopShellView::Notify(int id, NMHDR *pnmh)
{
    return super::Notify(id, pnmh);
}

bool DesktopShellView::DoContextMenu(int x, int y)
{
    IDataObject *selection;

    HRESULT hr = _pShellView->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (void **)&selection);
    if (FAILED(hr))
        return false;

    PIDList pidList;

    hr = pidList.GetData(selection);
    if (FAILED(hr)) {
        selection->Release();
#ifdef _DEBUG
        CHECKERROR(hr);
#endif
        return false;
    }

    LPIDA pida = pidList;
    if (!pida->cidl) {
        selection->Release();
        return false;
    }

    LPCITEMIDLIST parent_pidl = (LPCITEMIDLIST)((LPBYTE)pida + pida->aoffset[0]);

    LPCITEMIDLIST *apidl = (LPCITEMIDLIST *) alloca(pida->cidl * sizeof(LPCITEMIDLIST));

    for (int i = pida->cidl; i > 0; --i)
        apidl[i - 1] = (LPCITEMIDLIST)((LPBYTE)pida + pida->aoffset[i]);

    hr = ShellFolderContextMenu(ShellFolder(parent_pidl), _hwnd, pida->cidl, apidl, x, y, _cm_ifs, _pShellView);

    selection->Release();

    if (SUCCEEDED(hr)) {
        //refresh();
    } else {
#ifdef _DEBUG
        CHECKERROR(hr);
#endif
    }

    return true;
}

static HRESULT DoInvokeCommand(HWND hwnd, IContextMenu *pcm, UINT idCmd)
{
    CMINVOKECOMMANDINFO cmi = { 0 };
    cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
    cmi.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
    if (GetKeyState(VK_CONTROL) < 0) cmi.fMask |= CMIC_MASK_CONTROL_DOWN;
    if (GetKeyState(VK_SHIFT) < 0) cmi.fMask |= CMIC_MASK_SHIFT_DOWN;
    cmi.hwnd = hwnd;
    cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - FCIDM_SHVIEWFIRST);
    cmi.nShow = SW_SHOWNORMAL;

    HRESULT hr = pcm->InvokeCommand(&cmi);
    return hr;
}

HRESULT DesktopShellView::DoDesktopContextMenu(int x, int y)
{
    IContextMenu *pcm;

    HRESULT hr = _pShellView->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, (LPVOID *)&pcm);

    if (SUCCEEDED(hr)) {
        pcm = _cm_ifs.query_interfaces(pcm);

        HMENU hmenu = CreatePopupMenu();

        if (hmenu) {
            hr = pcm->QueryContextMenu(hmenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST - 1, CMF_NORMAL | CMF_EXPLORE | CMF_EXTENDEDVERBS);

            if (SUCCEEDED(hr)) {
                SetMenuDefaultItem(hmenu, -1, FALSE);
                AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hmenu, 0, FCIDM_SHVIEWLAST - 1, ResString(IDS_ABOUT_EXPLORER));

                UINT idCmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, x, y, 0, _hwnd, NULL);

                _cm_ifs.reset();

                if (idCmd == FCIDM_SHVIEWLAST - 1) {
                    explorer_about(_hwnd);
                } else if (idCmd) {
                    String menuname;
                    WCHAR namebuffer[MAX_PATH + 1] = { 0 };
                    pcm->GetCommandString(idCmd, GCS_VERBW, NULL, (char *)namebuffer, MAX_PATH);
                    if (_wcsicmp(namebuffer, TEXT("")) == 0) {
                        GetMenuString(hmenu, idCmd, namebuffer, MAX_PATH, MF_BYCOMMAND);
                        menuname = namebuffer;
                    }

                    if (_wcsicmp(namebuffer, L"cmd") == 0 || menuname == JCFG_VMN("cmdhere")) {
                        static TCHAR sDesktopPath[MAX_PATH + 1];
                        String parameters;
                        SHGetSpecialFolderPath(0, sDesktopPath, CSIDL_DESKTOPDIRECTORY, FALSE);
                        parameters.printf(JCFG_VMC("cmdhere", "parameters").ToString().c_str(), sDesktopPath);
                        launch_file(g_Globals._hwndShellView, JCFG_VMC("cmdhere", "command").ToString().c_str(), SW_SHOWNORMAL, parameters);
                        //} else if (_wcsicmp(namebuffer, L"refresh") == 0 || menuname == JCFG_VMN("refresh")) {
                        //	DoInvokeCommand(_hwnd, pcm, idCmd);
                        //	SetTimer(_hwnd, ID_TIMER_ADJUST_ICONPOSITION, 100, NULL);
                    } else {
                        DoInvokeCommand(_hwnd, pcm, idCmd);
                    }
                }
            } else
                _cm_ifs.reset();
            DestroyMenu(hmenu);
        }

        pcm->Release();
    }

    return hr;
}


#define ARRANGE_BORDER_DOWN  8
#define ARRANGE_BORDER_HV    9
#define ARRANGE_ROUNDABOUT  10

static const POINTS s_align_start[] = {
    {0, 0}, // left/top
    {0, 0},
    {1, 0}, // right/top
    {1, 0},
    {0, 1}, // left/bottom
    {0, 1},
    {1, 1}, // right/bottom
    {1, 1},

    {0, 0}, // left/top
    {0, 0},
    {0, 0}
};

static const POINTS s_align_dir1[] = {
    { 0, +1},   // down
    { +1,  0},  // right
    { -1,  0},  // left
    { 0, +1},   // down
    { 0, -1},   // up
    { +1,  0},  // right
    { -1,  0},  // left
    { 0, -1},   // up

    { 0, +1},   // down
    { +1,  0},  // right
    { +1,  0}   // right
};

static const POINTS s_align_dir2[] = {
    { +1,  0},  // right
    { 0, +1},   // down
    { 0, +1},   // down
    { -1,  0},  // left
    { +1,  0},  // right
    { 0, -1},   // up
    { 0, -1},   // up
    { -1,  0},  // left

    { +1,  0},  // right
    { 0, +1},   // down
    { 0, +1}    // down
};

typedef pair<int, int> IconPos;
typedef map<IconPos, int> IconMap;

void DesktopShellView::PositionIcons(int dir)
{
    DWORD spacing = ListView_GetItemSpacing(_hwndListView, FALSE);

    RECT work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);

    const POINTS &dir1 = s_align_dir1[_icon_algo];
    const POINTS &dir2 = s_align_dir2[_icon_algo];
    const POINTS &start_pos = s_align_start[_icon_algo];

    int dir_x1 = dir1.x;
    int dir_y1 = dir1.y;
    int dir_x2 = dir2.x;
    int dir_y2 = dir2.y;

    int cx = LOWORD(spacing);
    int cy = HIWORD(spacing);

    int dx1 = dir_x1 * cx;
    int dy1 = dir_y1 * cy;
    int dx2 = dir_x2 * cx;
    int dy2 = dir_y2 * cy;

    int xoffset = (cx - GetSystemMetrics(SM_CXICON)) / 2;
    int yoffset = (cy - GetSystemMetrics(SM_CYICON)) / 2;

    int start_x = start_pos.x * (work_area.right - cx) + xoffset;
    int start_y = start_pos.y * (work_area.bottom - cy) + yoffset;

    int x = start_x;
    int y = start_y;

    int all = ListView_GetItemCount(_hwndListView);
    if (all == 0) return; //no icon


    /* disable default allignment */
    // SetWindowStyle(_hwndListView, GetWindowStyle(_hwndListView)&~LVS_ALIGNMASK);//|LVS_ALIGNTOP|LVS_AUTOARRANGE);
    /* remove LVS_EX_SNAPTOGRID externded style before call SetItemPosition32 */
    DWORD ext_style = ListView_GetExtendedListViewStyle(_hwndListView);

    BOOL n = ext_style & LVS_EX_SNAPTOGRID;
    if ((ext_style & LVS_EX_SNAPTOGRID) == LVS_EX_SNAPTOGRID) {
        ListView_SetExtendedListViewStyle(_hwndListView, ext_style & ~LVS_EX_SNAPTOGRID);
    }

    int i1, i2;

    if (dir > 0) {
        i1 = 0;
        i2 = all;
    } else {
        i1 = all - 1;
        i2 = -1;
    }

    IconMap pos_idx;
    int cnt = 0;
    int xhv = start_x;
    int yhv = start_y;

    for (int idx = i1; idx != i2; idx += dir) {
        pos_idx[IconPos(y, x)] = idx;

        if (_icon_algo == ARRANGE_BORDER_DOWN) {
            if (++cnt & 1)
                x = work_area.right - x - cx + 2 * xoffset;
            else {
                y += dy1;

                if (y + cy - 2 * yoffset > work_area.bottom) {
                    y = start_y;
                    start_x += dx2;
                    x = start_x;
                }
            }

            continue;
        } else if (_icon_algo == ARRANGE_BORDER_HV) {
            if (++cnt & 1)
                x = work_area.right - x - cx + 2 * xoffset;
            else if (cnt & 2) {
                yhv += cy;
                y = yhv;
                x = start_x;

                if (y + cy - 2 * yoffset > work_area.bottom) {
                    start_x += cx;
                    xhv = start_x;
                    x = xhv;
                    start_y += cy;
                    yhv = start_y;
                    y = yhv;
                }
            } else {
                xhv += cx;
                x = xhv;
                y = start_y;

                if (x + cx - 2 * xoffset > work_area.right) {
                    start_x += cx;
                    xhv = start_x;
                    x = xhv;
                    start_y += cy;
                    yhv = start_y;
                    y = yhv;
                }
            }

            continue;
        } else if (_icon_algo == ARRANGE_ROUNDABOUT) {

            ///@todo

        }

        x += dx1;
        y += dy1;

        if (x < 0 || x + cx - 2 * xoffset > work_area.right) {
            x = start_x;
            y += dy2;
        } else if (y < 0 || y + cy - 2 * yoffset > work_area.bottom) {
            y = start_y;
            x += dx2;
        }
    }

    for (IconMap::const_iterator it = pos_idx.begin(); it != pos_idx.end(); ++it) {
        const IconPos &pos = it->first;

        ListView_SetItemPosition32(_hwndListView, it->second, pos.second, pos.first);
    }

    if ((ext_style & LVS_EX_SNAPTOGRID) == LVS_EX_SNAPTOGRID) {
        ListView_SetExtendedListViewStyle(_hwndListView, ext_style);
    }
    //ListView_RedrawItems(_hwndListView, 0,all - 1);
    //UpdateWindow(_hwndListView);
}
