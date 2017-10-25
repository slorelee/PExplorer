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
// desktopbar.cpp
//
// Martin Fuchs, 22.08.2003
//


#include <precomp.h>

#include "../resource.h"

#include "desktopbar.h"
#include "taskbar.h"
#include "startmenu.h"
#include "traynotify.h"
#include "quicklaunch.h"

#include "../dialogs/settings.h"


DesktopBar::DesktopBar(HWND hwnd)
    :  super(hwnd),
    _traySndVolIcon(hwnd, ID_TRAY_VOLUME),
    _trayNetworkIcon(hwnd, ID_TRAY_NETWORK)
{
    SetWindowIcon(hwnd, IDI_WINXSHELL);

    SystemParametersInfo(SPI_GETWORKAREA, 0, &_work_area_org, 0);
}

DesktopBar::~DesktopBar()
{
    if (_hbmQuickLaunchBack) DeleteObject(_hbmQuickLaunchBack);
    RegisterHotkeys(TRUE);
    // restore work area to the previous size
    SystemParametersInfo(SPI_SETWORKAREA, 0, &_work_area_org, 0);
    PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETWORKAREA, 0);

    // exit application after destroying desktop window
    PostQuitMessage(0);
}

#define DESKTOPBAR_TOP GetSystemMetrics(SM_CYSCREEN) - DESKTOPBARBAR_HEIGHT
HWND DesktopBar::Create()
{
    static BtnWindowClass wcDesktopBar(CLASSNAME_EXPLORERBAR);
    wcDesktopBar.hbrBackground = TASKBAR_BRUSH();

    RECT rect;

    rect.left = 0;
#ifdef TASKBAR_AT_TOP
    rect.top = 0;
#else
    rect.top = DESKTOPBAR_TOP;
#endif
    rect.right = GetSystemMetrics(SM_CXSCREEN);
    return Window::Create(WINDOW_CREATOR(DesktopBar), WS_EX_PALETTEWINDOW,
                          wcDesktopBar, TITLE_EXPLORERBAR,
                          WS_POPUP | WS_CLIPCHILDREN | WS_VISIBLE,
                          rect.left, rect.top, rect.right - rect.left, DESKTOPBARBAR_HEIGHT, 0);
}

static HBITMAP
CreateSolidBitmap(HDC hdc, int width, int height, COLORREF cref)
{
    // Create compatible memory DC and bitmap, and a solid brush
    MemCanvas canvas;
    HBITMAP hbmp = CreateCompatibleBitmap(hdc, width, height);
    HBRUSH hbrushFill = CreateSolidBrush(cref);
    BitmapSelection sel(canvas, hbmp);

    // Fill the whole area with solid color
    RECT rect = { 0, 0, width, height };
    FillRect(canvas, &rect, hbrushFill);

    // Delete brush
    DeleteObject(hbrushFill);

    // Set preferred dimensions
    SetBitmapDimensionEx(hbmp, width, height, NULL);

    return hbmp;
}

