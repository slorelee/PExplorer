
#include <Windows.h>
#include <Shlwapi.h>
#include "../utility/utility.h"
#include "../utility/window.h"
#include "../globals.h"
#include "../jconfig/jcfg.h"
#include "features.h"

extern ExplorerGlobals g_Globals;

EXTERN_C {
    extern void explorer_show_frame(int cmdshow, LPTSTR lpCmdLine = NULL);
}

void WaitForShellTerminated()
{
    HWND shellWindow = GetShellWindow();

    if (shellWindow) {
        DWORD pid;
        GetWindowThreadProcessId(shellWindow, &pid);
        if (pid <= 0) return;
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        WaitForSingleObject(hProcess, INFINITE); //INFINITE
        CloseHandle(hProcess);
    }
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

BOOL FolderExists(const TCHAR *strPath)
{
    WIN32_FIND_DATA wfd = { 0 };
    HANDLE hFile = FindFirstFile(strPath, &wfd);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    FindClose(hFile);
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;
    }
    return FALSE;
}

void ChangeUserProfileEnv()
{
    //HKLM\Software\Microsoft\Windows NT\CurrentVersion\ProfileList\S-1-5-18\ProfileImagePath
    TCHAR userprofile[MAX_PATH + 1] = { 0 };
    TCHAR desktop[MAX_PATH + 1] = { 0 };
    if (!g_Globals._isWinPE) return;

    GetEnvironmentVariable(TEXT("USERPROFILE"), userprofile, MAX_PATH);
    _tcscpy(desktop, userprofile);
    _tcscat(desktop, TEXT("\\Desktop"));
    if (FolderExists(desktop)) return;

    if (_tcsicmp(userprofile, TEXT("X:\\windows\\system32\\config\\systemprofile")) == 0) {
        _tcscpy(userprofile, TEXT("X:\\Users\\Default"));
        SetEnvironmentVariable(TEXT("USERPROFILE"), userprofile);
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

void RegistAppPath() {
    RegSetValue(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\WinXShell.exe"),
        REG_SZ, (JVAR("JVAR_MODULEFILENAME").ToString().c_str()), 0);

    RegSetValue(HKEY_CURRENT_USER,
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\WinXShell.exe"),
        REG_SZ, (JVAR("JVAR_MODULEFILENAME").ToString().c_str()), 0);
}

BOOL hasMSExplorer()
{
    TCHAR buff[32] = { 0 };
    static TCHAR file[] = TEXT("?:\\Windows\\explorer.exe");
    if (file[0] == TEXT('?')) {
        GetEnvironmentVariable(TEXT("SystemDrive"), buff, 10);
        file[0] = buff[0];
    }
    if (PathFileExists(file)) return TRUE;
    return FALSE;
}

BOOL isWinXShellAsShell()
{
    if (g_Globals._isShell) return TRUE;
    if (FindWindow(WINXSHELL_SHELLWINDOW, NULL)) return TRUE;
    return FALSE;
}

String getShellTheme() {
    if (!hasMSExplorer() || isWinXShellAsShell()) {
        JVAR("ShellTheme") = JCFG2("JS_TASKBAR", "theme").ToString();
        return JVAR("ShellTheme").ToString();
    }
    JVAR("ShellTheme") = TEXT("default");

    TCHAR theme[MAX_PATH + 1] = { 0 };
    DWORD dw =  GetEnvironmentVariable(TEXT("WINXSHELL_SHELLTHEME"), theme, MAX_PATH);
    if (dw != 0)  JVAR("ShellTheme") = theme;

    DWORD type = REG_DWORD, value = 0, size = sizeof(DWORD);
    SHGetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        TEXT("SystemUsesLightTheme"), &type, &value, &size);
    if (value == 1) JVAR("ShellTheme") = TEXT("light");
    return JVAR("ShellTheme").ToString();
}

static BOOL isTarget(LPCTSTR cmd, LPCTSTR target) {
    String str_cmd = cmd;
    String str_target = target;
    str_cmd.toLower();
    str_target.toLower();
    if (str_cmd == str_target) {
        return TRUE;
    } else if (str_cmd == TEXT("wxs-open:") + str_target) {
        return TRUE;
    }
    return FALSE;
}

void wxsOpen(LPTSTR cmd) {
    if (isTarget(cmd, TEXT("System"))) {
        if (hasMSExplorer()) {
            launch_file(g_Globals._hwndDesktop, TEXT("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\5\\::{BB06C0E4-D293-4F75-8A90-CB05B6477EEE}"));
        } else {
            gLuaCall("wxs_ui", TEXT("systeminfo"), TEXT(""));
        }
    } else if (isTarget(cmd, TEXT("NetworkConnections"))) {
        // ncpa.cpl
        launch_file(g_Globals._hwndDesktop, TEXT("::{7007ACC7-3202-11D1-AAD2-00805FC1270E}"));
    } else if (isTarget(cmd, TEXT("NetworkCenter"))) {
        launch_file(g_Globals._hwndDesktop, TEXT("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\3\\::{8E908FC9-BECC-40F6-915B-F4CA0E70D03D}"));
    } else if (isTarget(cmd, TEXT("Printers"))) {
        launch_file(g_Globals._hwndDesktop, TEXT("::{2227A280-3AEA-1069-A2DE-08002B30309D}"));
    } else if (isTarget(cmd, TEXT("UsersLibraries"))) {
        launch_file(g_Globals._hwndDesktop, TEXT("::{031E4825-7B94-4DC3-B131-E946B44C8DD5}"));
    } else if (isTarget(cmd, TEXT("DevicesAndPrinters"))) {
        launch_file(g_Globals._hwndDesktop, TEXT("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\2\\::{A8A91A66-3A7D-4424-8D24-04E180695C7A}"));
    } else {
        gLuaCall("wxs_open", cmd, TEXT(""));
    }
}

TOKEN_PRIVILEGES *AdjustToken()
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES *p = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        size_t s = sizeof(TOKEN_PRIVILEGES) + 2 * sizeof(LUID_AND_ATTRIBUTES);
        p = (PTOKEN_PRIVILEGES)malloc(s);

        if (!LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &(p->Privileges[0].Luid)) ||
            !LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &(p->Privileges[1].Luid)) ||
            !LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &(p->Privileges[2].Luid))) {
            printf("failed to LookupPrivilegeValue error code : %d \r\n", GetLastError());
            free(p);
            return NULL;
        }
        p->PrivilegeCount = 3;

        for (int i = 0; i < 3; ++i) {
            p->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        }

        if (!AdjustTokenPrivileges(hToken, FALSE, p, s, NULL, NULL) || GetLastError() != ERROR_SUCCESS) {
            printf("AdjustTokenPrivileges failed! error code : %d \r\n", GetLastError());
            free(p);
            return NULL;
        }
    } else {
        printf("Open process token failed! error code : %d \r\n", GetLastError());
        return NULL;
    }
    return p;
}

BOOL IsUEFIMode() {
    TCHAR buff[1024] = { 0 };

    DWORD dw = 0;
    TOKEN_PRIVILEGES *token = AdjustToken();
    if (!token) return FALSE;

    dw = GetFirmwareEnvironmentVariable(TEXT(""),
        TEXT("{00000000-0000-0000-0000-000000000000}"), buff, sizeof(buff));
    dw = GetLastError();
    free(token);
    if (dw == ERROR_NOACCESS) return TRUE;
    //dw = ERROR_INVALID_FUNCTION for legacy BIOS
    return FALSE;
}

BOOL isWinPE()
{
    DWORD type = REG_DWORD;
    TCHAR value[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control"),
        TEXT("SystemStartOptions"), &type, &value, &size);
    if (type == REG_SZ) {
        if (StrStr((TCHAR*)value, TEXT("MININT")) != NULL) {
            return TRUE;
        }
    }
    return FALSE;
}

