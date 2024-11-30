#include <precomp.h>
#include "../resource.h"

/// "About Explorer" Dialog
struct ExplorerAboutDlg : public
    CtlColorParent <
    OwnerDrawParent<Dialog>
    > {
    typedef CtlColorParent <
        OwnerDrawParent<Dialog>
    > super;

    ExplorerAboutDlg(HWND hwnd)
        : super(hwnd)
    {
        SetWindowIcon(hwnd, IDI_WINXSHELL);

        new FlatButton(hwnd, IDOK);

        _hfont = CreateFont(20, 0, 0, 0, FW_BOLD, TRUE, 0, 0, 0, 0, 0, 0, 0, TEXT("Sans Serif"));
        new ColorStatic(hwnd, IDC_PE_EXPLORER, RGB(32, 32, 128), 0, _hfont);

        new HyperlinkCtrl(hwnd, IDC_WWW);

        FmtString ver_txt(ResString(IDS_EXPLORER_VERSION_STR), (LPCTSTR)ResString(IDS_VERSION_STR));
        SetWindowText(GetDlgItem(hwnd, IDC_VERSION_TXT), ver_txt);
        SetWindowText(GetDlgItem(hwnd, IDC_COPYRIGHT_TXT), ResString(IDS_COPYRIGHT_STR));

        HWND hwnd_winver = GetDlgItem(hwnd, IDC_WIN_VERSION);
        SetWindowText(hwnd_winver, get_windows_version_str());
        SetWindowFont(hwnd_winver, g_Globals._hDefaultFont, FALSE);

        CenterWindow(hwnd);
    }

    ~ExplorerAboutDlg()
    {
        DeleteObject(_hfont);
    }

    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
    {
        switch (nmsg) {
        case WM_PAINT:
            Paint();
            break;

        default:
            return super::WndProc(nmsg, wparam, lparam);
        }

        return 0;
    }

    void Paint()
    {
        PaintCanvas canvas(_hwnd);

        HICON hicon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(IDI_WINXSHELL_BIG), IMAGE_ICON, 0, 0, LR_SHARED);

        DrawIconEx(canvas, 20, 10, hicon, 0, 0, 0, 0, DI_NORMAL);
    }

protected:
    HFONT    _hfont;
};



void explorer_about(HWND hwndParent)
{
    Dialog::DoModal(IDD_ABOUT_EXPLORER, WINDOW_CREATOR(ExplorerAboutDlg), hwndParent);
}