LRESULT DesktopBar::Init(LPCREATESTRUCT pcs)
{
    if (super::Init(pcs))
        return 1;

    // create start button
    string_t start_str(JCFG2("JS_STARTMENU", "text").ToString());
    WindowCanvas canvas(_hwnd);
    FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));
    RECT rect = {0, 0};
    DrawText(canvas, start_str.c_str(), -1, &rect, DT_SINGLELINE | DT_CALCRECT);

    _deskbar_pos_y = DESKTOPBAR_TOP;
    int start_btn_width = TASKBAR_ICON_SIZE + DPI_SX(rect.right) + (TASKBAR_ICON_SIZE / 4);

    int start_btn_padding = 0;
    BOOL isEmptyStartIcon = JCFG2_DEF("JS_STARTMENU", "start_icon", TEXT("")).ToString().compare(TEXT("empty")) == 0;
    if (isEmptyStartIcon) {
        start_btn_padding = JCFG2_DEF("JS_STARTMENU", "start_padding", 0).ToInt();
    }
    _taskbar_pos = start_btn_width + DPI_SX(start_btn_padding) + 1;
    // create "Start" button
    static WNDCLASS wc;
    GetClassInfo(NULL, TEXT("BUTTON"), &wc);
    wc.lpszClassName = TEXT("Start");
    wc.hInstance = NULL;
    RegisterClass(&wc);
    HWND hwndStart = SWButton(_hwnd, start_str.c_str(), 0, 0, start_btn_width, DESKTOPBARBAR_HEIGHT, IDC_START, WS_VISIBLE | WS_CHILD | BS_OWNERDRAW);
    SetWindowFont(hwndStart, GetStockFont(DEFAULT_GUI_FONT), FALSE);

    UINT idStartIcon = IDI_STARTMENU_B;
    if (isEmptyStartIcon) {
        idStartIcon = IDI_EMPTY;
    } else {
        if (JCFG2("JS_TASKBAR", "theme").ToString().compare(TEXT("dark")) == 0) {
            idStartIcon = IDI_STARTMENU_W;
        }
    }
    new StartButton(hwndStart, idStartIcon, TASKBAR_TEXTCOLOR(), true);

    /* Save the handle to the window, needed for push-state handling */
    _hwndStartButton = hwndStart;

    // disable double clicks
    SetClassLongPtr(hwndStart, GCL_STYLE, GetClassLongPtr(hwndStart, GCL_STYLE) & ~CS_DBLCLKS);

    // create task bar
    _hwndTaskBar = TaskBar::Create(_hwnd);

    if (!g_Globals._SHRestricted || !SHRestricted(REST_NOTRAYITEMSDISPLAY))
        // create tray notification area
        _hwndNotify = NotifyArea::Create(_hwnd);


    // notify all top level windows about the successfully created desktop bar
    SendNotifyMessage(HWND_BROADCAST, WM_TASKBARCREATED, 0, 0);

    //LoadSSO(); /* load in main() by SSOThread */

    _hwndQuickLaunch = QuickLaunchBar::Create(_hwnd);

    SetTimer(_hwnd, 0, 1000, NULL);

    if (JCFG_TB(2, "userebar").ToBool() == TRUE) {
        JCFG_QL_SET(2, "maxiconsinrow") = 0;
        // create rebar window to manage task and quick launch bar
        _hwndrebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            RBS_VARHEIGHT | RBS_AUTOSIZE | RBS_DBLCLKTOGGLE | RBS_REGISTERDROP |
            CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_TOP | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE,
            start_btn_width + 1, 1, 0, 0, _hwnd, 0, g_Globals._hInstance, 0);

        REBARBANDINFO rbBand;
        rbBand.cbSize = sizeof(REBARBANDINFO);
        rbBand.fMask = RBBIM_TEXT | RBBS_FIXEDBMP | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_ID | RBBIM_IDEALSIZE;
        {
            rbBand.fMask |= RBBIM_COLORS | RBBIM_BACKGROUND;
            rbBand.clrBack = 0;
            HDC hdc = GetDC(_hwnd);
            _hbmQuickLaunchBack = CreateSolidBitmap(hdc, 768, 16, TASKBAR_BKCOLOR());
            ReleaseDC(_hwnd, hdc);
            rbBand.hbmBack = _hbmQuickLaunchBack;
            //rbBand.hbmBack = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(IDB_TB_SH_DEF_16));
        }
        rbBand.cyChild = REBARBAND_HEIGHT - 5;
        rbBand.cyMaxChild = (ULONG)-1;
        rbBand.cyMinChild = REBARBAND_HEIGHT;
        rbBand.cyIntegral = REBARBAND_HEIGHT + 3;   //@@ OK?
        rbBand.fStyle = RBBS_VARIABLEHEIGHT | RBBS_FIXEDBMP | RBBS_HIDETITLE;
        if (JCFG_TB(2, "rebarlock").ToBool() == TRUE) {
            rbBand.fStyle |= RBBS_NOGRIPPER;
        } else {
            rbBand.fStyle |= RBBS_GRIPPERALWAYS;
        }
        TCHAR QuickLaunchBand[] = TEXT("Quicklaunch");
        rbBand.lpText = QuickLaunchBand;
        rbBand.cch = sizeof(QuickLaunchBand);
        rbBand.hwndChild = _hwndQuickLaunch;
        rbBand.cx = 100;
        rbBand.cxMinChild = 100;
        //rbBand.hbmBack = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(IDB_TB_SH_DEF_16));
        rbBand.wID = IDW_QUICKLAUNCHBAR;
        SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

        TCHAR TaskbarBand[] = TEXT("Taskbar");
        rbBand.lpText = TaskbarBand;
        rbBand.cch = sizeof(TaskbarBand);
        rbBand.hwndChild = _hwndTaskBar;
        rbBand.cx = 200;    //pcs->cx-_taskbar_pos-quicklaunch_width-(notifyarea_width+1);
        rbBand.cxMinChild = 50;
        rbBand.wID = IDW_TASKTOOLBAR;
        SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
    }

    RegisterHotkeys();

    // prepare Startmenu, but hide it for now
    _startMenuRoot = GET_WINDOW(StartMenuRoot, StartMenuRoot::Create(_hwndStartButton, STARTMENUROOT_ICON_SIZE));
    _startMenuRoot->_hwndStartButton = _hwndStartButton;

    return 0;
}


