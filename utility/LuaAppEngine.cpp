
#include <precomp.h>
#include <string>
#include <Windows.h>
#include "../resource.h"
#include "../systemsettings/DesktopCommand.h"
#include "../systemsettings/MonitorAdapter.h"
#include "../systemsettings/Volume.h"
#include "FolderOptions.h"
#include "Dialog.h"

extern int FakeExplorer();
extern HRESULT CreateShortcut(PTSTR lnk, PTSTR target, PTSTR param, PTSTR icon, INT iIcon, INT iShowCmd);
extern HRESULT DoFileVerb(PCTSTR tzFile, PCTSTR verb);
TCHAR *CompletePath(TCHAR *target, TCHAR *buff);

extern BOOL IsUEFIMode();
extern BOOL hasMSExplorer();
extern BOOL isWinXShellAsShell();
extern void wxsOpen(LPTSTR cmd);

extern void WaitForShellTerminated();
extern void CloseShellProcess();

extern LPVOID LoadCustomResource(UINT rID, LPTSTR rType);
extern BOOL FreeCustomResource(LPVOID res);

extern int GetScreenBrightness();
extern int SetScreenBrightness(int brightness);

#ifdef _DEBUG
#   ifdef _WIN64
#       pragma comment(lib, "lua/lua53_d_x64.lib")
#       pragma comment(lib, "lua/lua-cjson_d_x64.lib")
#   else
#       pragma comment(lib, "lua/lua53_d.lib")
#       pragma comment(lib, "lua/lua-cjson_d.lib")
#   endif
#else
#   ifdef _WIN64
#       pragma comment(lib, "lua/lua53_x64.lib")
#       pragma comment(lib, "lua/lua-cjson_x64.lib")
#   else
#       pragma comment(lib, "lua/lua53.lib")
#       pragma comment(lib, "lua/lua-cjson.lib")
#   endif
#endif


enum TokenValue {
    TOK_UNSET = 0,
    TOK_STRING,
    TOK_INTEGER,
    TOK_BOOL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_STRARR,
    TOK_LIST,
    TOK_ELEM,
    TOK_UNDEFINED,
};

typedef struct _Token {
    TokenValue type;
    string_t str;
    union {
        LPVOID pObj;
        int iVal;
        bool bVal;
    };
    string_t attr; //element
} Token;

extern Token GetResolutionList(int n = -1);
extern Token SetResolutionByStr(string_t wh);

struct LuaAppWindow : public Window {
    typedef Window super;
    LuaAppWindow(HWND hwnd);
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
    HWND GetHWND();
    static HWND Create();
    LuaAppEngine *_lua;
protected:
    const UINT WM_TASKBARCREATED;
};

LuaAppWindow::LuaAppWindow(HWND hwnd)
    : super(hwnd), _lua(NULL),
     WM_TASKBARCREATED(RegisterWindowMessage(WINMSG_TASKBARCREATED))
{
}

HWND LuaAppWindow::Create()
{
    static WindowClass wcLuaAppWindow(TEXT("WINXSHELL_LUAAPPWINDOW"));
    HWND hwndAM = Window::Create(WINDOW_CREATOR(LuaAppWindow),
                                 WS_EX_NOACTIVATE, wcLuaAppWindow, TEXT(""), WS_POPUP,
                                 0, 0, 0, 0, 0);
    return hwndAM;
}

LRESULT LuaAppWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    if (nmsg == WM_TIMER) {
        if (_lua) {
            _lua->onTimer((int)wparam);
            return S_OK;
        }
    }
    return super::WndProc(nmsg, wparam, lparam);
}

HWND LuaAppWindow::GetHWND()
{
    return super::WindowHandle::_hwnd;
}

LuaAppEngine::LuaAppEngine(string_t& file)
{
    init(file);
}

LuaAppEngine::~LuaAppEngine()
{
    lua_close(L);
}

void AutoHideTaskBar(BOOL bHide)
{
#ifndef   ABM_SETSTATE 
#define   ABM_SETSTATE             0x0000000A 
#endif
    LPARAM lParam = ABS_ALWAYSONTOP;
    if (bHide) lParam = ABS_AUTOHIDE;

    APPBARDATA apBar = { 0 };
    apBar.cbSize = sizeof(APPBARDATA);
    apBar.hWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (apBar.hWnd != NULL) {
        apBar.lParam = lParam;
        SHAppBarMessage(ABM_SETSTATE, &apBar);
    }
}

int TaskBarAutoHideState()
{
    UINT uState = 0;
    APPBARDATA apBar = { 0 };
    apBar.cbSize = sizeof(APPBARDATA);
    uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &apBar);
    if (uState && ABS_AUTOHIDE) return 1;
    return 0;
}

