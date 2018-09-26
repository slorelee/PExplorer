
#include <Windows.h>
#include <VersionHelpers.h>
#include "globals.h"
#include "vendor/json.h"
#include "jconfig/jcfg.h"
#include "utility/LuaAppEngine.h"

ExplorerGlobals g_Globals;

ExplorerGlobals::ExplorerGlobals()
{
    _hInstance = 0;
    _cfStrFName = 0;

    _cmdline = _T("");
    _winver = _T("");
#ifndef ROSSHELL
    _hframeClass = 0;
    _hMainWnd = 0;
    _desktop_mode = false;
    _prescan_nodes = false;
#endif

    _log = NULL;
    _SHRestricted = 0;
    _hDefaultFont = NULL;
    _hwndDesktopBar = 0;
    _hwndShellView = 0;
    _hwndDesktop = 0;
    _hwndDaemon = 0;

    _isWinPE = FALSE;
    _lua = NULL;
}

void ExplorerGlobals::init(HINSTANCE hInstance)
{
    _hInstance = hInstance;

    _SHRestricted = (DWORD(STDAPICALLTYPE *)(RESTRICTIONS)) GetProcAddress(GetModuleHandle(TEXT("SHELL32")), "SHRestricted");

    _icon_cache.init();
}

void ExplorerGlobals::get_modulepath()
{
    TCHAR szFile[MAX_PATH + 1] = { 0 };
    String strPath = TEXT("");
    String strFileName = TEXT("");
    JVAR("JVAR_MODULEFILENAME") = TEXT("");
    DWORD dwRet = GetModuleFileName(NULL, szFile, COUNTOF(szFile));
    if (dwRet != 0) {
        strPath = szFile;
        JVAR("JVAR_MODULEFILENAME") = strPath;
        size_t nPos = strPath.rfind(TEXT('\\'));
        if (nPos != -1) {
            strFileName = strPath.substr(nPos + 1);
            strPath = strPath.substr(0, nPos);
        }
    }
    JVAR("JVAR_MODULEPATH") = strPath;
    JVAR("JVAR_MODULENAME") = strFileName;
}

void ExplorerGlobals::get_uifolder()
{
    TCHAR uifolder[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("WINXSHELL_UIFOLDER"), uifolder, MAX_PATH);
    if (dw == 0) {
        g_Globals._uifolder = TEXT("wxsUI");
    } else {
        g_Globals._uifolder = uifolder;
    }
}

void ExplorerGlobals::load_config()
{
    get_uifolder();

    String jcfgfile = TEXT("WinXShell.jcfg");
#ifndef _DEBUG
    TCHAR buff[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("WINXSHELL_JCFGFILE"), buff, MAX_PATH);
    if (dw == 0) {
        jcfgfile = JVAR("JVAR_MODULEPATH").ToString() + TEXT("\\WinXShell.jcfg");
    } else {
        jcfgfile = JVAR("JVAR_MODULEPATH").ToString() + TEXT("\\") + buff;
    }
#endif
    Load_JCfg(jcfgfile);
}

DWORD PASCAL ReadKernelVersion(void)
{
    DWORD dwVersion = 0;
    HMODULE hinstDLL = LoadLibraryExW(L"kernel32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hinstDLL != NULL) {
        HRSRC hResInfo = FindResource(hinstDLL, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
        if (hResInfo != NULL) {
            HGLOBAL hResData = LoadResource(hinstDLL, hResInfo);
            if (hResData != NULL) {
                static const WCHAR wszVerInfo[] = L"VS_VERSION_INFO";
                struct VS_VERSIONINFO {
                    WORD wLength;
                    WORD wValueLength;
                    WORD wType;
                    WCHAR szKey[ARRAYSIZE(wszVerInfo)];
                    VS_FIXEDFILEINFO Value;
                    WORD Children[];
                } *lpVI = (struct VS_VERSIONINFO *)LockResource(hResData);
                if ((lpVI != NULL) && (lstrcmpiW(lpVI->szKey, wszVerInfo) == 0) && (lpVI->wValueLength > 0)) {
                    dwVersion = lpVI->Value.dwFileVersionMS;
                }
            }
        }
        FreeLibrary(hinstDLL);
    }
    return dwVersion;
}

void ExplorerGlobals::get_systeminfo()
{
    DWORD dwVer = ReadKernelVersion();
    g_Globals._winver = FmtString(TEXT("%d.%d"), HIWORD(dwVer), LOWORD(dwVer));
    g_Globals._isNT5 = !IsWindowsVistaOrGreater();

    Value v = JCFG2("JS_SYSTEMINFO", "langid");
    String langID = v.ToString();
    g_Globals._langID = langID;
    if (langID == TEXT("0")) {
        g_Globals._langID.printf(TEXT("%d"), GetSystemDefaultLangID());
    }
}

void ExplorerGlobals::read_persistent()
{
    // read configuration file
}

void ExplorerGlobals::write_persistent()
{
    // write configuration file
    //RecursiveCreateDirectory(_cfg_dir);
}