StartButton::StartButton(HWND hwnd, UINT nid, COLORREF textcolor, bool flat)
    :  PictureButton(hwnd, SizeIcon(nid, TASKBAR_ICON_SIZE), TASKBAR_BRUSH(), textcolor, flat)
{
}

extern int VK_WIN_HOOK();

LRESULT StartButton::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    switch (nmsg) {
    // one click activation: handle button-down message, don't wait for button-up
    case WM_LBUTTONDOWN:
        if (!Button_GetState(_hwnd)) {
            Button_SetState(_hwnd, TRUE);
            SetCapture(_hwnd);
            if (VK_WIN_HOOK() == 0) {
                SendMessage(GetParent(_hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(_hwnd), 0), 0);
            }
        }
        Button_SetState(_hwnd, FALSE);
        break;

    // re-target mouse move messages while moving the mouse cursor through the start menu
    case WM_MOUSEMOVE:
        if (GetCapture() == _hwnd) {
            POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

            ClientToScreen(_hwnd, &pt);
            HWND hwnd = WindowFromPoint(pt);

            if (hwnd && hwnd != _hwnd) {
                ScreenToClient(hwnd, &pt);
                SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
            }
        }
        break;

    case WM_LBUTTONUP:
        if (GetCapture() == _hwnd) {
            ReleaseCapture();

            POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

            ClientToScreen(_hwnd, &pt);
            HWND hwnd = WindowFromPoint(pt);

            if (hwnd && hwnd != _hwnd) {
                ScreenToClient(hwnd, &pt);
                PostMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
                PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
            }
        }
        break;

    case WM_CANCELMODE:
        ReleaseCapture();
        break;

    default:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

#define AUTOREGISTERHOTKEY(unreg, hwnd, id,fsModifiers, vk)\
  if (!unreg) RegisterHotKey(hwnd, id,fsModifiers, vk);\
  else UnregisterHotKey(hwnd, id);

void DesktopBar::RegisterHotkeys(BOOL unreg)
{
    // register hotkey CONTROL+ESC opening STARTMENU
    AUTOREGISTERHOTKEY(unreg, _hwnd, IDHK_STARTMENU, MOD_CONTROL, VK_ESCAPE);
    AUTOREGISTERHOTKEY(unreg, _hwnd, IDHK_DESKTOP, MOD_WIN, 'D');
}

void DesktopBar::ProcessHotKey(int id_hotkey)
{
    switch (id_hotkey) {
    case IDHK_DESKTOP: {
        if (_startMenuRoot && _startMenuRoot->IsStartMenuVisible()) {
            ShowOrHideStartMenu();
        } else {
            g_Globals._desktop.ToggleMinimize();
        }
        break;
    }

    case IDHK_STARTMENU:
        ShowOrHideStartMenu();
        break;
    }
}

//is not PECMD TEXT window
#define DEF_IGNORE_WINDOWS TEXT(";[PECMD;#32770]")
//is not InstallShield...
#define DEF_IGNORE_WINDOW_CLASSES TEXT(";InstallShield_Win")

static BOOL IsIgnoredWindow(HWND hwnd)
{
    String ignore_window_titles = JCFG2_DEF("JS_TASKBAR", "IGNORE_WINDOW_TITLES", TEXT("")).ToString();
    String ignore_window_classes = JCFG2_DEF("JS_TASKBAR", "IGNORE_WINDOW_CLASSES", TEXT("")).ToString();
    String ignore_windows = JCFG2_DEF("JS_TASKBAR", "IGNORE_WINDOWS", TEXT("")).ToString();
    ignore_window_classes.append(DEF_IGNORE_WINDOW_CLASSES);
    ignore_windows.append(DEF_IGNORE_WINDOWS);
    TCHAR title_buffer[BUFFER_LEN] = { 0 };
    TCHAR class_buffer[BUFFER_LEN] = { 0 };
    String strWindow;
    if (!GetWindowText(hwnd, title_buffer, BUFFER_LEN))
        title_buffer[0] = '\0';
    if (!ignore_window_titles.empty() && ignore_window_titles.find(title_buffer) != String::npos) {
        return TRUE;
    }
    if (!GetClassName(hwnd, class_buffer, BUFFER_LEN))
        class_buffer[0] = '\0';
    if (!ignore_window_classes.empty() && ignore_window_classes.find(class_buffer) != String::npos) {
        return TRUE;
    }
    strWindow.printf(TEXT("[%s;%s]"), title_buffer, class_buffer);
    LOG(strWindow);
    if (!ignore_windows.empty() && ignore_windows.find(strWindow) != String::npos) {
        return TRUE;
    }
    return FALSE;
}

static void HideForFullScreenWindow(HWND hwnd)
{
    static bool hiddenState = false;
    bool hasFullScreenWindow = false;

    HMONITOR hwndMonitor, taskbarMonitor;
    MONITORINFO hwndMonitorInfo;
    hwndMonitorInfo.cbSize = sizeof(MONITORINFO);
    RECT wndRect;
    HWND hTopWindow = GetForegroundWindow();
    if (!hTopWindow) return;
    taskbarMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    hwndMonitor = MonitorFromWindow(hTopWindow, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hwndMonitor, &hwndMonitorInfo);

    GetWindowRect(hTopWindow, &wndRect);

    // A full screen window is on the same monitor as the taskbar...
    if ((hwndMonitor == taskbarMonitor) &&
        // and hwnd is not an taskbar...
        (hwnd != hTopWindow) &&
        // and hwnd is not the Explorer desktop...
        (g_Globals._hwndDesktop != hTopWindow) &&
        // and hwnd is visible...
        IsWindowVisible(hTopWindow) &&
        // and hwnd size is greater than the resolution of the monitor which the
        // applet is on, hwnd is full screen.
        (wndRect.left <= hwndMonitorInfo.rcMonitor.left) &&
        (wndRect.top <= hwndMonitorInfo.rcMonitor.top) &&
        (wndRect.right >= hwndMonitorInfo.rcMonitor.right) &&
        (wndRect.bottom >= hwndMonitorInfo.rcMonitor.bottom) &&
        // and is not ignore window.
        !IsIgnoredWindow(hTopWindow)) {
        hasFullScreenWindow = true;
    }

    if (hasFullScreenWindow) {
        if (!hiddenState) {
            LOG(FmtString(TEXT("EXSTYLE:0x%x"), GetWindowExStyle(hTopWindow)));
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
                SWP_NOACTIVATE | SWP_HIDEWINDOW);
            hiddenState = true;
        }
    } else {
        if (hiddenState) {
            LOG(FmtString(TEXT("EXSTYLE:0x%x"), GetWindowExStyle(hTopWindow)));
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
            hiddenState = false;
        }
    }
}

