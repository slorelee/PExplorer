

#include <precomp.h>
#include <Dwmapi.h>

#include "../resource.h" /* IDI_EXPLORER */
#include "fileexplorer.h"

#pragma comment(lib, "Dwmapi.lib")

#define CLSID_MyComputerName     TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}")
#define CLSID_RecycleBinName     TEXT("::{645FF040-5081-101B-9F08-00AA002F954E}")
#define CLSID_UsersFilesName     TEXT("::{59031A47-3F72-44A7-89C5-5595FE6B30EE}")
#define CLSID_MyDocumentsName    TEXT("::"STR_MYDOCS_CLSID)
//"{450D8FBA-AD25-11D0-98A8-0800361B1103}"

#define DEF_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEF_GUID(CLSID_UsersFiles, 0x59031a47, 0x3f72, 0x44a7, 0x89, 0xc5, 0x55, 0x95, 0xfe, 0x6b, 0x30, 0xee);//59031A47-3F72-44A7-89c5-5595fe6b30ee
DEF_GUID(CLSID_MyDocuments, 0x450d8fba, 0xad25, 0x11d0, 0x98, 0xa8, 0x08, 0x95, 0x00, 0x36, 0x1b, 0x03);//450d8fba-ad25-11d0-98a8-0800361b1103

//#include "../utility/window.h"

HHOOK FileExplorerWindow::HookHandle = NULL;
FileExplorerWindow::FileExplorerWindow(HWND hwnd)
    : super(hwnd)
{
}

FileExplorerWindow::~FileExplorerWindow()
{
    // don't exit desktop when closing file manager window
    if (!g_Globals._desktop_mode) {
        ReleaseHook();
        PostQuitMessage(0);
    }
}

void FileExplorerWindow::ReleaseHook()
{
    if (HookHandle) {
        UnhookWindowsHookEx(HookHandle);
        HookHandle = NULL;
    }
}

#define WM_OPENDIALOG (WM_USER+1)
#define WM_CUSTOMDIALOG (WM_USER+2)

