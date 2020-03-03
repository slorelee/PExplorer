#pragma once

#include <Windows.h>

#define TRAYBUTTON_LEFT   0
#define TRAYBUTTON_RIGHT  1
#define TRAYBUTTON_MIDDLE 2

#ifndef PM_TRAYICON
#define PM_TRAYICON     (WM_APP + 0x20)
#endif // !PM_TRAYICON

#define WINMSG_TASKBARCREATED   TEXT("TaskbarCreated")

class CTrayIcon {
public:
    CTrayIcon(HWND hparent, UINT id);
    ~CTrayIcon();

    void Add(HICON hIcon, LPCTSTR tooltip = NULL);
    void Modify(HICON hIcon, LPCTSTR tooltip = NULL);
    void Remove();

protected:
    HWND _hparent;
    UINT _id;

    void Set(DWORD dwMessage, UINT id, HICON hIcon, LPCTSTR tooltip = NULL);
};


class CTrayIconController {
public:
    CTrayIconController();

    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
    virtual void AddTrayIcons();
    virtual void TrayMouseOver(UINT id);
    virtual void TrayClick(UINT id, int btn);
    virtual void TrayDblClick(UINT id, int btn);

protected:
    UINT WM_TASKBARCREATED;
    HICON hLastIcon;
};
