/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
// Explorer clone
//
// utility.cpp
//
// Martin Fuchs, 23.07.2003
//


#include <precomp.h>

//#include <shellapi.h>

#include <time.h>
#include <sstream>


DWORD WINAPI Thread::ThreadProc(void *para)
{
    Thread *pThis = (Thread *) para;

    int ret = pThis->Run();

    pThis->_alive = false;

    return ret;
}


void CenterWindow(HWND hwnd)
{
    RECT rt, prt;
    GetWindowRect(hwnd, &rt);

    DWORD style;
    HWND owner = 0;

    for (HWND wh = hwnd; (wh = GetWindow(wh, GW_OWNER)) != 0;)
        if (((style = GetWindowStyle(wh))&WS_VISIBLE) && !(style & WS_MINIMIZE))
        {owner = wh; break;}

    if (owner)
        GetWindowRect(owner, &prt);
    else
        SystemParametersInfo(SPI_GETWORKAREA, 0, &prt, 0);  //@@ GetDesktopWindow() wäre auch hilfreich.

    SetWindowPos(hwnd, 0, (prt.left + prt.right + rt.left - rt.right) / 2,
                 (prt.top + prt.bottom + rt.top - rt.bottom) / 2, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    MoveVisible(hwnd);
}

void MoveVisible(HWND hwnd)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int left = rc.left, top = rc.top;

    int xmax = GetSystemMetrics(SM_CXSCREEN);
    int ymax = GetSystemMetrics(SM_CYSCREEN);

    if (rc.left < 0)
        rc.left = 0;
    else if (rc.right > xmax)
        if ((rc.left -= rc.right - xmax) < 0)
            rc.left = 0;

    if (rc.top < 0)
        rc.top = 0;
    else if (rc.bottom > ymax)
        if ((rc.top -= rc.bottom - ymax) < 0)
            rc.top = 0;

    if (rc.left != left || rc.top != top)
        SetWindowPos(hwnd, 0, rc.left, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}


void display_error(HWND hwnd, DWORD error)  //@@ CONTEXT mit ausgeben -> display_error(HWND hwnd, const Exception& e)
{
    PTSTR msg;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL)) {
        LOG(FmtString(TEXT("display_error(%#x): %s"), error, msg));

        SetLastError(0);
        MessageBox(hwnd, msg, TEXT("WinXShell"), MB_OK);

        if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
            MessageBox(0, msg, TEXT("WinXShell"), MB_OK);
    } else {
        LOG(FmtString(TEXT("Unknown Error %#x"), error));

        FmtString msg(TEXT("Unknown Error %#x"), error);

        SetLastError(0);
        MessageBox(hwnd, msg, TEXT("WinXShell"), MB_OK);

        if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
            MessageBox(0, msg, TEXT("WinXShell"), MB_OK);
    }

    LocalFree(msg);
}


Context Context::s_main("-NO-CONTEXT-");
Context *Context::s_current = &Context::s_main;

String Context::toString() const
{
    String str = _ctx;

    if (!_obj.empty())
        str.appendf(TEXT("\nObject: %s"), (LPCTSTR)_obj);

    return str;
}

String Context::getStackTrace() const
{
    ostringstream str;

    str << "Context Trace:\n";

    for (const Context *p = this; p && p != &s_main; p = p->_last) {
        str << "- " << p->_ctx;

        if (!p->_obj.empty())
            str << " obj=" << ANS(p->_obj);

        str << '\n';
    }

    return str.str();
}