HWND FileExplorerWindow::Create()
{
    static IconWindowClass wcFileExplorer(FILEEXPLORERWINDOWCLASSNAME, IDI_EXPLORER);
    HWND hFrame = Window::Create(WINDOW_CREATOR(FileExplorerWindow), 0,
        wcFileExplorer, TEXT("File Explorer"),
        WS_OVERLAPPEDWINDOW, //WS_VISIBLE | WS_CHILD | SS_SIMPLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, 0);
    //ShowWindow(hFrame, SW_SHOW);
    return hFrame;
}

#define FILEEXPLORER_MAGICID 0x5758534d //'W', 'X', 'S', 'M' WinXShell Mark
static int isFileExplorerWindow(HWND hwnd)
{
    LONG_PTR mark = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (mark == FILEEXPLORER_MAGICID) return 1;
    return 0;
}

#define MINBUTTON_CLICKED MAKELONG(HTMINBUTTON, WM_LBUTTONDOWN)

LRESULT MinButtonHooker(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        CWPSTRUCT *pData = (CWPSTRUCT*)lParam;
        if (pData->message == WM_SETCURSOR) {
            if (pData->lParam == MINBUTTON_CLICKED) {
                if (isFileExplorerWindow(pData->hwnd)) {
                    ShowWindow(pData->hwnd, SW_SHOWMINNOACTIVE);
                }
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

HWND FileExplorerWindow::Create(HWND hwnd, String path)
{
    HWND hFrame = Create();
    BOOL dwmEnabled = FALSE;
    DwmGetWindowAttribute(hFrame, DWMWA_NCRENDERING_ENABLED, &dwmEnabled, sizeof(BOOL));

    //fix issue that can't minimize window when clicking minimizebox if dwm is enabled.
    if (FileExplorerWindow::HookHandle == NULL && dwmEnabled) {
        HookHandle = SetWindowsHookEx(WH_CALLWNDPROC,
            (HOOKPROC)MinButtonHooker, (HINSTANCE)NULL, (DWORD)GetCurrentThreadId());
    }

    String *dirpath = new String(path);
    PostMessage(hFrame, WM_OPENDIALOG, (WPARAM)hFrame, (LPARAM)dirpath);
    return hFrame;
}

LRESULT FileExplorerWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    //_log_(FmtString(TEXT("0x%x"), nmsg));
    LRESULT res = 0;
    if (nmsg == WM_OPENDIALOG) {
        String *path = (String *)lparam;
        OpenDialog((HWND)wparam, *path);
        delete path;
        PostMessage((HWND)wparam, WM_CLOSE, 0, 0);
        return S_OK;
    } else if (nmsg == WM_SETTINGCHANGE) {
        HandleEnvChangeBroadcast(lparam);
    }
    return super::WndProc(nmsg, wparam, lparam);
}


int FileExplorerWindow::OpenDialog(HWND hwnd, String path)
{
    HRESULT hr;
    IFileOpenDialog *pDlg = NULL;
    COMDLG_FILTERSPEC aFileTypes[] = {
        { L"All files", L"*.*" }
    };
    // Create the file-open dialog COM object.
    hr = CoCreateInstance(CLSID_FileOpenDialog,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pDlg));

    if (FAILED(hr))
        return 0;

    // Set the dialog's caption text and the available file types.
    // NOTE: Error handling omitted here for clarity.
    //pDlg->SetFileTypes(_countof(aFileTypes), aFileTypes);
    pDlg->SetTitle(path.c_str());
    pDlg->SetOptions(FOS_NOVALIDATE | FOS_ALLNONSTORAGEITEMS | FOS_ALLOWMULTISELECT | FOS_NODEREFERENCELINKS);

    IShellItem *psi = NULL;
    //LPITEMIDLIST pidlControlPanel;
    //SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlControlPanel);
    //hr = SHCreateItemFromIDList(pidlControlPanel, IID_PPV_ARGS(&psi));
    //hr = SHCreateItemInKnownFolder(CLSID_ControlPanel, 0, NULL, IID_PPV_ARGS(&psi));
    hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&psi));
    if (SUCCEEDED(hr)) {
        pDlg->SetFolder(psi);
        psi->Release();
    }

    // Create an event handling object, and hook it up to the dialog.
    IFileDialogEvents *pfde = NULL;
    hr = CFileDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
    if (FAILED(hr)) goto end;
    DWORD dwCookie = -1;
    pDlg->Advise(pfde, &dwCookie);
    if (FAILED(hr)) goto end;
    //PostMessage(hwnd, WM_CUSTOMDIALOG, (WPARAM)pDlg, (LPARAM)0);
    // Show the dialog.
    hr = pDlg->Show(hwnd);

    // Unhook the event handler
    if (dwCookie != -1) pDlg->Unadvise(dwCookie);
end:
    pDlg->Release();
    return 0;
}

