
#include <precomp.h>
#include <string>
#include <Windows.h>
#include "LuaAppEngine.h"
#include "../systemsettings/MonitorAdapter.h"
#include "../systemsettings/Volume.h"

extern int FakeExplorer();
extern DWORD Exec(PTSTR ptzCmd, BOOL bWait, INT iShowCmd);
extern HRESULT CreateShortcut(PTSTR lnk, PTSTR target, PTSTR param, PTSTR icon, INT iIcon, INT iShowCmd);
extern HRESULT DoFileVerb(PCTSTR tzFile, PCTSTR verb);
TCHAR *CompletePath(TCHAR *target, TCHAR *buff);

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

#ifndef LOGA
extern void _logA_(LPCSTR txt);

#define LOGA(txt) _logA_(txt)
#endif

extern "C" {
    extern int lua_print(lua_State* L);
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

    int lua_app_call(lua_State* L)
    {
        int ret = 0;
        Token v = { TOK_UNSET };
        int base = 0;

        if (lua_type(L, 1) == LUA_TTABLE) {
            //skip self for app:call
            base++;
        }
        luaL_checktype(L, base + 1, LUA_TSTRING);
        std::string func = lua_tostring(L, base + 1);
        char funcname[255] = { 0 };
        strcpy(funcname, func.c_str());
        func = _strlwr(funcname);

        if (func == "getresolutionlist") {
        } else if (func == "cd") {
            string_t str_path = s2w(lua_tostring(L, base + 2));
            SetCurrentDirectory(str_path.c_str());
        } else if (func == "FakeExplorer") {
            FakeExplorer();
        } else if (func == "file::doverb") {
            string_t file = s2w(lua_tostring(L, base + 2));
            string_t verb = s2w(lua_tostring(L, base + 3));
            varstr_expand(file);
            DoFileVerb(file.c_str(), verb.c_str());
        } else if (func == "taskbar::changenotify") {
            // g_Globals._SHSettingsChanged(0, TEXT("TraySettings"));
            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("TraySettings")));
        } else if (func == "system::changecolorthemenotify") {
            // g_Globals._SHSettingsChanged(0, TEXT("ImmersiveColorSet"));
            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("ImmersiveColorSet")));
        } else if (func == "taskbar::autohide") {
            if (lua_isinteger(L, base + 2)) {
                int val = (int)lua_tointeger(L, base + 2);
                AutoHideTaskBar(val == 1);
            }
        } else if (func == "taskbar::autohidestate") {
            v.iVal = TaskBarAutoHideState();
            PUSH_INT(v);
        } else if (func == "taskbar::pin") {
            string_t str_file = s2w(lua_tostring(L, base + 2));
            // Search target
            TCHAR tzTarget[MAX_PATH] = { 0 };
            TCHAR *target = CompletePath((TCHAR *)str_file.c_str(), tzTarget);
            if (!target) return 0;
            FakeExplorer();
            //CoInitialize(NULL);
            DoFileVerb(target, TEXT("taskbarpin"));
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
            FakeExplorer();
            DoFileVerb(str_lnk.c_str(), TEXT("taskbarunpin"));
        } else if (func == "desktop::updatewallpaper") {
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
        } else if (func == "screen::get") {
            v.str = s2w(lua_tostring(L, base + 2));
            if (v.str == TEXT("x") || v.str == TEXT("width")) {
                v.iVal = GetSystemMetrics(SM_CXSCREEN);
            } else if (v.str == TEXT("y") || v.str == TEXT("height")) {
                v.iVal = GetSystemMetrics(SM_CYSCREEN);
            } else if (v.str == TEXT("rotation")) {
                v.iVal = GetScreenRotation();
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
            }
            PUSH_INT(v);
        } else if (func == "volume::mute") {
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
        } else if (func == "gettickcount") {
            v.iVal = GetTickCount();
            PUSH_INT(v);
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
                int bewait = (int)lua_tointeger(L, base + 2);
            }
            if (bewait == 0) {
                //TODO: Thread
                PlaySoundA(lua_tostring(L, base + 2), NULL, SND_FILENAME);
            } else {
                PlaySoundA(lua_tostring(L, base + 2), NULL, SND_FILENAME);
            }
        } else if (func == "putenv") {
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
            int showflags = SW_SHOWNORMAL;
            bool wait = false;
            if (lua_isboolean(L, base + 3)) {
                wait = lua_toboolean(L, base + 3);
            }
            if (lua_isnumber(L, base + 4)) showflags = (int)lua_tointeger(L, base + 4);
            DWORD dwExitCode = Exec((PTSTR)cmd.c_str(), wait, showflags);
            v.iVal = (int)dwExitCode;
            PUSH_INT(v);
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
        } else if (func == "band") {
            UINT s = (UINT)lua_tointeger(L, base + 2);
            UINT b = (UINT)lua_tointeger(L, base + 3);
            v.iVal = (s & b);
            PUSH_INT(v);
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
        if (name == "cmdline") {
            v.str = g_Globals._cmdline;
            PUSH_STR(v);
        } else if (name == "winver") {
            if (top == base + 1) {
                v.str = g_Globals._winver;
            } else {
                v.str = FmtString(TEXT("%d.%d.%d.%d"),
                    g_Globals._winvers[0], g_Globals._winvers[1],
                    g_Globals._winvers[2], g_Globals._winvers[3]);
            }
            PUSH_STR(v);
        } else if (name == "langid") {
            v.str = g_Globals._langID;
            PUSH_STR(v);
        } else if (name == "locale") {
            v.str = g_Globals._locale;
            PUSH_STR(v);
        } else if (name == "iswinpe") {
            v.iVal = g_Globals._isWinPE;
            PUSH_INT(v);
        } else if (name == "path") {
            v.str = JVAR("JVAR_MODULEPATH").ToString();
            PUSH_STR(v);
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


    static const luaL_Reg lua_reg_app_funcs[] = {
        { "version", lua_app_version },
        { "call", lua_app_call },
        { "info", lua_app_info },
        { "jcfg", lua_app_jcfg },
        { "print", lua_print },
#ifdef _DEBUG
        { "stack", lua_stack },
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
"    app:print(\"----------------------------------------\")\n"
"    app:print(\"LUA ERROR: \" .. tostring(msg) .. \"\\n\")\n"
"    app:print(debug.traceback())\n"
"    app:print(\"----------------------------------------\")\n"
"end";

char *app_built_code =
"\n\
function app:jcfg(...)\n\
  return app.jcfg(...)\n\
end\n\
\n\
function app:run(...)\n\
  app.call('run', ...)\n\
  \n\
end";

static int lua_errorfunc = MININT;

void LuaAppEngine::init(string_t& file)
{
    L = luaL_newstate();
    HWND hwnd = LuaAppWindow::Create();
    _frame = (void *)LuaAppWindow::get_window(hwnd);
    ((LuaAppWindow *)_frame)->_lua = this;

    luaL_openlibs(L);
    luaL_requiref(L, "app", luaopen_app_module, 1);

    luaL_dostring(L, built_code_errorhandle);
    luaL_dostring(L, app_built_code);
    luaL_dofile(L, w2s(file).c_str());
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
    lua_getglobal(L, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        return 0;
    }
    return 1;
}

void LuaAppEngine::call(const char *funcname)
{
    char buff[1024] = { 0 };
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return;

    lua_getglobal(L, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(buff);
        return;
    }
    int rel = lua_pcall(L, 0, 0, lua_errorfunc);
    if (rel == -1) return;
}

void LuaAppEngine::call(const char *funcname, string_t& p1, string_t& p2)
{
    char buff[1024] = { 0 };
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return;

    lua_getglobal(L, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(buff);
        return;
    }
    lua_pushstring(L, w2s(p1).c_str());
    lua_pushstring(L, w2s(p2).c_str());
    int rel = lua_pcall(L, 2, 0, lua_errorfunc);
    if (rel == -1) return;
}

void LuaAppEngine::RunCode(string_t& code)
{
    luaL_dostring(L, w2s(code).c_str());
}

void LuaAppEngine::LoadFile(string_t& file)
{
    luaL_dofile(L, w2s(file).c_str());
}


void LuaAppEngine::onLoad()
{
    call("onload");
}

void LuaAppEngine::onFirstRun()
{
    call("onfirstrun");
}

void LuaAppEngine::preShell()
{
    call("preshell");
}

void LuaAppEngine::onShell()
{
    call("onshell");
}

void LuaAppEngine::onDaemon()
{
    call("ondaemon");
}

int LuaAppEngine::onClick(string_t& ctrl)
{
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return -1;

    lua_getglobal(L, "onclick");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find onclick() function");
        return -1;
    }
    lua_pushstring(L, w2s(ctrl).c_str());
    int rel = lua_pcall(L, 1, 0, lua_errorfunc);
    if (rel == -1) return -1;
    return (int)lua_tointeger(L, 1);
}

void LuaAppEngine::onTimer(int id)
{
    fetch_errorfunc(L);
    if (lua_errorfunc == MININT) return;

    lua_getglobal(L, "ontimer");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find ontimer() function");
        return;
    }
    lua_pushinteger(L, id);
    int rel = lua_pcall(L, 1, 0, lua_errorfunc);
    if (rel == -1) return;
}