void ShellContextMenuVerb(const TCHAR *file, TCHAR *verb)
{
    // Search target
    TCHAR tzTarget[MAX_PATH] = { 0 };
    TCHAR *target = CompletePath((TCHAR *)file, tzTarget);
    if (!target) return;
    FakeExplorer();
    //CoInitialize(NULL);
    DoFileVerb(target, verb);
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

#ifndef LOGA
extern void _logA_(LPCSTR txt);

#define LOGA(txt) _logA_(txt)
#endif

extern void ProductPolicyLoad(const char *key, const char *value);
extern void ProductPolicyGet(const wchar_t *name);
extern void ProductPolicySet(const wchar_t *name, DWORD val);
extern void ProductPolicySave();

extern int GetCurrentDPIScaling(int x);
extern int GetRecommendedDPIScaling();
extern void SetDpiScaling(int scale);

extern "C" {
    const void *lua_app_addr = NULL;
    int lua_app_loglevel = 0;
#ifdef _DEBUG
    extern int lua_stack(lua_State* L);
#endif

    int lua_app_version(lua_State* L)
    {
        lua_pushstring(L, LUA_RELEASE);
        return 1;
    }

#define PUSH_STR(v) {lua_pushstring(L, w2s(v.str).c_str());ret++;}
#define PUSH_INT(v) {lua_pushinteger(L, v.iVal);ret++;}
#define PUSH_BOOL(v) {lua_pushboolean(L, v.iVal);ret++;}
#define PUSH_INTVAL(val) {lua_pushinteger(L, val);ret++;}

    int lua_os_info(lua_State* L, int top, int base)
    {
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

    int lua_screen_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "screen::get") {
            MonitorAdapter m_monitorAdapter;
            int w, h, f, b;
            m_monitorAdapter.GetCurrentResolution(w, h, f, b);
            v.str = s2w(lua_tostring(L, base + 2));
            if (v.str == TEXT("x") || v.str == TEXT("width")) {
                v.iVal = w;
            } else if (v.str == TEXT("y") || v.str == TEXT("height")) {
                v.iVal = h;
            } else if (v.str == TEXT("rotation")) {
                v.iVal = GetScreenRotation();
            } else if (v.str == TEXT("resolutionlist")) {
                int n = -1;
                if (lua_isinteger(L, base + 3)) {
                    n = (int)lua_tointeger(L, base + 3);
                }
                v = GetResolutionList(n);
                PUSH_STR(v);

                if (n > 0) {
                    PUSH_INT(v);
                }
            } else if (v.str == TEXT("xy")) {
                v.iVal = w;
                PUSH_INT(v);
                v.iVal = h;
            } else if (v.str == TEXT("dpi")) {
                v.iVal = GetCurrentDPIScaling(w);
                PUSH_INT(v);
                v.iVal = GetRecommendedDPIScaling();
            } else if (v.str == TEXT("rdpi") || v.str == TEXT("recommendeddpi")) {
                v.iVal = GetRecommendedDPIScaling();
            } else if (v.str == TEXT("brightness")) {
                v.iVal = GetScreenBrightness();
            }
            PUSH_INT(v);
        } else if (func == "screen::set") {
            v.iVal = 0;
            v.str = s2w(lua_tostring(L, base + 2));
            if (v.str == TEXT("rotation")) {
                int r = (int)lua_tointeger(L, base + 3);
                SetScreenRotation(r);
            } else if (v.str == TEXT("resolution")) {
                int w = (int)lua_tointeger(L, base + 3);
                int h = (int)lua_tointeger(L, base + 4);
                MonitorAdapter m_monitorAdapter;
                VEC_MONITORMODE_INFO vecMointorListInfo;
                v.iVal = m_monitorAdapter.ChangeMonitorResolution(NULL, w, h);
            } else if (v.str == TEXT("maxresolution")) {
                v = GetResolutionList(1);
                if (v.iVal > 0) {
                    v = SetResolutionByStr(v.str);
                }
            } else if (v.str == TEXT("dpi")) {
                v.iVal = (int)lua_tointeger(L, base + 3);
                if (v.iVal >= 100 && v.iVal <= 500) {
                    SetDpiScaling(v.iVal);
                } else if (v.iVal == -1) {
                    // Set Recommended DPI Scaling
                    SetDpiScaling(-1);
                }
            } else if (v.str == TEXT("brightness")) {
                v.iVal = (int)lua_tointeger(L, base + 3);
                v.iVal = SetScreenBrightness(v.iVal);
            }
            PUSH_INT(v);
        }
        return ret;
    }

    int lua_volume_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "volume::mute") {
            v.iVal = SetVolumeMute((int)lua_tointeger(L, base + 2));
            PUSH_INT(v);
        } else if (func == "volume::ismuted") {
            v.iVal = GetVolumeMute();
            PUSH_INT(v);
        } else if (func == "volume::getlevel") {
            v.iVal = GetVolumeLevel();
            PUSH_INT(v);
        } else if (func == "volume::setlevel") {
            v.iVal = SetVolumeLevel((int)lua_tointeger(L, base + 2));
            PUSH_INT(v);
        } else if (func == "volume::getname") {
            GetEndpointVolume();
            v.str = GetVolumeDeviceName(NULL);
            PUSH_STR(v);
        }
        return ret;
    }

    int lua_desktop_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "desktop::updatewallpaper") {
            SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
        } else if (func == "desktop::getwallpaper") {
            TCHAR wpPath[MAX_PATH] = { 0 };
            SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, wpPath, 0);
            v.str = wpPath;
            PUSH_STR(v);
        } else if (func == "desktop::setwallpaper") {
            v.str = s2w(lua_tostring(L, base + 2));
            SHSetValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), TEXT("Wallpaper"), REG_SZ, v.str.c_str(), (DWORD)(v.str.length()) * sizeof(TCHAR));
            SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
        } else if (func == "desktop::refresh") {
            DesktopCommand dtcmd;
            dtcmd.Refresh();
        } else if (func == "desktop::seticonsize") {
            DesktopCommand dtcmd;
            if (lua_isinteger(L, base + 2)) {
                dtcmd.SetIconSize((int)lua_tointeger(L, base + 2));
                return ret;
                }
            v.str = s2w(lua_tostring(L, base + 2));
            if (v.str.find(_T("S")) == 0) {
                dtcmd.SetIconSize(32);
            } else if (v.str.find(_T("M")) == 0) {
                dtcmd.SetIconSize(48);
            } else if (v.str.find(_T("L")) == 0) {
                dtcmd.SetIconSize(96);
            }
        } else if (func == "desktop::autoarrange") {
            DesktopCommand dtcmd;
            v.iVal = (int)lua_tointeger(L, base + 2);
            dtcmd.AutoArrange(v.iVal);
        } else if (func == "desktop::snaptogrid") {
            DesktopCommand dtcmd;
            v.iVal = (int)lua_tointeger(L, base + 2);
            dtcmd.SnapToGrid(v.iVal);
        } else if (func == "desktop::showicons") {
            DesktopCommand dtcmd;
            v.iVal = (int)lua_tointeger(L, base + 2);
            dtcmd.ShowIcons(v.iVal);
        }
        return ret;
    }

    int lua_tasbar_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "taskbar::changenotify") {
            // g_Globals._SHSettingsChanged(0, TEXT("TraySettings"));
            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("TraySettings")));
        } else if (func == "taskbar::autohide") {
            if (lua_isinteger(L, base + 2)) {
                int val = (int)lua_tointeger(L, base + 2);
                AutoHideTaskBar(val == 1);
            }
        } else if (func == "taskbar::autohidestate") {
            v.iVal = TaskBarAutoHideState();
            PUSH_INT(v);
        } else if (func == "taskbar::pin") {
            if (isWinXShellAsShell()) return ret;
            string_t str_file = s2w(lua_tostring(L, base + 2));
            ShellContextMenuVerb(str_file.c_str(), TEXT("taskbarpin"));
        } else if (func == "taskbar::unpin") {
            /*
            function Taskbar:UnPin(name)
            menu_pintotaskbar()
            local pinned_path = [[%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar\]]
            app:call('Taskbar::UnPin', .. name .. '.lnk')
            end
            */
            string_t name = s2w(lua_tostring(L, base + 2));
            string_t str_lnk = TEXT("%APPDATA%\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\") + name + TEXT(".lnk");
            varstr_expand(str_lnk);
            if (isWinXShellAsShell()) {
                DeleteFile(str_lnk.c_str());
                return ret;
            }
            FakeExplorer();
            DoFileVerb(str_lnk.c_str(), TEXT("taskbarunpin"));
            return ret;
        }
        return ret;
    }

    int lua_startmenu_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;

        if (func == "startmenu::pin") {
            string_t str_file = s2w(lua_tostring(L, base + 2));
            ShellContextMenuVerb(str_file.c_str(), TEXT("startpin"));
        } else if (func == "startmenu::unpin") {
            string_t str_file = s2w(lua_tostring(L, base + 2));
            ShellContextMenuVerb(str_file.c_str(), TEXT("startunpin"));
        }
        return ret;
    }

    int lua_folderoptions_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "folderoptions::get") {
            v.str = s2w(lua_tostring(L, base + 2));
            v.iVal = FolderOptions->Get(v.str.c_str());
            PUSH_INT(v);
        } else if (func == "folderoptions::set") {
            if (lua_isnumber(L, base + 2)) {
                FolderOptions->Set((DWORD)lua_tointeger(L, base + 2), lua_tointeger(L, base + 3));
            } else {
                v.str = s2w(lua_tostring(L, base + 2));
                FolderOptions->Set(v.str.c_str(), (DWORD)lua_tointeger(L, base + 3));
            }
        }
        return ret;
    }

    int lua_dialog_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "dialog::openfile") {
            TCHAR titleBuff[MAX_PATH] = { 0 };
            TCHAR dirBuff[MAX_PATH] = { 0 };
            const TCHAR *title = NULL;
            const TCHAR *filters = NULL;
            const TCHAR *dir = NULL;
            if (lua_type(L, base + 2) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 2));
                lstrcpy(titleBuff, v.str.c_str());
                title = titleBuff;
            }
            if (lua_type(L, base + 4) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 4));
                varstr_expand(v.str);
                lstrcpy(dirBuff, v.str.c_str());
                dir = dirBuff;
            }
            if (lua_type(L, base + 3) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 3));
                filters = v.str.c_str();
            }
            v.iVal = Dialog->OpenFile(title, filters, dir);
            v.str = (TCHAR *)Dialog->SelectedFileName;
            PUSH_STR(v);
            PUSH_INT(v);
        } else if (func == "dialog::browsefolder") {
            const TCHAR *title = NULL;
            if (lua_type(L, base + 2) == LUA_TSTRING) {
                v.str = s2w(lua_tostring(L, base + 2));
                title = v.str.c_str();
            }
            if (lua_isinteger(L, base + 3)) {
                v.iVal = (int)lua_tointeger(L, base + 3);
            }
            v.iVal = Dialog->BrowseFolder(title, v.iVal);
            v.str = (TCHAR *)Dialog->SelectedFolderName;
            PUSH_STR(v);
            PUSH_INT(v);
        }
        return ret;
    }

    int lua_productpolicy_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "productpolicy::load") {
            ProductPolicyLoad(lua_tostring(L, base + 2), lua_tostring(L, base + 3));
        } else if (func == "productpolicy::get") {
            v.str = s2w(lua_tostring(L, base + 2));
            ProductPolicyGet(v.str.c_str());
        } else if (func == "productpolicy::set") {
            v.str = s2w(lua_tostring(L, base + 2));
            v.iVal = lua_tointeger(L, base + 3);
            ProductPolicySet(v.str.c_str(), v.iVal);
        } else if (func == "productpolicy::save") {
            ProductPolicySave();
        }
        return ret;
    }

    int lua_class_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        return ret;
    }

    int lua_app_call(lua_State* L)
    {
        int ret = 0;
        Token v = { TOK_UNSET };
        int base = 0;
        int top = lua_gettop(L);
        if (lua_type(L, 1) == LUA_TTABLE) {
            //skip self for app:call
            base++;
            top--;
        }
        luaL_checktype(L, base + 1, LUA_TSTRING);
        std::string func = lua_tostring(L, base + 1);
        char funcname[255] = { 0 };
        strcpy(funcname, func.c_str());
        func = _strlwr(funcname);

        if (func == "putenv") {
            string_t var = s2w(lua_tostring(L, base + 2));
            string_t str = TEXT("");
            if (lua_isstring(L, base + 3)) {
                str = s2w(lua_tostring(L, base + 3));
                var.append(TEXT("="));
                var.append(str);
            }
            _putenv(w2s(var).c_str());
        } else if (func == "envstr") {
            v.str = s2w(lua_tostring(L, base + 2));
            TCHAR buff[MAX_PATH * 5];
            ExpandEnvironmentStrings(v.str.c_str(), buff, MAX_PATH * 5);
            v.str = buff;
            PUSH_STR(v);
        } else if (func == "resstr") {
            v.str = s2w(lua_tostring(L, base + 2));
            resstr_expand(v.str);
            PUSH_STR(v);
        } else if (func == "varstr") {
            v.str = s2w(lua_tostring(L, base + 2));
            varstr_expand(v.str);
            PUSH_STR(v);
        } else if (func == "band") {
            UINT s = (UINT)lua_tointeger(L, base + 2);
            UINT b = (UINT)lua_tointeger(L, base + 3);
            v.iVal = (s & b);
            PUSH_INT(v);
        } else if (func == "cd") {
            string_t str_path = s2w(lua_tostring(L, base + 2));
            SetCurrentDirectory(str_path.c_str());
        } else if (func == "FakeExplorer") {
            FakeExplorer();
        } else if (func == "os::info") {
            return lua_os_info(L, top, base);
        } else if (func == "system::changecolorthemenotify") {
            // g_Globals._SHSettingsChanged(0, TEXT("ImmersiveColorSet"));
            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("ImmersiveColorSet")));
        } else if (func == "file::exists") {
            v.str = s2w(lua_tostring(L, base + 2));
            varstr_expand(v.str);
            if (PathFileExists(v.str.c_str())) {
                v.iVal = 1;
                if (FILE_ATTRIBUTE_DIRECTORY == PathIsDirectory(v.str.c_str())) {
                    v.iVal = 0;
                }
            }
            PUSH_INT(v);
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
        } else if (func.compare(0, 8, "screen::") == 0) {
            return lua_screen_call(L, funcname, top, base);
        } else if (func.compare(0, 8, "volume::") == 0) {
            return lua_volume_call(L, funcname, top, base);
        } else if (func.compare(0, 11, "desktop::") == 0) {
            return lua_desktop_call(L, funcname, top, base);
        } else if (func.compare(0, 9, "taskbar::") == 0) {
            return lua_tasbar_call(L, funcname, top, base);
        } else if (func.compare(0, 11, "startmenu::") == 0) {
            return lua_startmenu_call(L, funcname, top, base);
        } else if (func.compare(0, 15, "folderoptions::") == 0) {
            return lua_folderoptions_call(L, funcname, top, base);
        } else if (func.compare(0, 15, "productpolicy::") == 0) {
            return lua_productpolicy_call(L, funcname, top, base);
        } else if (func.compare(0, 8, "dialog::") == 0) {
            return lua_dialog_call(L, funcname, top, base);
        } else if (func == "setvar") {
            v.str = s2w(lua_tostring(L, base + 2));
            if (v.str == TEXT("ClockText")) {
                string_t clocktext = s2w(lua_tostring(L, base + 3));
                _tcscpy(g_Globals._varClockTextBuffer, clocktext.c_str());
            } else if (v.str == TEXT("Debug")) {
                if (lua_isboolean(L, base + 3)) {
                    g_Globals._isDebug = lua_toboolean(L, base + 3);
                }
            } else if (v.str == TEXT("LogLevel")) {
                if (lua_isinteger(L, base + 3)) {
                    lua_app_loglevel = (int)lua_tointeger(L, base + 3);
                }
            }
        } else if ((func == "settimer") || (func == "killtimer")) {
            LuaAppEngine *lua = g_Globals._lua;
            if (!lua) {
                LOGA("error:Cannot find the lua script file specified");
                return ret;
            }
            LuaAppWindow *frame = (LuaAppWindow *)lua->getFrame();
            if (!frame) return ret;
            if (func == "settimer") {
                SetTimer(frame->GetHWND(), (UINT_PTR)lua_tointeger(L, base + 2), (UINT)lua_tointeger(L, base + 3), NULL);
            } else {
                KillTimer(frame->GetHWND(), lua_tointeger(L, base + 2));
            }
        } else if (func == "sleep") {
            if (lua_isinteger(L, base + 2)) {
                int ms = (int)lua_tointeger(L, base + 2);
                Sleep(ms);
            }
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
        } else if (func == "wxs_open") {
            v.str = s2w(lua_tostring(L, base + 2));
            wxsOpen((LPTSTR)v.str.c_str());
            v.iVal = 0;
            PUSH_INT(v);
        } else if (func == "run") {
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
        } else if (func == "exec") {
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
        } else if (func == "rundll") {
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
        } else if (func == "link") {
            string_t str_lnk = s2w(lua_tostring(L, base + 2));
            string_t str_target = s2w(lua_tostring(L, base + 3));
            string_t str_param, str_icon;
            resstr_expand(str_lnk);
            resstr_expand(str_target);
            PTSTR lnk = (PTSTR)str_lnk.c_str();
            PTSTR target = (PTSTR)str_target.c_str();
            PTSTR param = NULL, icon = NULL;
            INT iIcon = 0, iShowCmd = SW_SHOWNORMAL;
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
        } else if (func == "waitforshellterminated") {
            WaitForShellTerminated();
        } else if (func == "closeshell") {
            CloseShellProcess();
        } else if (func == "exitcode") {
            if (lua_isinteger(L, base + 2)) {
                int code = (int)lua_tointeger(L, base + 2);
                g_Globals._exitcode = code;
            }
        } else if (func == "exit") {
            if (lua_isinteger(L, base + 2)) {
                int code = (int)lua_tointeger(L, base + 2);
                exit(code);
            }
        } else {
            char buff[100];
            sprintf(buff, "error:function %s() is not implemented", func.c_str());
            LOGA(buff);
        }
        return ret;
    }

    int lua_app_info(lua_State* L)
    {
        int ret = 0;
        Token v = { TOK_UNSET };
        int base = 0;
        int top = lua_gettop(L);
        if (lua_type(L, 1) == LUA_TTABLE) {
            //skip self for app:call
            base++;
        }

        luaL_checktype(L, base + 1, LUA_TSTRING);
        std::string name = lua_tostring(L, base + 1);
        char infoname[MAX_PATH] = { 0 };
        strcpy(infoname, name.c_str());
        name = _strlwr(infoname);

        if (name == "cmdline") {
            v.str = g_Globals._cmdline;
            PUSH_STR(v);
        } else if (name == "path") {
            v.str = JVAR("JVAR_MODULEPATH").ToString();
            PUSH_STR(v);
        } else if (name == "name") {
            v.str = JVAR("JVAR_MODULENAME").ToString();
            PUSH_STR(v);
        } else if (name == "fullpath") {
            v.str = JVAR("JVAR_MODULEFILENAME").ToString();
            PUSH_STR(v);
        } else if (name == "iswinpe") {
            v.iVal = g_Globals._isWinPE;
            PUSH_INT(v);
        } else if (name == "isshell") {
            v.iVal = isWinXShellAsShell();
            PUSH_INT(v);
        } else if (name == "isshellmode") {
            v.iVal = g_Globals._isShell;
            PUSH_INT(v);
        } else if (name == "isdebugmodule") {
#ifdef _DEBUG
            v.iVal = TRUE;
#else
            v.iVal = FALSE;
#endif // DEBUG
            PUSH_BOOL(v);
        } else if (name == "hasexplorer") {
            v.iVal = hasMSExplorer();
            PUSH_INT(v);
        }
        return ret;
    }

    int lua_app_jcfg(lua_State* L)
    {
#ifdef _DEBUG
        lua_stack(L);
#endif
        int ret = 0;
        int top = lua_gettop(L);

        if (top <= 0) {
            lua_pushnil(L);
            return 1;
        }

        return 1;
    }

    int lua_print(lua_State* L)
    {
        int i = 1;
        size_t len = 0;
        const char *s = NULL;
        int top = lua_gettop(L);

        /* skip app table self */
        if (lua_app_addr && top >= 1) {
            if (lua_app_addr == lua_topointer(L, 1)) {
                i = 2;
            }
        }
        for (; i <= top; i++) {
            s = luaL_tolstring(L, i, &len);
            if (i == top) {
                LOGA(s);
            } else {
                LOGA2(s, '\t');
            }
        }
        return 0;
    }

    int lua_alert(lua_State* L)
    {
        int i = 1;
        size_t len = 0;
        const char *s = NULL;
        int top = lua_gettop(L);

        /* skip app table self */
        if (lua_app_addr && top >= 1) {
            if (lua_app_addr == lua_topointer(L, 1)) {
                i = 2;
            }
        }
        for (; i <= top; i++) {
            s = luaL_tolstring(L, i, &len);
            MessageBoxA(NULL, s, "", 0);
        }
        return 0;
    }

