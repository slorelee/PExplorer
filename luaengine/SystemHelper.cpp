#include "precomp.h"

#include<winternl.h>
#include<Shlwapi.h>
#include "LuaEngine.h"

extern BOOL IsUEFIMode();

extern HRESULT CreateShortcut(PTSTR lnk, PTSTR target, PTSTR param, PTSTR icon, int iIcon, int iShowCmd);
extern void WaitForShellTerminated();
extern void CloseShellProcess();

extern void InitUserSession(ULONG offset);

void WaitForSession();
void SwitchSession(int id);


DWORD WINAPI PlaySndProc(LPVOID param)
{
    char *pFileName = (char *)param;
    PlaySoundA(pFileName, NULL, SND_FILENAME);
    free(pFileName);
    return 0;
}

int osinfo_mem(lua_State* L) {
    int ret = 0;
    Token v = { TOK_STRING };
    ULONGLONG memInstalled = 0;
    MEMORYSTATUS memStatus;
    memset(&memStatus, 0x00, sizeof(MEMORYSTATUS));
    memStatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&memStatus);
    SIZE_T zt = memStatus.dwTotalPhys;
    GetPhysicallyInstalledSystemMemory(&memInstalled);
    int err = GetLastError();
    char buff[MAXBYTE] = { 0 };
    char *fmt = "%ld";
#ifdef _WIN64
    fmt = "%lld";
#endif
    sprintf(buff, fmt, memInstalled);
    lua_pushstring(L, buff); ret++;
    sprintf(buff, fmt, memStatus.dwTotalPhys);
    lua_pushstring(L, buff); ret++;
    sprintf(buff, fmt, memStatus.dwAvailPhys);
    lua_pushstring(L, buff); ret++;
    if (err) printf("GetPhysicallyInstalledSystemMemory error(ec=%d).\n", err);
    return ret;
}

// Get privilege
HRESULT Priv(PCTSTR ptzName)
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return FALSE;
    }
    TOKEN_PRIVILEGES tPriv;
    LookupPrivilegeValue(NULL, ptzName, &tPriv.Privileges[0].Luid);
    tPriv.PrivilegeCount = 1;
    tPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    return AdjustTokenPrivileges(hToken, FALSE, &tPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
}

#define REG_MemMgr TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management")
#define Session_Manager TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager")
#define REG_PageFile TEXT("PagingFiles")

#define MB_TO_BYTES(mb) (1024ULL * 1024ULL * mb)

int CreatePagingFile(string_t *path, int min_mb, int max_mb) {
    int len = 0;
    ULARGE_INTEGER ulMin, ulMax;
    HRESULT hResult = 0;
    UNICODE_STRING sPath;
    TCHAR wzDrive[MAX_PATH] = { 0 };
    TCHAR wzPath[MAX_PATH] = { 0 };

    lstrcpyn(wzDrive, path->c_str(), 4);
    len = QueryDosDevice(wzDrive, wzPath, MAX_PATH);
    if (len <= 0) {
        return -1;
    }
    lstrcat(wzDrive, TEXT("\\pagefile.sys"));
    path->assign(wzDrive);

    lstrcat(wzPath, TEXT("\\pagefile.sys"));
    len = lstrlen(wzPath);

    ulMin.QuadPart = MB_TO_BYTES(min_mb);
    ulMax.QuadPart = MB_TO_BYTES(max_mb);

    sPath.Length = len * sizeof(WCHAR);
    sPath.MaximumLength = len + sizeof(WCHAR);

    sPath.Buffer = wzPath;

    // Get function address
    typedef NTSTATUS(NTAPI* PNtCreatePagingFile)(PUNICODE_STRING sPath, PULARGE_INTEGER puInitSize, PULARGE_INTEGER puMaxSize, ULONG uPriority);
    PNtCreatePagingFile NtCreatePagingFile = (PNtCreatePagingFile)GetProcAddress(GetModuleHandle(TEXT("NTDLL")), "NtCreatePagingFile");
    if (!NtCreatePagingFile) {
        return -1;
    }
    // Create page file
    Priv(SE_CREATE_PAGEFILE_NAME);
    hResult = NtCreatePagingFile(&sPath, &ulMin, &ulMax, 0);
    if (hResult == S_OK) {
        // Log to Windows Registry
        TCHAR tzStr[MAX_PATH*5];
        DWORD i = sizeof(tzStr);
        if (SHGetValue(HKEY_LOCAL_MACHINE, REG_MemMgr, REG_PageFile, NULL, tzStr, &i) != S_OK) {
            i = 0;
        } else {
            i = (i / sizeof(TCHAR)) - 1;
        }
        i += _stprintf(tzStr + i, TEXT("%s %d %d"), path->c_str(), min_mb, max_mb);
        tzStr[++i] = TEXT('\0');
        SHSetValue(HKEY_LOCAL_MACHINE, REG_MemMgr, REG_PageFile, REG_MULTI_SZ, tzStr, i * sizeof(TCHAR));
    }
    return 0;
}