static int CustomFileDialog(IFileOpenDialog *pfd)
{
    IOleWindow *pWindow = NULL;
    HRESULT hr = pfd->QueryInterface(IID_PPV_ARGS(&pWindow));

    if (FAILED(hr)) return -1;
    HWND hwndDialog;
    hr = pWindow->GetWindow(&hwndDialog);
    if (FAILED(hr)) return 0;
    pWindow->Release();

    // Disable non-client area rendering on the window.
    //DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
    //DwmSetWindowAttribute(hwndDialog, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

    LONG lStyle = 0;
    lStyle = GetWindowLong(hwndDialog, GWL_EXSTYLE);
    SetWindowExStyle(hwndDialog, lStyle & ~WS_EX_DLGMODALFRAME & ~WS_EX_CONTROLPARENT |
        WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW);
    lStyle = GetWindowStyle(hwndDialog);
    SetWindowStyle(hwndDialog, lStyle | WS_OVERLAPPEDWINDOW);
    // Set magic id for MinButtonHooker() function
    SetWindowLongPtr(hwndDialog, GWLP_USERDATA, FILEEXPLORER_MAGICID);

    //_log_(FmtString(TEXT("0x%x"), lStyle | ~WS_EX_DLGMODALFRAME | ~WS_EX_CONTROLPARENT));
    //SetWindowExStyle(hwndDialog, 0x110 | WS_EX_APPWINDOW);
    RECT rc;
    GetWindowRect(hwndDialog, &rc);
    SetWindowPos(hwndDialog, HWND_NOTOPMOST, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_DRAWFRAME);

    static int height = JCFG2_DEF("JS_FILEEXPLORER", "padding-bottom", 20).ToInt();
    if (height == 0) return 1;

    /* hide filename label */
    HWND item = GetDlgItem(hwndDialog, 0x442);
    ShowWindow(item, SW_HIDE);

    /* hide filename combox */
    item = GetDlgItem(hwndDialog, 0x47C);
    ShowWindow(item, SW_HIDE);

    /* hide filename filter combox */
    item = GetDlgItem(hwndDialog, 0x470);
    ShowWindow(item, SW_HIDE);

    /* hide OK button */
    item = GetDlgItem(hwndDialog, 1);
    ShowWindow(item, SW_HIDE);
    GetWindowRect(item, &rc);

    /* move OK button to adjust the listview height */
    MoveWindow(item, rc.left, ((rc.bottom - rc.top) * 15 - height), rc.right - rc.left, rc.bottom - rc.top, FALSE);

    /* hide Cancel button */
    item = GetDlgItem(hwndDialog, 2);
    ShowWindow(item, SW_HIDE);

    return 1;
}

IFACEMETHODIMP CFileDialogEventHandler::OnFolderChanging(IFileDialog *pDlg, IShellItem *pItem)
{
    LPOLESTR pwsz = NULL;
    HRESULT hr = pItem->GetDisplayName(SIGDN_PARENTRELATIVEFORUI, &pwsz);

    if (SUCCEEDED(hr)) {
        LOG(pwsz);
        CoTaskMemFree(pwsz);
    }
    return S_OK; 
}

IFACEMETHODIMP CFileDialogEventHandler::OnFolderChange(IFileDialog *pDlg)
{
    if (!m_DialogInited) {
        CustomFileDialog((IFileOpenDialog *)pDlg);
        m_DialogInited = TRUE;
    }
    IShellItem *pItem = NULL;
    HRESULT hr = pDlg->GetFolder(&pItem);
    if (FAILED(hr)) return S_OK;
    LPOLESTR pwsz = NULL;
    hr = pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pwsz);
    pItem->Release();
    if (FAILED(hr)) return S_OK;
    pDlg->SetTitle(pwsz);
    CoTaskMemFree(pwsz);
    return S_OK;
}

IFACEMETHODIMP CFileDialogEventHandler::OnFileOk(IFileDialog *pDlg)
{
    TCHAR path[MAX_PATH] = {0};
    IShellItem *pItem = NULL;
    HRESULT hr = pDlg->GetCurrentSelection(&pItem);
    if (SUCCEEDED(hr)) {
        LPOLESTR pwsz = NULL;
        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);
        if (SUCCEEDED(hr)) {
            LOG(pwsz);
            //PathCchRemoveFileSpec
            _tcscpy(path, pwsz);
            PathRemoveFileSpec(path);
            SetCurrentDirectory(path);
            launch_file(g_Globals._hwndDesktop, pwsz);
            CoTaskMemFree(pwsz);
        }
        pItem->Release();
    }
    return S_FALSE;
}