#include<time.h>
#define LOG_FATAL_LEVEL 1
#define LOG_ERROR_LEVEL 2
#define LOG_WARN_LEVEL 3
#define LOG_INFO_LEVEL 4
#define LOG_DEBUG_LEVEL 5

    static void log_getheader(char *buff, int level)
    {
        time_t curtime = time(0);
        tm tim;
        localtime_s(&tim, &curtime);
        char *levelname = "UNKOWN";

        switch (level) {
        case LOG_DEBUG_LEVEL:
            levelname = "DEBUG";
            break;
        case LOG_INFO_LEVEL:
            levelname = " INFO";
            break;
        case LOG_WARN_LEVEL:
            levelname = " WARN";
            break;
        case LOG_ERROR_LEVEL:
            levelname = "ERROR";
            break;
        case LOG_FATAL_LEVEL:
            levelname = "FATAL";
            break;
        default:
            break;
        }

        sprintf(buff, "%d/%02d/%02d %02d:%02d:%02d %s ", tim.tm_year + 1900, tim.tm_mon + 1,
            tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec, levelname);
    }

    int lua_log(lua_State* L)
    {
        int base = 0;
        size_t len = 0;
        const char *s = NULL;
        int top = lua_gettop(L);
        int level = 0;

        /* skip app table self */
        if (lua_app_addr && top >= 1) {
            if (lua_app_addr == lua_topointer(L, 1)) {
                base++;
            }
        }
        if (lua_isinteger(L, base + 1) == 0) {
            lua_pop(L, -1);
            return 0;
        }
        level = (int)lua_tointeger(L, base + 1);

        if (level <= lua_app_loglevel) {
            char logheader[128] = { 0 };

            if (base == 1) lua_remove(L, 1);
            log_getheader(logheader, level);
            if (top > base + 2) {
                strcat(logheader, "======================================\n");
            }

            lua_pushstring(L, logheader);
            lua_replace(L, 1);

            if (top > base + 2) {
                lua_pushstring(L, "\n================================================================");
            }
            lua_print(L);
        }
        return 0;
    }

    static const luaL_Reg lua_reg_app_funcs[] = {
        { "Version", lua_app_version },
        { "Call", lua_app_call },
        { "Info", lua_app_info },
        { "JCfg", lua_app_jcfg },
        { "Print", lua_print },
        { "Alert", lua_alert },
        { "Log", lua_log },
#ifdef _DEBUG
        { "Stack", lua_stack },
#endif // DEBUG
        { NULL, NULL },
    };

    int luaopen_app_module(lua_State* L)
    {
        luaL_newlib(L, lua_reg_app_funcs);
        return 1;
    }

}