BOOL time_to_filetime(const time_t *t, FILETIME *ftime)
{
#if defined(__STDC_WANT_SECURE_LIB__) && defined(_MS_VER)
    SYSTEMTIME stime;
    struct tm tm_;
    struct tm *tm = &tm_;

    if (gmtime_s(tm, t) != 0)
        return FALSE;
#else
    struct tm *tm = gmtime(t);
    SYSTEMTIME stime;

    if (!tm)
        return FALSE;
#endif

    stime.wYear = tm->tm_year + 1900;
    stime.wMonth = tm->tm_mon + 1;
    stime.wDayOfWeek = (WORD) - 1;
    stime.wDay = tm->tm_mday;
    stime.wHour = tm->tm_hour;
    stime.wMinute = tm->tm_min;
    stime.wSecond = tm->tm_sec;
    stime.wMilliseconds = 0;

    return SystemTimeToFileTime(&stime, ftime);
}


BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow, LPCTSTR parameters)
{
    CONTEXT("launch_file()");

    HINSTANCE hinst = ShellExecute(hwnd, NULL/*operation*/, cmd, parameters, NULL/*dir*/, nCmdShow);

    if ((int)hinst <= 32) {
        display_error(hwnd, GetLastError());
        return FALSE;
    }

    return TRUE;
}

#ifdef UNICODE
BOOL launch_fileA(HWND hwnd, LPSTR cmd, UINT nCmdShow, LPCSTR parameters)
{
    HINSTANCE hinst = ShellExecuteA(hwnd, NULL/*operation*/, cmd, parameters, NULL/*dir*/, nCmdShow);

    if ((int)hinst <= 32) {
        display_error(hwnd, GetLastError());
        return FALSE;
    }

    return TRUE;
}
#endif

void GetShortcutPath(const TCHAR *lnk, TCHAR *path, DWORD cchBuffer)
{
    IShellLink *psl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        IPersistFile *ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Load(lnk, STGM_READ);
            if (SUCCEEDED(hr)) {
                WIN32_FIND_DATA wfd;
                psl->GetPath(path, cchBuffer, &wfd, SLGP_UNCPRIORITY | SLGP_RAWPATH);
            }
            ppf->Release();
        }
        psl->Release();
    }
}

#include "UNIBASE.h"