static void OnTraySndVol(HWND hwnd, UINT id)
{
    if (hwnd) KillTimer(hwnd, id); // finish one-click timer
    //launch volume control in rightbottom(x:4096, y:4096)
    launch_file(hwnd, TEXT("SndVol.exe"), SW_SHOWNORMAL, TEXT("-m 268439552"));
}

static void OnTrayNetwork(HWND hwnd, UINT id)
{
    if (hwnd) KillTimer(hwnd, id);
    launch_file(hwnd, TEXT("winxshell.exe"), SW_SHOWNORMAL, _T("-ui -jcfg UI_WIFI\\main.jcfg"));
}

LRESULT DesktopBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    //if (nmsg != WM_TIMER) LOG(FmtString(TEXT("NMSG - %d"), nmsg));
    switch (nmsg) {
    case WM_NCHITTEST: {
#ifndef TASKBAR_AT_TOP
        POINTS pts = MAKEPOINTS(lparam);
        RECT rc;
        GetWindowRect(_hwnd, &rc);
        if (pts.y <= rc.top + 8)
            return HTTOP;
#endif
        LRESULT res = super::WndProc(nmsg, wparam, lparam);

        if (res >= HTSIZEFIRST && res <= HTSIZELAST) {
#ifdef TASKBAR_AT_TOP
            if (res == HTBOTTOM)    // enable vertical resizing at the lower border
#else
            if (res == HTTOP)       // enable vertical resizing at the upper border
#endif
                return res;
            else
                return HTCLIENT;    // disable any other resizing
        }
        return res;
    }

    case WM_SYSCOMMAND:
        if ((wparam & 0xFFF0) == SC_SIZE) {
#ifdef TASKBAR_AT_TOP
            if (wparam == SC_SIZE + 6) // enable vertical resizing at the lower border
#else
            if (wparam == SC_SIZE + 3) // enable vertical resizing at the upper border
#endif
                goto def;
            else
                return 0;           // disable any other resizing
        } else if (wparam == SC_TASKLIST)
            ShowOrHideStartMenu();
        goto def;

    case WM_SIZE:
        Resize(LOWORD(lparam), HIWORD(lparam));
        break;

    case WM_SIZING:
        ControlResize(wparam, lparam);
        break;

    case PM_RESIZE_CHILDREN: {
        ClientRect size(_hwnd);
        Resize(size.right, size.bottom);
        break;
    }

    case WM_CLOSE:
        ShowExitWindowsDialog(_hwnd);
        break;

    case WM_HOTKEY:
        ProcessHotKey((int)wparam);
        break;

    case WM_COPYDATA:
        return ProcessCopyData((COPYDATASTRUCT *)lparam);

    case WM_CONTEXTMENU: {
        POINTS p;
        p.x = LOWORD(lparam);
        p.y = HIWORD(lparam);
        PopupMenu menu(IDM_DESKTOPBAR);
        SetMenuDefaultItem(menu, 0, MF_BYPOSITION);
        if (GetKeyState(VK_SHIFT) < 0) {
            menu.Append(0, NULL, MF_SEPARATOR);
            menu.Append(ID_ABOUT_EXPLORER, ResString(IDS_ABOUT_EXPLORER));
            menu.Append(FCIDM_SHVIEWLAST - 1, ResString(IDS_TERMINATE));
        }
        menu.TrackPopupMenu(_hwnd, p);
        break;
    }

    case PM_GET_LAST_ACTIVE:
        if (_hwndTaskBar)
            return SendMessage(_hwndTaskBar, nmsg, wparam, lparam);
        break;

    case PM_REFRESH_CONFIG:   ///@todo read desktop bar settings
        SendMessage(_hwndNotify, PM_REFRESH_CONFIG, 0, 0);
        break;

    case WM_TIMER:
        if (wparam == 0) {
            SendMessage(_hwndQuickLaunch, PM_RELOAD_BUTTONS, 0, 0);
            if (JCFG2_DEF("JS_TASKBAR", "hideforfullscreenwindow", true).ToBool() != FALSE) {
                HideForFullScreenWindow(_hwnd);
            }
        } else if (wparam == ID_TRAY_VOLUME) {
            OnTraySndVol(_hwnd, (UINT)wparam);
        } else if (wparam == ID_TRAY_NETWORK) {
            OnTrayNetwork(_hwnd, (UINT)wparam);
        }
        break;

    case PM_GET_NOTIFYAREA:
        return (LRESULT)(HWND)_hwndNotify;

    case WM_SYSCOLORCHANGE:
        /* Forward WM_SYSCOLORCHANGE to common controls */
        if (_hwndrebar) {
            SendMessage(_hwndrebar, WM_SYSCOLORCHANGE, 0, 0);
        }
        SendMessage(_hwndQuickLaunch, WM_SYSCOLORCHANGE, 0, 0);
        SendMessage(_hwndTaskBar, WM_SYSCOLORCHANGE, 0, 0);
        break;

default: def:
        return super::WndProc(nmsg, wparam, lparam);
    }

    return 0;
}