static char *built_code_errorhandle = "function __G__TRACKBACK__(msg)\n"
"    App:Print(\"----------------------------------------\")\n"
"    App:Print(\"LUA ERROR: \" .. tostring(msg) .. \"\\n\")\n"
"    App:Print(debug.traceback())\n"
"    App:Print(\"----------------------------------------\")\n"
"end";

char *app_built_code = "\n\
\n\
\n\
App.FATAL_LEVEL = 1\n\
App.ERROR_LEVEL = 2\n\
App.WARN_LEVEL = 3\n\
App.INFO_LEVEL = 4\n\
App.DEBUG_LEVEL = 5\n\
\n\
\n\
function App:SetVar(...)\n\
  App.Call('SetVar', ...)\n\
end\n\
\n\
App:SetVar('LogLevel', App.FATAL_LEVEL)\n\
\n\
if os.getenv('WINXSHELL_DEBUG') then\n\
  App:SetVar('Debug', true)\n\
  App:SetVar('LogLevel', App.DEBUG_LEVEL)\n\
end\n\
\n\
function App:Fatal(...)\n\
    return App.Log(App.FATAL_LEVEL, ...)\n\
end\n\
function App:Error(...)\n\
    return App.Log(App.ERROR_LEVEL, ...)\n\
end\n\
function App:Warn(...)\n\
    return App.Log(App.WARN_LEVEL, ...)\n\
end\n\
function App:InfoLog(...)\n\
    return App.Log(App.INFO_LEVEL, ...)\n\
end\n\
function App:Debug(...)\n\
    return App.Log(App.DEBUG_LEVEL, ...)\n\
end\n\
  \n\
function App:JCfg(...)\n\
  return App.JCfg(...)\n\
end\n\
\n\
function App:SetTimer(...)\n\
  App.Call('SetTimer', ...)\n\
end\n\
function App:KillTimer(...)\n\
  App.Call('KillTimer', ...)\n\
end\n\
function App:Run(...)\n\
  App.Call('run', ...)\n\
end\n\
function App:RunDll(...)\n\
  App.Call('RunDll', ...)\n\
end\n\
\n\
function App:Sleep(...)\n\
  App.Call('sleep', ...)\n\
end\n\
\n\
Shell = {}\n\
\n";