TCHAR *CompletePath(TCHAR *target, TCHAR *out)
{
    TCHAR buff[MAX_PATH] = { 0 };
    ExpandEnvironmentStrings(target, out, MAX_PATH);
    if (PathFileExists(out)) return out;
    StrCpy(buff, out);
    if (SearchPath(NULL, buff, NULL, MAX_PATH, out, NULL)) {
        return out;
     }
     return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Create shortcut
HRESULT CreateShortcut(PTSTR lnk, PTSTR target,
    PTSTR param = NULL, PTSTR icon = NULL,
    int iIcon = 0, int iShowCmd = SW_SHOWNORMAL)
{
    if (target == NULL) {
        return ERROR_PATH_NOT_FOUND;
    }

    // Search target
    TCHAR tzTarget[MAX_PATH];
    target = CompletePath(target, tzTarget);
    if (!target) return ERROR_PATH_NOT_FOUND;

    // Create shortcut
    IShellLink *pLink = NULL;
    CoInitialize(NULL);
    HRESULT hResult = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (PVOID *)&pLink);
    if (hResult == S_OK) {
        IPersistFile *pFile = NULL;
        hResult = pLink->QueryInterface(IID_IPersistFile, (PVOID *)&pFile);
        if (hResult == S_OK) {
            // Shortcut settings
            if (iShowCmd > SW_SHOWNORMAL) hResult = pLink->SetShowCmd(iShowCmd);

            hResult = pLink->SetPath(target);
            hResult = pLink->SetArguments(param);
            hResult = pLink->SetIconLocation(icon, iIcon);

            if (DirSplitPath(target) != target) {
                hResult = pLink->SetWorkingDirectory(target);
            }

            // Save link
            TCHAR tzLink[MAX_PATH];
            ExpandEnvironmentStrings(lnk, tzLink, MAX_PATH);
            DirCreate(tzLink);
            hResult = pFile->Save(tzLink, FALSE);
            pFile->Release();
        }
        pLink->Release();
    }
    CoUninitialize();
    return hResult;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL ShellExecuteWithSEInfo(SHELLEXECUTEINFO &ei, TCHAR *cmd)
{
    TCHAR paramSep = _T(' ');
    TCHAR *cp = cmd;
    if (cmd[0] == _T('\"') || cmd[0] == _T('\'')) {
        cp = cmd + 1;
        paramSep = cmd[0];
    }

    {
        while (*cp != _T('\0') && *cp != paramSep) {
            if (paramSep != _T(' ')) *(cp - 1) = *cp;
            cp++;
        }
        if (paramSep != _T(' ')) {
            *(cp - 1) = _T('\0');
            cp++;
        } else {
            *cp = _T('\0');
        }
        ei.lpParameters = cp + 1;
    }
    return ShellExecuteEx(&ei);
}

// Execute command
DWORD Exec(PTSTR ptzCmd, BOOL bWait, INT iShowCmd, PTSTR ptzVerb)
{
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    DWORD dwExitCode = 0;

    TCHAR tzExpandCmd[MAX_PATH * 10];
    ExpandEnvironmentStrings(ptzCmd, tzExpandCmd, MAX_PATH * 10);

    BOOL bResult = FALSE;
    if (ptzVerb && ptzVerb[0] != _T('\0')) {
        SHELLEXECUTEINFO ei = { sizeof(ei) };
        ei.fMask = SEE_MASK_INVOKEIDLIST;
        ei.hwnd = NULL;
        ei.nShow = iShowCmd;
        ei.lpVerb = ptzVerb;
        ei.lpFile = tzExpandCmd;

        bResult = ShellExecuteWithSEInfo(ei, tzExpandCmd);
        if (!bResult) return S_FALSE;

        hProcess = ei.hProcess;
    } else {
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi;
        si.cb = sizeof(STARTUPINFO);
        si.lpDesktop = TEXT("WinSta0\\Default");
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = iShowCmd;
        bResult = CreateProcess(NULL, tzExpandCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (!bResult) return S_FALSE;

        hProcess = pi.hProcess;
        hThread = pi.hThread;
    }

    if (bWait && hProcess) {
        WaitForSingleObject(hProcess, INFINITE);
        GetExitCodeProcess(hProcess, &dwExitCode);
    }

    if (hThread) CloseHandle(hThread);
    if (hProcess) CloseHandle(hProcess);
    return dwExitCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CommandHook(HWND hwnd, const TCHAR *act, const TCHAR *sect)
{
    String cmd = TEXT("");
    INT showflags = SW_SHOWNORMAL;
    String parameters = TEXT("");
    if (!hwnd) return 0;
    cmd = JCFG_CMDW(sect, act, "command", TEXT("")).ToString();
    if (cmd != TEXT("")) {
        showflags = JCFG_CMDW(sect, act, "showflags", showflags).ToInt();
        parameters = JCFG_CMDW(sect, act, "parameters", TEXT("")).ToString();
        launch_file(hwnd, cmd, showflags, parameters);
        return 1;
    }
    return 0;
}

BOOL HandleEnvChangeBroadcast(LPARAM lparam)
{
    static BOOL(WINAPI *RegenerateUserEnvironment)(void **, BOOL) = NULL;
    //
    // Check if the user's environment variables have changed, if so
    // regenerate the environment, so that new apps started from
    // taskman will have the latest environment.
    //
    if (lparam && (!lstrcmpi((LPTSTR)lparam, (LPTSTR)TEXT("Environment")))) {
        PVOID pEnv;
        if (!RegenerateUserEnvironment) {
            RegenerateUserEnvironment = (BOOL(WINAPI *)(void **, BOOL))
                GetProcAddress(GetModuleHandle(TEXT("SHELL32")), "RegenerateUserEnvironment");
        }
        if (RegenerateUserEnvironment) {
            RegenerateUserEnvironment(&pEnv, TRUE);
        }
        return TRUE;
    }
    return FALSE;
}

/* search for already running instance */

static int g_foundPrevInstance = 0;

static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam)
{
    TCHAR cls[128];

    GetClassName(hwnd, cls, 128);

    if (!lstrcmp(cls, (LPCTSTR)lparam)) {
        g_foundPrevInstance++;
        return FALSE;
    }

    return TRUE;
}

/* search for window of given class name to allow only one running instance */
int find_window_class(LPCTSTR classname)
{
    EnumWindows(EnumWndProc, (LPARAM)classname);

    if (g_foundPrevInstance)
        return 1;

    return 0;
}


String get_windows_version_str()
{
    OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
    BOOL osvie_val;
    String str;

    if (!(osvie_val = GetVersionEx((OSVERSIONINFO *)&osvi))) {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (!GetVersionEx((OSVERSIONINFO *)&osvi))
            return TEXT("???");
    }

    switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
#ifdef __REACTOS__  // This work around can be removed if ReactOS gets a unique version number.
        str = TEXT("ReactOS");
#else
        if (osvi.dwMajorVersion <= 4)
            str = TEXT("Microsoft Windows NT");
        else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
            str = TEXT("Microsoft Windows 2000");
        else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
            str = TEXT("Microsoft Windows XP");
#endif

        if (osvie_val) {
            if (osvi.wProductType == VER_NT_WORKSTATION) {
                if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
                    str += TEXT(" Personal");
                else
                    str += TEXT(" Professional");
            } else if (osvi.wProductType == VER_NT_SERVER) {
                if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                    str += TEXT(" DataCenter Server");
                else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                    str += TEXT(" Advanced Server");
                else
                    str += TEXT(" Server");
            } else if (osvi.wProductType == VER_NT_DOMAIN_CONTROLLER) {
                str += TEXT(" Domain Controller");
            }
        } else {
            TCHAR type[80];
            DWORD dwBufLen;
            HKEY hkey;

            if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_QUERY_VALUE, &hkey)) {
                RegQueryValueEx(hkey, TEXT("ProductType"), NULL, NULL, (LPBYTE)type, &dwBufLen);
                RegCloseKey(hkey);

                if (!_tcsicmp(TEXT("WINNT"), type))
                    str += TEXT(" Workstation");
                else if (!_tcsicmp(TEXT("LANMANNT"), type))
                    str += TEXT(" Server");
                else if (!_tcsicmp(TEXT("SERVERNT"), type))
                    str += TEXT(" Advanced Server");
            }
        }
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        if (osvi.dwMajorVersion > 4 ||
            (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion > 0)) {
            if (osvi.dwMinorVersion == 90)
                str = TEXT("Microsoft Windows ME");
            else
                str = TEXT("Microsoft Windows 98");

            if (osvi.szCSDVersion[1] == 'A')
                str += TEXT(" SE");
        } else {
            str = TEXT("Microsoft Windows 95");

            if (osvi.szCSDVersion[1] == 'B' || osvi.szCSDVersion[1] == 'C')
                str += TEXT(" OSR2");
        }
        break;

    case VER_PLATFORM_WIN32s:
        str = TEXT("Microsoft Win32s");

    default:
        return TEXT("???");
    }

    String vstr;

    if (osvi.dwMajorVersion <= 4)
        vstr.printf(TEXT(" Version %d.%d %s Build %d"),
                    osvi.dwMajorVersion, osvi.dwMinorVersion,
                    osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
    else
        vstr.printf(TEXT(" %s (Build %d)"), osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);

    return str + vstr;
}