//
//   FUNCTION: CFileDialogEventHandler_CreateInstance(REFIID, void**)
//
//   PURPOSE:  CFileDialogEventHandler instance creation helper function.
//
HRESULT CFileDialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
    *ppv = NULL;
    CFileDialogEventHandler* pFileDialogEventHandler =
        new CFileDialogEventHandler();
    HRESULT hr = pFileDialogEventHandler ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr)) {
        hr = pFileDialogEventHandler->QueryInterface(riid, ppv);
        pFileDialogEventHandler->Release();
    }
    return hr;
}

HWND WINAPI MyGetShellWindow()
{
    return 0;
}

/*
    Hook the GetShellWindow() function for forcing this return 0,
    so the SHChangeNotifyRegisterThread() will create a thread for
    dealing with shell change notifification that would autorefresh
    the OpenFileDialg's content when you add, rename, copy or delete
    files/folders.

    hackercode for x86 (5 bytes):
    --------------------------------
    JMP <user function addr> - <hook system function addr> - 5
    --------------------------------

    hackercode for x64 (12 bytes):
    --------------------------------
    mov rax, <user function addr>
    push rax
    ret
    --------------------------------

 */
static void HookGetShellWindow()
{
    static HMODULE hUser32Dll = NULL;
    static BOOL hookAPI = JCFG2_DEF("JS_FILEEXPLORER", "hook_GetShellWindow", true).ToBool();
#ifdef _WIN64
    int shellcode_len = 12;
#else
    int shellcode_len = 5;
#endif
    if (!hookAPI || hUser32Dll) return;
    hUser32Dll = GetModuleHandle(TEXT("USER32"));
    if (!hUser32Dll) return;
    void *pAddr = NULL;
    pAddr = GetProcAddress(hUser32Dll, "GetShellWindow");

    DWORD lpflOldProtect = 0;
    if (!pAddr) return;
    if (VirtualProtect(pAddr, 32, PAGE_EXECUTE_READWRITE, &lpflOldProtect)) {
        if (lpflOldProtect < shellcode_len) return;
#ifdef _WIN64
        BYTE HackCode[12] = { 0x48, 0xb8 };
        HackCode[10] = 0x50;
        HackCode[11] = 0xc3;
        long long dwJmpAddr = (long long)MyGetShellWindow;
        memcpy(&HackCode[2], &dwJmpAddr, 8);
#else
        BYTE HackCode[5] = { 0xe9 };
        DWORD dwJmpAddr = (DWORD)MyGetShellWindow - (DWORD)pAddr - 5;
        memcpy(&HackCode[1], &dwJmpAddr, 4);
#endif
        memcpy(((unsigned char *)pAddr), HackCode, shellcode_len);
    }
    return;
}

/*
_SCNGetWindow
00007FFF7E053608
...
00007FFF7E053643  e8 xx xx xx xx call        _GetDesktop (07FFF7DFBD95Ch)
00007FFF7E053648  48 85 c0       test        rax,rax
00007FFF7E05364B  74             je         _SCNGetWindow+6Ch (07FFF7E053674h)
                  74->EB ---> je->jmp
*/
/*
static void Shell32DllHacker()
{
    static HMODULE hShell32Dll = NULL;
    static BOOL shell32_hacker = JCFG2_DEF("JS_FILEEXPLORER", "shell32_hacker", false).ToBool();
#ifdef _WIN64
    static String strRVAddr = JCFG2_DEF("JS_FILEEXPLORER", "shell32x64_hacker_addr", TEXT("0x0")).ToString();
    static String strCode = JCFG2_DEF("JS_FILEEXPLORER", "shell32x64_hacker_code", TEXT("\u00EB")).ToString();
#else
    static String strRVAddr = JCFG2_DEF("JS_FILEEXPLORER", "shell32x86_hacker_addr", TEXT("0x0")).ToString();
    static String strCode = JCFG2_DEF("JS_FILEEXPLORER", "shell32x86_hacker_code", TEXT("\u00EB")).ToString();
#endif
    if (!shell32_hacker || hShell32Dll) return;
    hShell32Dll = GetModuleHandle(TEXT("SHELL32"));
    if (!hShell32Dll) return;
    void *pAddr = NULL;
    //pAddr = GetProcAddress(hShell32Dll, "SHChangeNotifyRegister");
    int iRVAddr = std::stoi(strRVAddr, 0, 16);
    pAddr = (unsigned char *)hShell32Dll + iRVAddr + 0xC00;
    DWORD lpflOldProtect = 0;
    if (!pAddr) return;
    if (VirtualProtect(pAddr, 16, PAGE_EXECUTE_READWRITE, &lpflOldProtect)) {
        size_t iCodeLen = strCode.length();
        LPCTSTR pCode = strCode.c_str();
        //a little trick for UNICODE
        for (int i = 0;i < iCodeLen;i++) {
            memcpy(((unsigned char *)pAddr) + i, pCode + i, 1);
        }
    }
    return;
}
*/

