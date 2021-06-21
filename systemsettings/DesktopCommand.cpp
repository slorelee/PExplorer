
#include "DesktopCommand.h"

#include <atlcomcli.h>
#include <Windows.h>
#include "../globals.h"

extern BOOL isWinXShellAsShell();

// resource.h
#define ID_REFRESH                      1704

class CCoInitialize {
public:
    CCoInitialize() : m_hr(CoInitialize(NULL)) { }
    ~CCoInitialize() { if (SUCCEEDED(m_hr)) CoUninitialize(); }
    operator HRESULT() const { return m_hr; }
    HRESULT m_hr;
};

DesktopCommand::DesktopCommand()
{
    CCoInitialize initCom;
    _pShellView = NULL;
    _pFolderView = NULL;
}

DesktopCommand::DesktopCommand(IShellView * pShellView, IFolderView2 * pFolderView)
{
    _pShellView = pShellView;
    _pFolderView = pFolderView;
}

DesktopCommand::~DesktopCommand()
{
}


void FindDesktopFolderView(REFIID riid, void **ppv)
{
    CComPtr<IShellWindows> spShellWindows;
    spShellWindows.CoCreateInstance(CLSID_ShellWindows);
    CComVariant vtLoc(CSIDL_DESKTOP);
    CComVariant vtEmpty;

    long lhwnd;
    CComPtr<IDispatch> spdisp;

    spShellWindows->FindWindowSW(
        &vtLoc, &vtEmpty,
        SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp);

    if (!spdisp) return;

    CComPtr<IShellBrowser> spBrowser;
    CComQIPtr<IServiceProvider>(spdisp)->
        QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&spBrowser));


    CComPtr<IShellView> spView;
    spBrowser->QueryActiveShellView(&spView);
    spView->QueryInterface(riid, ppv);
}

void DesktopCommand::Refresh()
{
    if (isWinXShellAsShell()) {
        HWND desktop = FindWindow(TEXT("Progman"), TEXT("Program Manager"));
        SendMessage(desktop, WM_USER + WM_COMMAND, WM_DESKTOP_REFRESH, 0x0);
        return;
    }

    CComPtr<IShellView> spView;

    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    if (NULL == spView) {
        return;
    }
    spView->Refresh();
}

void DesktopCommand::SetIconSize(int size)
{
    if (_pShellView) {
        if (_pFolderView) {
            _pFolderView->SetViewModeAndIconSize(FVM_ICON, size);
        }
        return;
    }

    if (isWinXShellAsShell()) {
        HWND desktop = FindWindow(TEXT("Progman"), TEXT("Program Manager"));
        SendMessage(desktop, WM_USER + WM_COMMAND, WM_DESKTOP_SETICONSIZE, size);
        return;
    }

    CComPtr<IFolderView2> spView;

    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    if (NULL == spView) {
        return;
    }
    spView->SetViewModeAndIconSize(FVM_ICON, size);
}

void DesktopCommand::SetFolderFlags(DWORD dwMask, int checked)
{
    if (_pFolderView) {
        if (checked) {
            _pFolderView->SetCurrentFolderFlags(dwMask, dwMask);
        } else {
            _pFolderView->SetCurrentFolderFlags(dwMask, 0);
        }
        return;
    }

    if (isWinXShellAsShell()) {
        HWND desktop = FindWindow(TEXT("Progman"), TEXT("Program Manager"));
        SendMessage(desktop, WM_USER + WM_COMMAND, WM_DESKTOP_UNSETFOLDERFLAGS + checked, dwMask);
        return;
    }

    CComPtr<IFolderView2> spView;
    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    if (NULL == spView) {
        return;
    }

    if (checked) {
        spView->SetCurrentFolderFlags(dwMask, dwMask);
    } else {
        spView->SetCurrentFolderFlags(dwMask, 0);
    }
}

void DesktopCommand::AutoArrange(int checked)
{
    SetFolderFlags(FWF_AUTOARRANGE, checked);
}

void DesktopCommand::SnapToGrid(int checked)
{
    SetFolderFlags(FWF_SNAPTOGRID, checked);
}

void DesktopCommand::ShowIcons(int checked)
{
    SetFolderFlags(FWF_NOICONS, 1^checked);
}