static int lua_errorfunc = MININT;


EXTERN_C {
    extern int luaopen_winapi_lib(lua_State *L);
}

void LuaAppEngine::init(string_t& file)
{
	HWND hwnd = LuaAppWindow::Create();
    L = luaL_newstate();
    _frame = (void *)LuaAppWindow::get_window(hwnd);
    ((LuaAppWindow *)_frame)->_lua = this;

	LuaApp.L = L;
    luaL_openlibs(L);
    luaopen_winapi_lib(L);
    _name = "App";
    luaL_requiref(L, _name, luaopen_app_module, 1);
    luaL_dostring(L, built_code_errorhandle);
    luaL_dostring(L, app_built_code);

    lua_getglobal(L, "App");
    if (lua_type(L, -1) == LUA_TTABLE) {
        lua_app_addr = lua_topointer(L, -1);
    }

    if (g_Globals._isDebug) {
        lua_app_loglevel = LOG_DEBUG_LEVEL;
    }

    char *rescode = (char *)LoadCustomResource(IDR_LUA_HELPER, TEXT("FILE"));
    if (rescode) {
        if (luaL_dostring(L, rescode) != 0) {
            LOGA(lua_tostring(L, -1));
        }
    }
    FreeCustomResource(rescode);

    rescode = (char *)LoadCustomResource(IDR_APP_HELPERS, TEXT("FILE"));
    if (rescode) {
        if (luaL_dostring(L, rescode) != 0) {
            LOGA(lua_tostring(L, -1));
        }
    }
    FreeCustomResource(rescode);

    if (PathFileExists(file.c_str())) {
        if (luaL_dofile(L, w2s(file).c_str()) != 0) {
            LOGA(lua_tostring(L, -1));
    }
}
}

