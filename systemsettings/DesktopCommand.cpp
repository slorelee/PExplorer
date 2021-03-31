
#include "DesktopCommand.h"

#include <atlcomcli.h>
#include <ShlObj.h>
#include <Windows.h>

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

    CComPtr<IShellBrowser> spBrowser;
    CComQIPtr<IServiceProvider>(spdisp)->
        QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&spBrowser));


    CComPtr<IShellView> spView;
    spBrowser->QueryActiveShellView(&spView);
    spView->QueryInterface(riid, ppv);
}

void DesktopCommand::Refresh()
{
    CComPtr<IShellView> spView;

    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    if (NULL == spView) {
        return;
    }
    spView->Refresh();
}

void DesktopCommand::SetIconSize(int size)
{
    CComPtr<IFolderView2> spView;

    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    if (NULL == spView) {
        return;
    }
    spView->SetViewModeAndIconSize(FVM_ICON, size);
}

static void SetDesktopFlags(DWORD dwMask, int checked)
{
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
    SetDesktopFlags(FWF_AUTOARRANGE, checked);
}

void DesktopCommand::SnapToGrid(int checked)
{
    SetDesktopFlags(FWF_SNAPTOGRID, checked);
}

void DesktopCommand::ShowIcons(int checked)
{
    SetDesktopFlags(FWF_NOICONS, 1^checked);
}