int EnableEUDC(BOOL fEnableEUDC) {
    int rc = 1;
    typedef void (WINAPI* EnableEUDCProc)(BOOL);
    EnableEUDCProc EnableEUDC;
    HMODULE hm = LoadLibrary(TEXT("Gdi32.dll"));
    if (NULL != hm) {
        EnableEUDC = (EnableEUDCProc)GetProcAddress(hm, "EnableEUDC");
        if (NULL != EnableEUDC) {
            EnableEUDC(fEnableEUDC);
            rc = 0;
        }
        FreeLibrary(hm);
    }
    return rc;
}

int AppxSysprepInit() {
    int rc = 1;
    typedef void (WINAPI* AppxSysprepFunc)(void);
    AppxSysprepFunc AppxSysprepSpecializeOnline;
    HMODULE hm = LoadLibrary(TEXT("AppxSysPrep.dll"));
    if (NULL != hm) {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);
        AppxSysprepSpecializeOnline = (AppxSysprepFunc)GetProcAddress(hm, "AppxSysprepSpecializeOnline");
        if (NULL != AppxSysprepSpecializeOnline) {
            AppxSysprepSpecializeOnline();
            rc = 0;
        }
        FreeLibrary(hm);
        CoUninitialize();
    }
    return rc;
}

EXTERN_C {
    int lua_os_info(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        string_t info = s2w(lua_tostring(L, base + 2));
        TCHAR infoname[MAX_PATH] = { 0 };
        _tcscpy(infoname, info.c_str());
        info = _tcslwr(infoname);

        if (info == TEXT("mem")) {
            ret += osinfo_mem(L);
        } else if (info == TEXT("copyright")) {
            v.str = TEXT("#{@Branding\\Basebrd\\basebrd.dll,14}");
            //varstr_expand(v.str);
            resstr_expand(v.str);
            PUSH_STR(v);
        } else if (info == TEXT("localename")) {
            v.str = g_Globals._locale;
            PUSH_STR(v);
        } else if (info == TEXT("winver")) {
            if (top == base + 1) {
                v.str = g_Globals._winver;
                PUSH_STR(v);
            } else {
                v.str = FmtString(TEXT("%d.%d.%d.%d"),
                    g_Globals._winvers[0], g_Globals._winvers[1],
                    g_Globals._winvers[2], g_Globals._winvers[3]);
                PUSH_STR(v);
                PUSH_INTVAL(g_Globals._winvers[0]);
                PUSH_INTVAL(g_Globals._winvers[1]);
                PUSH_INTVAL(g_Globals._winvers[2]);
                PUSH_INTVAL(g_Globals._winvers[3]);
            }
        } else if (info == TEXT("langid")) {
            v.str = g_Globals._langID;
            PUSH_STR(v);
        } else if (info == TEXT("locale")) {
            v.str = g_Globals._locale;
            PUSH_STR(v);
        } else if (info == TEXT("firmwaretype")) {
            v.str = TEXT("BIOS");
            if (IsUEFIMode()) v.str = TEXT("UEFI");
            PUSH_STR(v);
        } else if (info == TEXT("isuefimode")) {
            v.iVal = IsUEFIMode();
            PUSH_BOOL(v);
        } else if (info == TEXT("tickcount")) {
            v.iVal = GetTickCount();
            PUSH_INT(v);
        }
        return ret;
    }

    int lua_file_exists(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        v.str = s2w(lua_tostring(L, base + 2));
        varstr_expand(v.str);
        if (PathFileExists(v.str.c_str())) {
            v.iVal = 1;
            if (FILE_ATTRIBUTE_DIRECTORY == PathIsDirectory(v.str.c_str())) {
                v.iVal = 0;
            }
        }
        PUSH_INT(v);
        return ret;
    }

    int lua_path_trans(lua_State* L, int top, int base, int toshortpath) {
        int ret = 0;
        Token v = { TOK_UNSET };
        TCHAR buff[MAX_PATH] = { 0 };
        v.str = s2w(lua_tostring(L, base + 2));
        varstr_expand(v.str);
        if (toshortpath == 0) {
            ret = GetLongPathName(v.str.c_str(), buff, MAX_PATH);
        } else {
            ret = GetShortPathName(v.str.c_str(), buff, MAX_PATH);
        }
        if (ret > 0) {
            v.str = buff;
        } else {
            v.str = TEXT("");
        }
        ret = 0;
        PUSH_STR(v);
        return ret;
    }

    /*
    int lua_xx_yyy(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        return ret;
    }
    */

    int lua_os_run(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        string_t cmd = s2w(lua_tostring(L, base + 2));
        string_t param = TEXT("");
        LPCTSTR param_ptr = NULL;
        int showflags = SW_SHOWNORMAL;
        if (lua_isstring(L, base + 3)) {
            param = s2w(lua_tostring(L, base + 3));
            param_ptr = param.c_str();
        }
        if (lua_isnumber(L, base + 4)) showflags = (int)lua_tointeger(L, base + 4);
        launch_file(g_Globals._hwndDesktop, cmd.c_str(), showflags, param_ptr);
        v.iVal = 0;
        PUSH_INT(v);
        return ret;
    }

    int lua_os_exec(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        string_t cmd = s2w(lua_tostring(L, base + 2));
        string_t verb = _T("");
        int showflags = SW_SHOWNORMAL;
        bool wait = false;
        if (lua_isboolean(L, base + 3)) {
            wait = lua_toboolean(L, base + 3);
        }
        if (lua_isnumber(L, base + 4)) {
            showflags = (int)lua_tointeger(L, base + 4);
        }
        if ((lua_type(L, (base + 5)) == LUA_TSTRING)) {
            verb = s2w(lua_tostring(L, base + 5));
        }
        DWORD dwExitCode = Exec((PTSTR)cmd.c_str(), wait, showflags, (PTSTR)verb.c_str());
        v.iVal = (int)dwExitCode;
        PUSH_INT(v);
        return ret;
    }

    int lua_os_link(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        string_t str_lnk, str_target;
        string_t str_param, str_icon;

        if (!lua_isstring(L, base + 2)) return 0;
        if (!lua_isstring(L, base + 3)) return 0;

        str_lnk = s2w(lua_tostring(L, base + 2));
        str_target = s2w(lua_tostring(L, base + 3));

        resstr_expand(str_lnk);
        resstr_expand(str_target);
        PTSTR lnk = (PTSTR)str_lnk.c_str();
        PTSTR target = (PTSTR)str_target.c_str();
        PTSTR param = NULL, icon = NULL;
        int iIcon = 0, iShowCmd = SW_SHOWNORMAL;
        if (lua_isstring(L, base + 4)) {
            str_param = s2w(lua_tostring(L, base + 4));
            resstr_expand(str_param);
            param = (PTSTR)str_param.c_str();
        }
        if (lua_isstring(L, base + 5)) {
            str_icon = s2w(lua_tostring(L, base + 5));
            icon = (PTSTR)str_icon.c_str();
        }
        if (lua_isinteger(L, base + 6)) {
            iIcon = lua_tointeger(L, base + 6);
        }
        if (lua_isinteger(L, base + 7)) {
            iShowCmd = lua_tointeger(L, base + 7);
        }
        CreateShortcut(lnk, target, param, icon, iIcon, iShowCmd);
        v.iVal = 0;
        PUSH_INT(v);
        return ret;
    }

    int lua_os_rundll(lua_State* L, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        // App:RunDll('Kernel32.dll','SetComputerName','WINDOWS-PC')
        // App:RunDll('Netapi32.dll','NetJoinDomain',nil,'WORKGROUP',nil,nil,nil,1)
        // Call DLL function
        typedef HRESULT(WINAPI *PROC1)(PVOID pv0);
        typedef HRESULT(WINAPI *PROC2)(PVOID pv0, PVOID pv1);
        typedef HRESULT(WINAPI *PROC3)(PVOID pv0, PVOID pv1, PVOID pv2);
        typedef HRESULT(WINAPI *PROC4)(PVOID pv0, PVOID pv1, PVOID pv2, PVOID pv3);
        typedef HRESULT(WINAPI *PROC5)(PVOID pv0, PVOID pv1, PVOID pv2, PVOID pv3, PVOID pv4);
        typedef HRESULT(WINAPI *PROC6)(PVOID pv0, PVOID pv1, PVOID pv2, PVOID pv3, PVOID pv4, PVOID pv5);
        typedef HRESULT(WINAPI *PROC7)(PVOID pv0, PVOID pv1, PVOID pv2, PVOID pv3, PVOID pv4, PVOID pv5, PVOID pv6);
        typedef HRESULT(WINAPI *PROC8)(PVOID pv0, PVOID pv1, PVOID pv2, PVOID pv3, PVOID pv4, PVOID pv5, PVOID pv6, PVOID pv7);
        typedef HRESULT(WINAPI *PROC9)(PVOID pv0, PVOID pv1, PVOID pv2, PVOID pv3, PVOID pv4, PVOID pv5, PVOID pv6, PVOID pv7, PVOID pv8);
        HRESULT hResult = E_NOINTERFACE;
        int uArg = top - 3;
        string_t strArg[9];
        PTSTR ptzArg[9] = { NULL };
        for (int i = 1; i <= uArg; i++) {
            int n = base + 3 + i;
            if (lua_isnumber(L, n)) {
                ptzArg[i - 1] = (PTSTR)(INT_PTR)lua_tointeger(L, n);
            } else if (!lua_isnil(L, n)) {
                strArg[i - 1] = s2w(lua_tostring(L, n));
                ptzArg[i - 1] = (PTSTR)strArg[i - 1].c_str();
            }
        }
        HMODULE hLib = NULL;
        if (uArg >= 0) hLib = LoadLibraryA(lua_tostring(L, base + 2));
        if (hLib) {
            PROC f = GetProcAddress(hLib, lua_tostring(L, base + 3));
            if (f) {
                switch (uArg) {
                case 0: hResult = (HRESULT)f(); break;
                case 1: hResult = ((PROC1)f)(ptzArg[0]); break;
                case 2: hResult = ((PROC2)f)(ptzArg[0], ptzArg[1]); break;
                case 3: hResult = ((PROC3)f)(ptzArg[0], ptzArg[1], ptzArg[2]); break;
                case 4: hResult = ((PROC4)f)(ptzArg[0], ptzArg[1], ptzArg[2], ptzArg[3]); break;
                case 5: hResult = ((PROC5)f)(ptzArg[0], ptzArg[1], ptzArg[2], ptzArg[3], ptzArg[4]); break;
                case 6: hResult = ((PROC6)f)(ptzArg[0], ptzArg[1], ptzArg[2], ptzArg[3], ptzArg[4], ptzArg[5]); break;
                case 7: hResult = ((PROC7)f)(ptzArg[0], ptzArg[1], ptzArg[2], ptzArg[3], ptzArg[4], ptzArg[5], ptzArg[6]); break;
                case 8: hResult = ((PROC8)f)(ptzArg[0], ptzArg[1], ptzArg[2], ptzArg[3], ptzArg[4], ptzArg[5], ptzArg[6], ptzArg[7]); break;
                case 9: hResult = ((PROC9)f)(ptzArg[0], ptzArg[1], ptzArg[2], ptzArg[3], ptzArg[4], ptzArg[5], ptzArg[6], ptzArg[7], ptzArg[8]); break;
                }
            }
            FreeLibrary(hLib);
        }
        v.iVal = hResult;
        DWORD dw = GetLastError();
        PUSH_INT(v);
        return ret;
    }

    int lua_system_call(lua_State* L, const char *funcname, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "putenv") {
            string_t var = s2w(lua_tostring(L, base + 2));
            string_t str = TEXT("");
            if (lua_isstring(L, base + 3)) {
                str = s2w(lua_tostring(L, base + 3));
                var.append(TEXT("="));
                var.append(str);
            }
            _putenv(w2s(var).c_str());
        } else if (func == "cd") {
            string_t str_path = s2w(lua_tostring(L, base + 2));
            SetCurrentDirectory(str_path.c_str());
        } else if (func == "run") {
            return lua_os_run(L, top, base);
        } else if (func == "exec") {
            return lua_os_exec(L, top, base);
        } else if (func == "link") {
            return lua_os_link(L, top, base);
        } else if (func == "fakeexplorer") {
            FakeExplorer();
        } else if (func == "os::info") {
            return lua_os_info(L, top, base);
        } else if (func == "file::exists") {
            return lua_file_exists(L, top, base);
        } else if (func == "file::doverb") {
            string_t file = s2w(lua_tostring(L, base + 2));
            string_t verb = s2w(lua_tostring(L, base + 3));
            varstr_expand(file);
            DoFileVerb(file.c_str(), verb.c_str());
        } else if (func == "folder::exists") {
            v.str = s2w(lua_tostring(L, base + 2));
            varstr_expand(v.str);
            v.iVal = PathFileExists(v.str.c_str());
            if (v.iVal == 1) v.iVal = PathIsDirectory(v.str.c_str());
            if (v.iVal == FILE_ATTRIBUTE_DIRECTORY) v.iVal = 1;
            PUSH_INT(v);
        } else if (func == "path::getfullpath") {
            return lua_path_trans(L, top, base, 0);
        } else if (func == "path::getshortpath") {
            return lua_path_trans(L, top, base, 1);
        } else if (func == "system::changecolorthemenotify") {
            // g_Globals._SHSettingsChanged(0, TEXT("ImmersiveColorSet"));
            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("ImmersiveColorSet")));
        } else if (func == "waitforshellterminated") {
            WaitForShellTerminated();
        } else if (func == "closeshell") {
            CloseShellProcess();
        } else if (func == "userlogoninit") {
            string_t enabled = JCFG2_DEF("JS_SESSION", "userlogoninit", TEXT("enabled")).ToString();
            ULONG offset = (ULONG)JCFG2_DEF("JS_SESSION", "userlogoninit_offset", 0.0).ToDouble();
            if (enabled == TEXT("enabled")) InitUserSession(offset);
        } else if (func == "waitforsession") {
            WaitForSession();
        } else if (func == "switchsession") {
            v.str = s2w(lua_tostring(L, base + 2));
            v.iVal = 2;
            if (v.str == TEXT("SYSTEM")) v.iVal = 1;
            SwitchSession(v.iVal);
        } else if (func == "system::createpagefile") {
            int iMin, iMax;
            if (!lua_isinteger(L, base + 3)) {
                return 0;
            }
            if (!lua_isinteger(L, base + 4)) {
                return 0;
            }
            v.str = s2w(lua_tostring(L, base + 2));
            iMin = (int)lua_tointeger(L, base + 3);
            iMax = (int)lua_tointeger(L, base + 4);
            if (CreatePagingFile(&v.str, iMin, iMax)) return 0;
        } else if (func == "system::setcursors") {
            if ((lua_type(L, (base + 2)) == LUA_TSTRING)) {
                v.str = s2w(lua_tostring(L, base + 2));
            }
            SystemParametersInfo(SPI_SETCURSORS, 0, NULL, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
        } else if (func == "system::enableeudc") {
            int f = (int)lua_tointeger(L, base + 2);
            v.iVal = EnableEUDC(f);
            PUSH_INT(v);
        } else if (func == "system::appxsysprepinit") {
            v.iVal = AppxSysprepInit();
            PUSH_INT(v);
        } else if (func == "beep") {
            if (lua_isinteger(L, base + 2)) {
                int type = (int)lua_tointeger(L, base + 2);
                MessageBeep(type);
            }
        } else if (func == "play") {
            int bewait = 1;
            if (lua_isinteger(L, base + 3)) {
                bewait = (int)lua_tointeger(L, base + 3);
            }
            if (bewait == 0) {
                char *file = (char *)malloc(MAX_PATH);
                strcpy_s(file, MAX_PATH, lua_tostring(L, base + 2));
                CreateThread(NULL, 0, PlaySndProc, (void *)file, 0, NULL);
            } else {
                PlaySoundA(lua_tostring(L, base + 2), NULL, SND_FILENAME);
            }
        } else if (func == "rundll") {
            return lua_os_rundll(L, top, base);
        } else {
            return -1;
        }
        return ret;
    }

}