static int fetch_errorfunc(lua_State *L)
{
    if (lua_errorfunc != MININT) return lua_errorfunc;

    int top = lua_gettop(L);
    lua_getglobal(L, "__G__TRACKBACK__");

    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find function <__G__TRACKBACK__>err");
        lua_settop(L, top);
        return MININT;
    }

    int errfunc = lua_gettop(L);
    lua_errorfunc = errfunc;
    return errfunc;
}

int LuaAppEngine::hasfunc(const char *funcname)
{
    int luatype = 0;
    lua_getglobal(L, funcname);
    luatype = lua_type(L, -1);
    if (luatype != LUA_TFUNCTION) {
        return 0;
    }
    return 1;
}

int LuaAppEngine::hasfunc(const char *tablename, const char *funcname)
{
    int luatype = 0;
    lua_getglobal(L, tablename);
    luatype = lua_type(L, -1);
    if (luatype != LUA_TTABLE) {
        return 0;
    }
    lua_getfield(L, -1, funcname);
    luatype = lua_type(L, -1);
    if (luatype != LUA_TFUNCTION) {
        return 0;
    }
    return 1;
}

int LuaAppEngine::call(const char *funcname, int nres)
{
    char buff[1024] = { 0 };
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return -1;

    lua_getglobal(L, _name);
    if (!lua_istable(L, -1)) {
        return -1;
    }

    lua_getfield(L, -1, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(buff);
        return -1;
    }
    int rel = lua_pcall(L, 0, nres, lua_errorfunc);
    if (rel == -1) return -1;
    if (nres <= 0) return 0;
    rel = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return rel;
}