typedef void (WINAPI *RUNDLLPROC)(HWND hwnd, HINSTANCE hinst, LPCTSTR cmdline, DWORD nCmdShow);

BOOL RunDLL(HWND hwnd, LPCTSTR dllname, LPCSTR procname, LPCTSTR cmdline, UINT nCmdShow)
{
    HMODULE hmod = LoadLibrary(dllname);
    if (!hmod)
        return FALSE;

    /*TODO
        <Windows NT/2000>
        It is possible to create a Unicode version of the function.
        Rundll32 first tries to find a function named EntryPointW.
        If it cannot find this function, it tries EntryPointA, then EntryPoint.
        To create a DLL that supports ANSI on Windows 95/98/Me and Unicode otherwise,
        export two functions: EntryPointW and EntryPoint.
    */
    RUNDLLPROC proc = (RUNDLLPROC)GetProcAddress(hmod, procname);
    if (!proc) {
        FreeLibrary(hmod);
        return FALSE;
    }

    proc(hwnd, hmod, cmdline, nCmdShow);

    FreeLibrary(hmod);

    return TRUE;
}


#ifdef UNICODE
#define CONTROL_RUNDLL "Control_RunDLLW"
#else
#define CONTROL_RUNDLL "Control_RunDLLA"
#endif

BOOL launch_cpanel(HWND hwnd, LPCTSTR applet)
{
    TCHAR parameters[MAX_PATH];

    _tcscpy(parameters, TEXT("shell32.dll,Control_RunDLL "));
    _tcscat(parameters, applet);

    return ((int)ShellExecute(hwnd, TEXT("open"), TEXT("rundll32.exe"), parameters, NULL, SW_SHOWDEFAULT) > 32);
}


