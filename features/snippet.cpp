
#include <Windows.h>
#include <Shlwapi.h>
#include "../utility/utility.h"
#include "../utility/window.h"
#include "../globals.h"

extern ExplorerGlobals g_Globals;

EXTERN_C {
    extern void explorer_show_frame(int cmdshow, LPTSTR lpCmdLine = NULL);
}

void CloseShellProcess()
{
    HWND shellWindow = GetShellWindow();

    if (shellWindow) {
        DWORD pid;

        // terminate shell process for NT like systems
        GetWindowThreadProcessId(shellWindow, &pid);
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

        // On Win 9x it's sufficient to destroy the shell window.
        DestroyWindow(shellWindow);

        if (TerminateProcess(hProcess, 0))
            WaitForSingleObject(hProcess, 10000); //INFINITE

        CloseHandle(hProcess);
    }
}

void ChangeUserProfileEnv()
{
    //HKLM\Software\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-18\ProfileImagePath
    TCHAR userprofile[MAX_PATH + 1] = { 0 };
    if (g_Globals._isWinPE) {
        GetEnvironmentVariable(TEXT("USERPROFILE"), userprofile, MAX_PATH);
        if (_tcsicmp(userprofile, TEXT("X:\\windows\\system32\\config\\systemprofile")) == 0) {
            _tcscpy(userprofile, TEXT("X:\\Users\\Default"));
            SetEnvironmentVariable(TEXT("USERPROFILE"), userprofile);
        }
    }
}

static void ocf(const TCHAR *szPath)
{
    LPITEMIDLIST  pidl;
    LPCITEMIDLIST cpidl_dir;
    LPCITEMIDLIST cpidl_file;
    LPSHELLFOLDER pDesktopFolder;
    ULONG         chEaten;
    ULONG         dwAttributes;
    HRESULT       hr;
    TCHAR          szDirPath[MAX_PATH];

    // 
    // Get a pointer to the Desktop's IShellFolder interface.
    // 
    if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder))) {
        StrCpy(szDirPath, szPath);
        PathRemoveFileSpec(szDirPath);

        hr = pDesktopFolder->ParseDisplayName(NULL, 0, szDirPath, &chEaten, &pidl, &dwAttributes);
        if (FAILED(hr)) {
            pDesktopFolder->Release();
            return;
            // Handle error.
        }
        cpidl_dir = pidl;

        // 
        // Convert the path to an ITEMIDLIST.
        // 
        hr = pDesktopFolder->ParseDisplayName(NULL, 0, (LPWSTR)szPath, &chEaten, &pidl, &dwAttributes);
        if (FAILED(hr)) {
            pDesktopFolder->Release();
            return;
            // Handle error.
        }
        cpidl_file = pidl;
        HRESULT RE = CoInitialize(NULL);
        int re = SHOpenFolderAndSelectItems(cpidl_dir, 1, &cpidl_file, NULL);

        //
        // pidl now contains a pointer to an ITEMIDLIST.
        // This ITEMIDLIST needs to be freed using the IMalloc allocator
        // returned from SHGetMalloc().
        //
        //release the desktop folder object
        pDesktopFolder->Release();
    }
}

void OpenContainingFolder(LPTSTR pszCmdline)
{
    String cmdline = pszCmdline;
    String lnkfile;
    size_t pos = cmdline.find(_T("-ocf"));
    lnkfile = cmdline.substr(pos + 5);
    if (lnkfile[0U] == TEXT('\"')) lnkfile = lnkfile.substr(1, lnkfile.length() - 2);
    TCHAR path[MAX_PATH];
    GetShortcutPath(lnkfile.c_str(), path, MAX_PATH);

    String strPath = path;
    if (g_Globals._lua) {
        if (g_Globals._lua->hasfunc("do_ocf")) {
            g_Globals._lua->call("do_ocf", lnkfile, strPath);
            return;
        }
    }

    //if (!PathIsDirectory(path)) {
    //size_t nPos = strPath.rfind(TEXT('\\'));
    //if (nPos == String::npos) return;
    //strPath = strPath.substr(0, nPos);
    //}
    if (cmdline.find(_T("-explorer")) != String::npos) {
        ocf(strPath.c_str());
        return;
    }
    strPath = TEXT("/select,") + strPath;
    explorer_show_frame(SW_SHOWNORMAL, (LPTSTR)(strPath.c_str()));
    Window::MessageLoop();
    return;

}

extern string_t GetParameter(string_t cmdline, string_t key, BOOL hasValue = TRUE);

static DWORD GetColorRef(String color)
{
    DWORD clrColor = 0;
    LPCTSTR pstrValue = color.c_str();
    LPTSTR pstr = NULL;
    if (color != TEXT("") && color.length() >= 8) {
        pstrValue = ::CharNext(pstrValue);
        pstrValue = ::CharNext(pstrValue);
        TCHAR buff[10] = { 0 };
        buff[0] = pstrValue[4]; buff[1] = pstrValue[5];
        buff[2] = pstrValue[2]; buff[3] = pstrValue[3];
        buff[4] = pstrValue[0]; buff[5] = pstrValue[1];
        clrColor = _tcstoul(buff, &pstr, 16);
        return clrColor;
    }
    return 0;
}

void UpdateSysColor(LPTSTR pszCmdline)
{
    String cmdline = pszCmdline;
    cmdline += TEXT(" ");
    String hl = GetParameter(cmdline, TEXT("color_highlight"), TRUE);
    INT elements[2] = { COLOR_HIGHLIGHT, COLOR_HOTLIGHT };
    if (hl != TEXT("")) {
        DWORD clrColor = GetColorRef(hl);
        SetSysColors(1, elements, &clrColor);
    }

    hl = GetParameter(cmdline, TEXT("color_selection"), TRUE);
    if (hl != TEXT("")) {
        DWORD clrColor = GetColorRef(hl);
        SetSysColors(1, elements + 1, &clrColor);
    }
}