int LuaAppEngine::call(const char *funcname, int p1, int p2, int nres)
{
    char buff[1024] = { 0 };
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return -1;

    lua_getglobal(L, _name);
    if (!lua_istable(L, -1)) {
        return -1;
    }

    lua_getfield(L, -1, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(buff);
        return -1;
    }
    lua_pushinteger(L, p1);
    lua_pushinteger(L, p2);
    int rel = lua_pcall(L, 2, nres, lua_errorfunc);
    if (rel == -1) return -1;
    if (nres <= 0) return 0;
    rel = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return rel;
}

int LuaAppEngine::call(const char *funcname, string_t& p1, string_t& p2, int nres)
{
    char buff[1024] = { 0 };
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return -1;

    if (funcname) {
        lua_getglobal(L, _name);
        if (!lua_istable(L, -1)) {
            return -1;
        }

        lua_getfield(L, -1, funcname);
        if (lua_type(L, -1) != LUA_TFUNCTION) {
            LOGA(buff);
            return -1;
        }
    }
    lua_pushstring(L, w2s(p1).c_str());
    lua_pushstring(L, w2s(p2).c_str());
    int rel = lua_pcall(L, 2, nres, lua_errorfunc);
    if (rel == -1) return -1;
    if (nres <= 0) return 0;
    rel = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return rel;
}