BOOL RecursiveCreateDirectory(LPCTSTR path_in)
{
    TCHAR path[MAX_PATH], hole_path[MAX_PATH];

    _tcscpy(hole_path, path_in);

    int drv_len = 0;
    LPCTSTR d;

    for (d = hole_path; *d && *d != '/' && *d != '\\'; ++d) {
        ++drv_len;

        if (*d == ':')
            break;
    }

    LPTSTR dir = hole_path + drv_len;

    int l;
    LPTSTR p = hole_path + (l = (int)_tcslen(hole_path));

    while (--p >= hole_path && (*p == '/' || *p == '\\'))
        *p = '\0';

    WIN32_FIND_DATA w32fd;

    HANDLE hFind = FindFirstFile(hole_path, &w32fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        _tcsncpy(path, hole_path, drv_len);
        int i = drv_len;

        for (p = dir; *p == '/' || *p == '\\'; p++)
            path[i++] = *p++;

        for (; i < l; i++) {
            memcpy(path, hole_path, i * sizeof(TCHAR));

            for (; hole_path[i] && hole_path[i] != '/' && hole_path[i] != '\\'; i++)
                path[i] = hole_path[i];

            path[i] = '\0';

            hFind = FindFirstFile(path, &w32fd);

            if (hFind != INVALID_HANDLE_VALUE)
                FindClose(hFind);
            else {
                LOG(FmtString(TEXT("CreateDirectory(\"%s\")"), path));

                if (!CreateDirectory(path, 0))
                    return FALSE;
            }
        }
    } else
        FindClose(hFind);

    return TRUE;
}


DWORD RegGetDWORDValue(HKEY root, LPCTSTR path, LPCTSTR valueName, DWORD def)
{
    HKEY hkey;
    DWORD ret;

    if (!RegOpenKey(root, path, &hkey)) {
        DWORD len = sizeof(ret);

        if (RegQueryValueEx(hkey, valueName, 0, NULL, (LPBYTE)&ret, &len))
            ret = def;

        RegCloseKey(hkey);

        return ret;
    } else
        return def;
}


BOOL RegSetDWORDValue(HKEY root, LPCTSTR path, LPCTSTR valueName, DWORD value)
{
    HKEY hkey;
    BOOL ret = FALSE;

    if (!RegOpenKey(root, path, &hkey)) {
        ret = RegSetValueEx(hkey, valueName, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));

        RegCloseKey(hkey);
    }

    return ret;
}