#ifndef ROSSHELL
void explorer_show_frame(int cmdShow, LPTSTR lpCmdLine)
{
    ExplorerCmd cmd;

    if (g_Globals._hMainWnd) {
        if (IsIconic(g_Globals._hMainWnd))
            ShowWindow(g_Globals._hMainWnd, SW_RESTORE);
        else
            SetForegroundWindow(g_Globals._hMainWnd);

        return;
    }

    g_Globals._prescan_nodes = false;

    cmd._mdi = true;
    cmd._cmdShow = cmdShow;

    if (lpCmdLine)
        cmd.ParseCmdLine(lpCmdLine);

    if (g_Globals._hwndDesktopBar == (HWND)0) {
        HookGetShellWindow();
    }

    // create main window
    FileExplorerWindow::Create(NULL, cmd._path.c_str());
}
#endif

int explorer_open_frame(int cmdShow, LPTSTR lpCmdLine, int mode)
{
    String explorer_path = JCFG2_DEF("JS_FILEEXPLORER", "3rd_filename", TEXT("")).ToString();
    BOOL seperate_mode = JCFG2_DEF("JS_FILEEXPLORER", "seperate_mode", true).ToBool();
    String explorer_open;
    String explorer_parameters;
    if (lpCmdLine == NULL) lpCmdLine = TEXT("");
    explorer_parameters = lpCmdLine;
    if (explorer_path.empty()) {
        if (!g_Globals._desktop_mode || !seperate_mode) {
            explorer_show_frame(cmdShow, lpCmdLine);
            return 1;
        }

        explorer_path = JVAR("JVAR_MODULEPATH").ToString() + TEXT("\\") + JVAR("JVAR_MODULENAME").ToString();

        if (mode == EXPLORER_OPEN_NORMAL) {
            explorer_open = JCFG2_DEF("JS_FILEEXPLORER", "open_arguments", TEXT("%s")).ToString();
        }
        else if (mode == EXPLORER_OPEN_QUICKLAUNCH) {
            explorer_open = JCFG2_DEF("JS_QUICKLAUNCH", "open_arguments", TEXT("")).ToString();
        }
        explorer_parameters = FmtString(explorer_open, lpCmdLine);
        launch_file(g_Globals._hwndDesktop, explorer_path.c_str(), cmdShow, explorer_parameters.c_str());
        return 1;
    }

    if (mode == EXPLORER_OPEN_NORMAL) {
        explorer_open = JCFG2_DEF("JS_FILEEXPLORER", "3rd_open_arguments", TEXT("%s")).ToString();
    }
    else if (mode == EXPLORER_OPEN_QUICKLAUNCH) {
        explorer_open = JCFG2_DEF("JS_QUICKLAUNCH", "3rd_open_arguments", TEXT("")).ToString();
    }

    explorer_parameters = FmtString(explorer_open, lpCmdLine);
    launch_file(g_Globals._hwndDesktop, explorer_path.c_str(), cmdShow, explorer_parameters.c_str());
    return 0;
}