void LuaAppEngine::RunCode(string_t& code)
{
    if (luaL_dostring(L, w2s(code).c_str()) != 0) {
        LOGA(lua_tostring(L, -1));
}
}

void LuaAppEngine::LoadFile(string_t& file)
{
    if (luaL_dofile(L, w2s(file).c_str()) != 0) {
        LOGA(lua_tostring(L, -1));
}
}


void LuaAppEngine::onLoad()
{
    call("onLoad");
}

void LuaAppEngine::onFirstShellRun()
{
    call("_onFirstShellRun");
    call("onFirstShellRun");
}

void LuaAppEngine::preShell()
{
    call("_PreShell");
    call("PreShell");
}

void LuaAppEngine::onShell()
{
    call("_onShell");
    call("onShell");
}

void LuaAppEngine::onDaemon()
{
    call("_onDaemon");
    call("onDaemon");
}

int LuaAppEngine::onClick(string_t& ctrl)
{
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return -1;

    lua_getglobal(L, "Shell:onClick");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find onclick() function");
        return -1;
    }
    lua_pushstring(L, w2s(ctrl).c_str());
    int rel = lua_pcall(L, 1, 0, lua_errorfunc);
    if (rel == -1) return -1;
    return (int)lua_tointeger(L, 1);
}

#define TID_USER 20000
void LuaAppEngine::onTimer(int id)
{
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return;

    if (id < TID_USER) {
        lua_getglobal(L, "App:_onTimer");
    } else {
        lua_getglobal(L, "App:onTimer");
    }
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find ontimer() function");
        return;
    }
    lua_pushinteger(L, id);
    int rel = lua_pcall(L, 1, 0, lua_errorfunc);
    if (rel == -1) return;
}