int DesktopBar::Notify(int id, NMHDR *pnmh)
{
    if (pnmh->code == RBN_CHILDSIZE) {
        /* align the task bands to the top, so it's in row with the Start button */
        NMREBARCHILDSIZE *childSize = (NMREBARCHILDSIZE *)pnmh;

        if (childSize->wID == IDW_TASKTOOLBAR) {
            int cy = childSize->rcChild.top - childSize->rcBand.top;

            if (cy) {
                childSize->rcChild.bottom -= cy;
                childSize->rcChild.top -= cy;
            }
        }
    }

    return 0;
}

static int CommandHook(HWND hwnd, const TCHAR *act)
{
    String cmd = TEXT("");
    INT showflags = SW_SHOWNORMAL;
    String parameters = TEXT("");
    if (!hwnd) return 0;
    cmd = JCFG_CMD("JS_TASKBAR", act, "command", TEXT("")).ToString();
    if (cmd != TEXT("")) {
        showflags = JCFG_CMD("JS_TASKBAR", act, "showflags", showflags).ToInt();
        parameters = JCFG_CMD("JS_TASKBAR", act, "parameters", TEXT("")).ToString();
        launch_file(hwnd, cmd, showflags, parameters);
        return 1;
    }
    return 0;
}

void DesktopBar::Resize(int cx, int cy)
{
    ///@todo general children resizing algorithm
    int quicklaunch_width = (int)SendMessage(_hwndQuickLaunch, PM_GET_WIDTH, 0, 0);
    int notifyarea_width = (int)SendMessage(_hwndNotify, PM_GET_WIDTH, 0, 0);
    //_log_(FmtString("Resize - %d,%d\r\n", cx, cy));
    HDWP hdwp = BeginDeferWindowPos(3);

    if (_hwndrebar)
        DeferWindowPos(hdwp, _hwndrebar, 0, _taskbar_pos, 1, cx - _taskbar_pos - (notifyarea_width + 1), cy - 2, SWP_NOZORDER | SWP_NOACTIVATE);
    else {
        if (_hwndQuickLaunch)
            DeferWindowPos(hdwp, _hwndQuickLaunch, 0, _taskbar_pos, 1, quicklaunch_width, cy - 2, SWP_NOZORDER | SWP_NOACTIVATE);

        if (_hwndTaskBar)
            DeferWindowPos(hdwp, _hwndTaskBar, 0, _taskbar_pos + quicklaunch_width, 1, cx - _taskbar_pos - quicklaunch_width - (notifyarea_width + 1), cy - 2, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (_hwndNotify)
        DeferWindowPos(hdwp, _hwndNotify, 0, cx - (notifyarea_width + 3), 1, notifyarea_width, cy - 2, SWP_NOZORDER | SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);

    WindowRect rect(_hwnd);
    RECT work_area = {0, 0, GetSystemMetrics(SM_CXSCREEN), rect.top};
    if (memcmp(&_work_area, &work_area, sizeof(RECT)) != 0) {
        SystemParametersInfo(SPI_SETWORKAREA, 0, &work_area, 0);    // don't use SPIF_SENDCHANGE because then we have to wait for any message being delivered
        PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETWORKAREA, 0);
        SendMessage(g_Globals._hwndShellView, WM_SETTINGCHANGE, SPI_SETWORKAREA, 0);
        _work_area = work_area;
        CommandHook(g_Globals._hwndDesktop, TEXT("onDisplayChanged"));
    }
}

int DesktopBar::Command(int id, int code)
{
    static int isStartButtonHooked = -1;
    switch (id) {
    case IDC_START: {
        if (isStartButtonHooked == -1) {
            String def_value = TEXT("");
            BOOL isEmptyStartIcon = JCFG2_DEF("JS_STARTMENU", "start_icon", TEXT("")).ToString().compare(TEXT("empty")) == 0;
            if (isEmptyStartIcon) {
                def_value = TEXT("none");
            }
            isStartButtonHooked = 0;
            if (JCFG2_DEF("JS_STARTMENU", "start_command", def_value).ToString().compare(TEXT("none")) == 0) {
                isStartButtonHooked = 1;
            }
        }
        if (!isStartButtonHooked) ShowOrHideStartMenu();
        break;
    }
    case ID_ABOUT_EXPLORER:
        explorer_about(g_Globals._hwndDesktop);
        break;

    case ID_DESKTOPBAR_SETTINGS:
        ExplorerPropertySheet(g_Globals._hwndDesktop);
        break;

    case ID_MINIMIZE_ALL:
        g_Globals._desktop.ToggleMinimize();
        break;

    case ID_EXPLORE: {
        explorer_open_frame(SW_SHOWNORMAL, NULL, EXPLORER_OPEN_QUICKLAUNCH);
        break;
    }
    case ID_TASKMGR:
        launch_file(_hwnd, TEXT("taskmgr.exe"), SW_SHOWNORMAL);
        break;
    case FCIDM_SHVIEWLAST - 1: {
        DestroyWindow(g_Globals._hwndDesktopBar);
        DestroyWindow(g_Globals._hwndDesktop);
        break;
    }
    case ID_TRAY_VOLUME:
        OnTraySndVol(NULL, 0);
        break;

    case ID_VOLUME_PROPERTIES:
        launch_cpanel(_hwnd, TEXT("mmsys.cpl"));
        break;

    default:
        if (_hwndQuickLaunch)
            return (int)SendMessage(_hwndQuickLaunch, WM_COMMAND, MAKEWPARAM(id, code), 0);
        else
            return 1;
    }

    return 0;
}


void DesktopBar::ShowOrHideStartMenu()
{
    if (_startMenuRoot) {
        // set the Button, if not set
        if (!Button_GetState(_hwndStartButton))
            Button_SetState(_hwndStartButton, TRUE);

        if (_startMenuRoot->IsStartMenuVisible())
            _startMenuRoot->CloseStartMenu();
        else
            _startMenuRoot->TrackStartmenu();

        // StartMenu was closed, release button state
        Button_SetState(_hwndStartButton, FALSE);
    }
}


// copy data structure for tray notifications
struct TrayNotifyCDS {
    DWORD   cookie;
    DWORD   notify_code;
    NOTIFYICONDATA nicon_data;
};

static LRESULT IconIdentifierEvent(COPYDATASTRUCT *pcds)
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    TrayNotifyCDS *ptr = (TrayNotifyCDS *)pcds->lpData;
    switch (ptr->notify_code)
    {
    case 2:
        return MAKELONG(16, 16);
    default:
        return MAKELONG(cursorPos.x, cursorPos.y);
    }

    return 0;
}

