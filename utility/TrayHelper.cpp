
#include <Windows.h>
#include "TrayHelper.h"

CTrayIcon::CTrayIcon(HWND hparent, UINT id)
    : _hparent(hparent), _id(id) {}

CTrayIcon::~CTrayIcon() {
    Remove();
}

void CTrayIcon::Add(HICON hIcon, LPCTSTR tooltip) {
    Set(NIM_ADD, _id, hIcon, tooltip);
}

void CTrayIcon::Modify(HICON hIcon, LPCTSTR tooltip) {
    Set(NIM_MODIFY, _id, hIcon, tooltip);
}

void CTrayIcon::Remove() {
    NOTIFYICONDATA nid = {
        sizeof(NOTIFYICONDATA), // cbSize
        _hparent,               // hWnd
        _id,                    // uID
    };

    Shell_NotifyIcon(NIM_DELETE, &nid);
}


void CTrayIcon::Set(DWORD dwMessage, UINT id, HICON hIcon, LPCTSTR tooltip) {
    NOTIFYICONDATA nid = {
        sizeof(NOTIFYICONDATA), // cbSize
        _hparent,               // hWnd
        id,                     // uID
        NIF_MESSAGE | NIF_ICON, // uFlags
        PM_TRAYICON,            // uCallbackMessage
        hIcon                   // hIcon
    };

    if (tooltip)
        lstrcpyn(nid.szTip, tooltip, sizeof(nid.szTip));

    if (nid.szTip[0])
        nid.uFlags |= NIF_TIP;

    Shell_NotifyIcon(dwMessage, &nid);
}


CTrayIconController::CTrayIconController() {
    WM_TASKBARCREATED = RegisterWindowMessage(WINMSG_TASKBARCREATED);
}

LRESULT CTrayIconController::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam) {
    if (nmsg == PM_TRAYICON) {
        switch (lparam) {
        case WM_MOUSEMOVE:
            TrayMouseOver((UINT)wparam);
            break;

        case WM_LBUTTONDOWN:
            TrayClick((UINT)wparam, TRAYBUTTON_LEFT);
            break;

        case WM_LBUTTONDBLCLK:
            TrayDblClick((UINT)wparam, TRAYBUTTON_LEFT);
            break;

        case WM_RBUTTONDOWN:
            TrayClick((UINT)wparam, TRAYBUTTON_RIGHT);
            break;

        case WM_RBUTTONDBLCLK:
            TrayDblClick((UINT)wparam, TRAYBUTTON_RIGHT);
            break;

        case WM_MBUTTONDOWN:
            TrayClick((UINT)wparam, TRAYBUTTON_MIDDLE);
            break;

        case WM_MBUTTONDBLCLK:
            TrayDblClick((UINT)wparam, TRAYBUTTON_MIDDLE);
            break;
        }
        return S_OK;
    } else if (nmsg == WM_TASKBARCREATED) {
        AddTrayIcons();
        return S_FALSE; /* fallthough */
    }
    return S_FALSE;
}

void CTrayIconController::AddTrayIcons() {}
void CTrayIconController::TrayMouseOver(UINT id) {}
void CTrayIconController::TrayClick(UINT id, int btn) {}
void CTrayIconController::TrayDblClick(UINT id, int btn) {}

