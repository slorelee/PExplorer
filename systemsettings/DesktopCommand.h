#pragma once
#include <ShlObj.h>

class DesktopCommand
{
public:
    DesktopCommand();
    DesktopCommand(IShellView *pShellView, IFolderView2 *pFolderView);
    ~DesktopCommand();
    void Refresh();
    void SetIconSize(int size);
    void AutoArrange(int checked);
    void SnapToGrid(int checked);
    void ShowIcons(int checked);
    void SetFolderFlags(DWORD dwMask, int checked);
private:
    IShellView *_pShellView;
    IFolderView2 *_pFolderView;
};

// WM_USERCOMMAND for desktop
#define WM_DESKTOP_REFRESH              0x01
#define WM_DESKTOP_SETICONSIZE          0x02
#define WM_DESKTOP_UNSETFOLDERFLAGS     0x10
#define WM_DESKTOP_SETFOLDERFLAGS       0x11