typedef struct _APPBARDATA_WIN10
{
    DWORD cbSize;
    DWORD hWnd;
    UINT uCallbackMessage;
    UINT uEdge;
    RECT rc;
    LPARAM lParam; // message specific
#ifndef _WIN64
    DWORD  dwPadding1;
#endif
}APPBARDATA_WIN10, *PAPPBARDATA_WIN10;

/* Win10 */
typedef struct
{
    APPBARDATA_WIN10 abd;
    DWORD  dwMessage;
    DWORD  dwPadding1;
    HANDLE hSharedMemory;
#ifndef _WIN64
    DWORD  dwPadding2;
#endif
    DWORD  dwSourceProcessId;
    DWORD  dwPadding3;
}APPBARMSGDATA_WIN10, *PAPPBARMSGDATA_WIN10;
typedef const APPBARMSGDATA_WIN10 *PCAPPBARMSGDATA_WIN10;

// Data sent with AppBar Message
typedef struct _SHELLAPPBARDATA
{
    _SHELLAPPBARDATA(APPBARDATA_WIN10& abdsrc) :abd(abdsrc) {}

    const APPBARDATA_WIN10& abd;
    /**/
    DWORD  dwMessage;
    HANDLE hSharedMemory;
    DWORD  dwSourceProcessId;
    /**/
} SHELLAPPBARDATA, *PSHELLAPPBARDATA;