int OpenShellFolders(HWND hwnd, LPIDA pida)
{
    int cnt = 0;

    LPCITEMIDLIST parent_pidl = (LPCITEMIDLIST)((LPBYTE)pida + pida->aoffset[0]);
    ShellFolder parentfolder(parent_pidl);
    LOG(FmtString(TEXT("FileExplorer::OpenShellFolders(): parent_pidl=%s"), (LPCTSTR)FileSysShellPath(parent_pidl)));

    for (int i = pida->cidl; i > 0; --i) {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)((LPBYTE)pida + pida->aoffset[i]);

        SFGAOF attribs = SFGAO_FOLDER;
        HRESULT hr = parentfolder->GetAttributesOf(1, &pidl, &attribs);

        String explorer_path = JCFG2_DEF("JS_FILEEXPLORER", "3rd_filename", TEXT("%s")).ToString();
        int mutil_open_interval = JCFG2_DEF("JS_FILEEXPLORER", "open_interval", 500).ToInt();
        String explorer_open;
        if (!explorer_path.empty()) {
            explorer_open = JCFG2_DEF("JS_DESKTOP", "3rd_open_arguments", TEXT("%s")).ToString();
        } else {
            explorer_open = JCFG2_DEF("JS_DESKTOP", "open_arguments", TEXT("%s")).ToString();
        }

        if (SUCCEEDED(hr)) {
            if (attribs & SFGAO_FOLDER) {
                try {
                    ShellPath pidl_abs = ShellPath(pidl).create_absolute_pidl(parent_pidl);
                    LOG(FmtString(TEXT("FileExplorer::OpenShellFolders(): pidl_abs=%s"), (LPCTSTR)FileSysShellPath(pidl_abs)));

                    String explorer_parameters;
                    SHDESCRIPTIONID desc = { 0 };
                    SHGetDataFromIDList(parentfolder, pidl, SHGDFIL_DESCRIPTIONID, &desc, sizeof(desc));
                    if (desc.dwDescriptionId == SHDID_ROOT_REGITEM) {
                        /*LPOLESTR strclsid = NULL;
                        StringFromCLSID(desc.clsid, &strclsid);
                        CoTaskMemFree(strclsid);*/
                        if (IsEqualCLSID(desc.clsid, CLSID_MyComputer)) {
                            explorer_parameters = FmtString(explorer_open, CLSID_MyComputerName);
                        }
                        else if (IsEqualCLSID(desc.clsid, CLSID_RecycleBin)) {
                            explorer_parameters = FmtString(explorer_open, CLSID_RecycleBinName);
                        }
                        else if (IsEqualCLSID(desc.clsid, CLSID_MyDocuments)) {
                            explorer_parameters = FmtString(explorer_open, CLSID_MyDocumentsName);
                        }
                        else if (IsEqualCLSID(desc.clsid, CLSID_UsersFiles)) {
                            explorer_parameters = FmtString(explorer_open, CLSID_UsersFilesName);
                        }
                    } else {
                        explorer_parameters = FmtString(explorer_open, (LPCTSTR)FileSysShellPath(pidl_abs));
                    }
                    BOOL seperate_mode = JCFG2_DEF("JS_FILEEXPLORER", "seperate_mode", true).ToBool();
                    if (explorer_path.empty() && seperate_mode) {
                        explorer_path = JVAR("JVAR_MODULEPATH").ToString() + TEXT("\\") + JVAR("JVAR_MODULENAME").ToString();
                    }
                    if (!explorer_path.empty()) {
                        launch_file(g_Globals._hwndDesktop, explorer_path.c_str(), SW_SHOWNORMAL, explorer_parameters.c_str());
                    } else {
                        FileExplorerWindow::Create(hwnd, explorer_parameters);
                        if (i > 1) Sleep(mutil_open_interval);
                    }
                    ++cnt;
                } catch (COMException &e) {
                    HandleException(e, g_Globals._hMainWnd);
                }
            }
        }
    }
    return cnt;
}

