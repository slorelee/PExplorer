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
// Explorer and Desktop clone
//
// desktopbar.h
//
// Martin Fuchs, 22.08.2003
//

#include "../customization/startbutton.h"

#define CLASSNAME_EXPLORERBAR   TEXT("Shell_TrayWnd")
#define TITLE_EXPLORERBAR       TEXT("")    // use an empty window title, so windows taskmanager does not show the window in its application list

#define IDC_START               0x1000
#define IDC_LOGOFF              0x1001
#define IDC_SHUTDOWN            0x1002
#define IDC_LAUNCH              0x1003
#define IDC_START_HELP          0x1004
#define IDC_SEARCH_FILES        0x1005
#define IDC_SEARCH_COMPUTER     0x1006
#define IDC_SETTINGS            0x1007
#define IDC_ADMIN               0x1008
#define IDC_DOCUMENTS           0x1009
#define IDC_RECENT              0x100A
#define IDC_FAVORITES           0x100B
#define IDC_PROGRAMS            0x100C
#define IDC_EXPLORE             0x100D
#define IDC_NETWORK             0x100E
#define IDC_CONNECTIONS         0x100F
#define IDC_DRIVES              0x1010
#define IDC_CONTROL_PANEL       0x1011
#define IDC_SETTINGS_MENU       0x1012
#define IDC_PRINTERS            0x1013
#define IDC_PRINTERS_MENU       0x1014
#define IDC_BROWSE              0x1015
#define IDC_SEARCH_PROGRAM      0x1016
#define IDC_SEARCH              0x1017
#define IDC_TERMINATE           0x1018
#define IDC_RESTART             0x1019

#define IDC_FIRST_MENU          0x3000

// hotkeys
#define IDHK_EXPLORER 0
#define IDHK_RUN 1
#define IDHK_DESKTOP 2
#define IDHK_LOGOFF 3
#define IDHK_STARTMENU 4

/// desktop bar window, also known as "system tray"
struct DesktopBar : public
    TrayIconControllerTemplate <
    OwnerDrawParent<Window> >
{
    typedef TrayIconControllerTemplate <
    OwnerDrawParent<Window> > super;

    DesktopBar(HWND hwnd);
    ~DesktopBar();

    static HWND Create();

protected:
    RECT    _work_area_org;
    RECT    _work_area;
    int     _taskbar_pos;
    int     _deskbar_pos_y;
    LRESULT Init(LPCREATESTRUCT pcs);
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
    int     Notify(int id, NMHDR *pnmh);
    int     Command(int id, int code);

    void    Resize(int cx, int cy);
    void    ControlResize(WPARAM wparam, LPARAM lparam);
    void    RegisterHotkeys(BOOL unreg = FALSE);
    void    ProcessHotKey(int id_hotkey);
    void    ShowOrHideStartMenu();
    LRESULT ProcessCopyData(COPYDATASTRUCT *pcd);

    WindowHandle _hwndTaskBar;
    WindowHandle _hwndNotify;
    WindowHandle _hwndQuickLaunch;
    WindowHandle _hwndrebar;
    /* Needed to make the StartButton pushed, if it's called by windowskey: SC_TASKLIST command */
    WindowHandle _hwndStartButton;

    struct StartMenuRoot *_startMenuRoot;

    HBITMAP _hbmQuickLaunchBack;
    int     _iQuickLaunchPadding;
    TrayIcon    _traySndVolIcon;
    TrayIcon    _trayNetworkIcon;
    void    AddTrayIcons();
    virtual void TrayClick(UINT id, int btn);
    virtual void TrayDblClick(UINT id, int btn);
};


/// special "Start" button with one click activation
struct StartButton : public PictureButton2 {
    typedef PictureButton2 super;

    StartButton(HWND hwnd, UINT nid, COLORREF textcolor = -1, bool flat = false);

protected:
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};