static LRESULT HandleAppBarMessage(PSHELLAPPBARDATA psad)
{
    LRESULT lResult = 0;
    if (psad == NULL) return 0;
    switch (psad->dwMessage) {
    case ABM_GETSTATE:
        /* Returns the current TaskBar state(0:autohide, 1:alwaysontop) */
        break;
    case ABM_GETTASKBARPOS: {
        PAPPBARDATA_WIN10 pabd = NULL;
        if (psad->hSharedMemory) {
            pabd = (APPBARDATA_WIN10 *)SHLockShared(psad->hSharedMemory, psad->dwSourceProcessId);
        }
        if (pabd) {
            GetWindowRect(g_Globals._hwndDesktopBar, &(pabd->rc));
            pabd->uEdge = ABE_BOTTOM;
            SHUnlockShared(psad);
            lResult = 1;
        }
        break;
    }
    }

    return lResult;
}

static LRESULT ProcessAppBarMessage(COPYDATASTRUCT *pcds)
{
    LRESULT lResult = 0;

    switch (pcds->cbData) {
    case sizeof(APPBARMSGDATA_WIN10) : {
        PAPPBARMSGDATA_WIN10 pamd = (PAPPBARMSGDATA_WIN10)pcds->lpData;
        //TRACE("APPBARDATA_WIN10: %u", pamd->abd.cbSize);
        if (sizeof(APPBARDATA_WIN10) != pamd->abd.cbSize)
            break;
        //TRACE("dwMessage: %u", pamd->dwMessage);
        SHELLAPPBARDATA sbd((APPBARDATA_WIN10&)(pamd->abd));
        sbd.dwMessage = pamd->dwMessage;
        sbd.hSharedMemory = pamd->hSharedMemory;
        sbd.dwSourceProcessId = pamd->dwSourceProcessId;
        lResult = HandleAppBarMessage(&sbd);
        break;
    }
    default: {
        //TRACE("Unknown APPBARMSGDATA size: %u", pcds->cbData);
    }
    }
    return lResult;
}