BOOL exists_path(LPCTSTR path)
{
    WIN32_FIND_DATA fd;

    HANDLE hfind = FindFirstFile(path, &fd);

    if (hfind != INVALID_HANDLE_VALUE) {
        FindClose(hfind);

        return TRUE;
    } else
        return FALSE;
}


bool SplitFileSysURL(LPCTSTR url, String &dir_out, String &fname_out)
{
    if (!_tcsnicmp(url, TEXT("file://"), 7)) {
        url += 7;

        // remove third slash in front of drive characters
        if (*url == '/')
            ++url;
    }

    if (exists_path(url)) {
        TCHAR path[_MAX_PATH];

        // convert slashes to back slashes
        GetFullPathName(url, COUNTOF(path), path, NULL);

        if (GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY)
            fname_out.erase();
        else {
            TCHAR drv[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

            _tsplitpath_s(path, drv, COUNTOF(drv), dir, COUNTOF(dir), fname, COUNTOF(fname), ext, COUNTOF(ext));
            _stprintf(path, TEXT("%s%s"), drv, dir);

            fname_out.printf(TEXT("%s%s"), fname, ext);
        }

        dir_out = path;

        return true;
    } else
        return false;
}


static char *getmsgstr(UINT msgid)
{
    char *msg = NULL;
    static char buff[200];
    switch (msgid) {
    case WM_COMMAND:msg = ("WM_COMMAND"); break;
    case WM_NOTIFY:msg = ("WM_NOTIFY"); break;
    case WM_CONTEXTMENU: msg = ("WM_CONTEXTMENU"); break;
    case WM_INITDIALOG: msg = ("WM_INITDIALOG"); break;
    case WM_ACTIVATEAPP: msg = ("WM_ACTIVATEAPP"); break;
    case WM_STYLECHANGING: msg = ("WM_STYLECHANGING"); break;
    case WM_STYLECHANGED: msg = ("WM_STYLECHANGED"); break;
    case WM_NCPAINT: msg = ("WM_NCPAINT"); break;
    case WM_NCACTIVATE: msg = ("WM_NCACTIVATE"); break;
    case WM_CHANGEUISTATE: msg = ("WM_CHANGEUISTATE"); break;
    case WM_ACTIVATE: msg = ("WM_ACTIVATE"); break;
    case WM_SHOWWINDOW: msg = ("WM_SHOWWINDOW"); break;
    case WM_CTLCOLORDLG: msg = ("WM_CTLCOLORDLG"); break;
    case WM_PRINTCLIENT: msg = ("WM_PRINTCLIENT"); break;
    case WM_SETCURSOR: msg = ("WM_SETCURSOR"); break;
    case WM_LBUTTONUP: msg = ("WM_LBUTTONUP"); break;
    case WM_LBUTTONDBLCLK: msg = ("WM_LBUTTONDBLCLK"); break;
    case WM_RBUTTONDOWN: msg = ("WM_RBUTTONDOWN"); break;
    case WM_RBUTTONUP: msg = ("WM_RBUTTONUP"); break;
    case WM_RBUTTONDBLCLK: msg = ("WM_RBUTTONDBLCLK"); break;
    case WM_MBUTTONDOWN: msg = ("WM_MBUTTONDOWN"); break;
    case WM_MBUTTONUP: msg = ("WM_MBUTTONUP"); break;
    case WM_NCHITTEST: msg = ("WM_NCHITTEST"); break;
    default:
        sprintf_s(buff, 200, "0x%x", msgid);
        return buff;
    }
    return msg;
}

#ifndef LOGA
extern void _logA_(LPCSTR txt);

#define LOGA(txt) _logA_(txt)
#endif
void PrintMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buff[200];
    if (message != WM_SETCURSOR && message != WM_NCMOUSEMOVE && message != WM_MOUSEMOVE) {
        sprintf_s(buff, 200, "hWnd:0x%x %s 0x%x 0x%x\r\n", hWnd, getmsgstr(message), wParam, lParam);
        LOGA(buff);
    }
}