#define SH_APPBAR_DATA 0
#define SH_TRAY_DATA 1
#define SH_TRAY_ICON_IDENT_DATA 3

LRESULT DesktopBar::ProcessCopyData(COPYDATASTRUCT *pcds)
{
    // Is this a tray notification message?
    if (pcds->dwData == SH_APPBAR_DATA) {
        return ProcessAppBarMessage(pcds);
    } else if (pcds->dwData == SH_TRAY_DATA) {
        TrayNotifyCDS *ptr = (TrayNotifyCDS *)pcds->lpData;
        NotifyArea *notify_area = GET_WINDOW(NotifyArea, _hwndNotify);
        if (notify_area)
            return notify_area->ProcessTrayNotification(ptr->notify_code, &ptr->nicon_data);
    } else if (pcds->dwData == SH_TRAY_ICON_IDENT_DATA) {
        return IconIdentifierEvent(pcds);
    }

    return FALSE;
}


void DesktopBar::ControlResize(WPARAM wparam, LPARAM lparam)
{
    PRECT dragRect = (PRECT) lparam;
    //int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    ///@todo write code for taskbar being at sides or top.

    switch (wparam) {
    case WMSZ_BOTTOM: ///@todo Taskbar is at the top of the screen
        break;

    case WMSZ_TOP:    // Taskbar is at the bottom of the screen
        dragRect->top = screenHeight - ((screenHeight - dragRect->top) / DESKTOPBARBAR_HEIGHT) * DESKTOPBARBAR_HEIGHT;
        if (dragRect->top < screenHeight / 2)
            dragRect->top = screenHeight - ((screenHeight / 2 / DESKTOPBARBAR_HEIGHT) * DESKTOPBARBAR_HEIGHT);
        else if (dragRect->top > screenHeight - DESKTOPBARBAR_HEIGHT)
            dragRect->top = screenHeight - DESKTOPBARBAR_HEIGHT;
        break;

    case WMSZ_RIGHT:  ///@todo Taskbar is at the left of the screen
        break;

    case WMSZ_LEFT:   ///@todo Taskbar is at the right of the screen
        break;
    }
}


void DesktopBar::AddTrayIcons()
{
    HICON icon = g_Globals._icon_cache.get_icon(ICID_TRAY_SND_NONE).get_hicon();
    _traySndVolIcon.Add(icon, ResString(IDS_VOLUME));
    icon = g_Globals._icon_cache.get_icon(ICID_TRAY_NET_WIRED_LAN).get_hicon();
    _trayNetworkIcon.Add(icon, ResString(IDS_NETWORK));
}

void DesktopBar::TrayClick(UINT id, int btn)
{
    switch (id) {
    case ID_TRAY_VOLUME:
        if (btn == TRAYBUTTON_LEFT) {
            SetTimer(_hwnd, ID_TRAY_VOLUME, 500, NULL); // wait a bit to correctly handle double clicks
        } else {
            PopupMenu menu(IDM_VOLUME);
            SetMenuDefaultItem(menu, 0, MF_BYPOSITION);
            menu.TrackPopupMenuAtPos(_hwnd, GetMessagePos());
        }
        break;
    case ID_TRAY_NETWORK:
        if (btn == TRAYBUTTON_LEFT) {
            SetTimer(_hwnd, ID_TRAY_NETWORK, 500, NULL); // wait a bit to correctly handle double clicks
        }
        break;
    }
}

void DesktopBar::TrayDblClick(UINT id, int btn)
{
    switch (id) {
    case ID_TRAY_VOLUME:
        OnTraySndVol(_hwnd, id);
        break;
    case ID_TRAY_NETWORK:
        OnTrayNetwork(_hwnd, id);
        break;
    }
}

