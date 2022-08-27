
#include <precomp.h>
#include <string>
#include <Windows.h>
#include "../resource.h"
#include "LuaEngine.h"



extern BOOL hasMSExplorer();

extern void wxsOpen(LPTSTR cmd);

extern LPVOID LoadCustomResource(UINT rID, LPTSTR rType);
extern BOOL FreeCustomResource(LPVOID res);


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

#ifndef LOGA
extern void _logA_(LPCSTR txt);

#define LOGA(txt) _logA_(txt)
#endif

extern "C" {
    const void *lua_app_addr = NULL;
    int lua_app_loglevel = 0;

    extern int lua_system_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_screen_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_volume_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_desktop_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_startmenu_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_taskbar_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_folderoptions_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_dialog_call(lua_State* L, const char *funcname, int top, int base);
    extern int lua_productpolicy_call(lua_State* L, const char *funcname, int top, int base);

#ifdef _DEBUG
    extern int lua_stack(lua_State* L);
#endif

    int lua_app_version(lua_State* L)
    {
        lua_pushstring(L, LUA_RELEASE);
        return 1;
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
        int rc = 0;

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

        if ((rc = lua_system_call(L, funcname, top, base)) != -1) {
            return rc;
        } else if (func.compare(0, 8, "screen::") == 0) {
            return lua_screen_call(L, funcname, top, base);
        } else if (func.compare(0, 8, "volume::") == 0) {
            return lua_volume_call(L, funcname, top, base);
        } else if (func.compare(0, 9, "desktop::") == 0) {
            return lua_desktop_call(L, funcname, top, base);
        } else if (func.compare(0, 9, "taskbar::") == 0) {
            return lua_taskbar_call(L, funcname, top, base);
        } else if (func.compare(0, 11, "startmenu::") == 0) {
            return lua_startmenu_call(L, funcname, top, base);
        } else if (func.compare(0, 15, "folderoptions::") == 0) {
            return lua_folderoptions_call(L, funcname, top, base);
        } else if (func.compare(0, 15, "productpolicy::") == 0) {
            return lua_productpolicy_call(L, funcname, top, base);
        } else if (func.compare(0, 8, "dialog::") == 0) {
            return lua_dialog_call(L, funcname, top, base);
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
        } else if (func == "utf8toansi") {
            lua_pushstring(L, (utf8toansi(lua_tostring(L, base + 2))).c_str());
            ret++;
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
        } else if (func == "wxs_open") {
            v.str = s2w(lua_tostring(L, base + 2));
            wxsOpen((LPTSTR)v.str.c_str());
            v.iVal = 0;
            PUSH_INT(v);
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
            char buff[100] = {0};
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

        Value jVal;
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string key1 = lua_tostring(L, 1);
        if (top == 1) {
            jVal = JCfg_GetValue(&g_JCfg, s2w(key1), Value());
        } else {
            luaL_checktype(L, 2, LUA_TSTRING);
            std::string key2 = lua_tostring(L, 2);
            if (top == 2) {
                jVal = JCfg_GetValue(&g_JCfg, s2w(key1), s2w(key2), Value());
            } else {
                luaL_checktype(L, 3, LUA_TSTRING);
                std::string key3 = lua_tostring(L, 3);
                jVal = JCfg_GetValue(&g_JCfg, s2w(key1), s2w(key2), s2w(key3), Value());
            }
        }
        if (jVal.GetType() == NULLVal) {
            lua_pushnil(L);
        } else if (jVal.GetType() == StringVal) {
            lua_pushstring(L, w2s(jVal.ToString()).c_str());
        } else if (jVal.GetType() == IntVal) {
            lua_pushinteger(L, jVal.ToInt());
        } else if (jVal.GetType() == FloatVal) {
            lua_pushnumber(L, jVal.ToFloat());
        } else if (jVal.GetType() == DoubleVal) {
            lua_pushnumber(L, jVal.ToDouble());
        } else if (jVal.GetType() == BoolVal) {
            lua_pushboolean(L, jVal.ToBool());
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    int lua_app_var(lua_State* L)
    {
        int ret = 0;
        Token v = { TOK_UNSET };
        int base = 0;
        int top = lua_gettop(L);

        if (top <= 0) return 0;

        if (lua_type(L, 1) == LUA_TTABLE) {
            //skip self for app:call
            base++;
            top--;
        }

        if (top <= 1) return 0;

        Value jVal;
        string_t varname = TEXT("");

        luaL_checktype(L, base + 1, LUA_TSTRING);
        varname = s2w(lua_tostring(L, base + 1));
        
        if ((lua_type(L, (base + 2)) == LUA_TSTRING)) {
            v.str = s2w(lua_tostring(L, base + 2));
            jVal = Value(v.str);
        } else if (lua_isboolean(L, base + 2)) {
            v.bVal = lua_toboolean(L, base + 2);
            jVal = Value(v.bVal);
        } else if (lua_isinteger(L, base + 2)) {
            v.iVal = (int)lua_tointeger(L, base + 2);
            jVal = Value(v.iVal);
        } else if (lua_isnil(L, base + 2)) {
            jVal = Value();
        } else if (lua_isnumber(L, base + 2)) {
            v.type = TOK_NUMBER;
            v.dVal = lua_tonumber(L, base + 2);
            jVal = Value(v.dVal);
        } else {
            v.str = TEXT("UNKNOWN VAR TYPE");
            jVal = Value(v.str);
        }
        g_JVARMap[varname] = jVal;
        return 0;
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
            MessageBoxA(NULL, s, "", MB_OK);
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
        { "JVar", lua_app_var },
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
App:SetVar('LogLevel', App.INFO_LEVEL)\n\
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
function App:Var(...)\n\
  return App.JVar(...)\n\
end\n\
\n\
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

EXTERN_C {
    extern int luaopen_winapi_lib(lua_State *L);
}

LuaAppEngine::LuaAppEngine(string_t& file)
{
    init(file);
}

LuaAppEngine::~LuaAppEngine()
{
    lua_close(L);
}

void LuaAppEngine::init(string_t& file)
{
    HWND hwnd = LuaAppWindow::Create();
    L = luaL_newstate();
    _frame = (void *)LuaAppWindow::get_window(hwnd);
    ((LuaAppWindow *)_frame)->_lua = this;

    _name = "App";

    luaL_openlibs(L);
    luaopen_winapi_lib(L);

    luaL_requiref(L, _name, luaopen_app_module, 1);
    luaL_dostring(L, built_code_errorhandle);
    luaL_dostring(L, app_built_code);

    LuaApp.Init(L, _name);

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
            const char *msg = lua_tostring(L, -1);
            LOGA(msg);
            MessageBoxA(NULL, msg, "WinXShell", MB_OK);
        }
    }
    FreeCustomResource(rescode);

    rescode = (char *)LoadCustomResource(IDR_APP_HELPERS, TEXT("FILE"));
    if (rescode) {
        if (luaL_dostring(L, rescode) != 0) {
            const char *msg = lua_tostring(L, -1);
            LOGA(msg);
            MessageBoxA(NULL, msg, "WinXShell", MB_OK);
        }
    }
    FreeCustomResource(rescode);

    if (PathFileExists(file.c_str())) {
        if (luaL_dofile(L, w2s(file).c_str()) != 0) {
            LOGA(lua_tostring(L, -1));
        }
    }
}

int LuaAppEngine::call(const char *funcname, int nres)
{
    return LuaApp.Call(funcname, nres);
}

int LuaAppEngine::call(const char *funcname, int p1, int p2, int nres)
{
    return LuaApp.Call(funcname, p1, p2, nres);
}

int LuaAppEngine::call(const char *funcname, string_t& p1, string_t& p2, int nres)
{
    return LuaApp.Call(funcname, p1, p2, nres);
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
    LuaApp.Call(":onLoad");
}

void LuaAppEngine::onFirstShellRun()
{
    LuaApp.Call(":_onFirstShellRun");
    LuaApp.Call(":onFirstShellRun");
}

void LuaAppEngine::preShell()
{
    LuaApp.Call(":_PreShell");
    LuaApp.Call(":PreShell");
}

void LuaAppEngine::onShell()
{
    LuaApp.Call(":_onShell");
    LuaApp.Call(":onShell");
}

void LuaAppEngine::onDaemon()
{
    LuaApp.Call(":_onDaemon");
    LuaApp.Call(":onDaemon");
}

int LuaAppEngine::onClick(string_t& ctrl)
{
    if (LuaApp._errorfunc == MININT) return -1;

    lua_getglobal(L, "Shell:onClick");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find onclick() function");
        return -1;
    }
    lua_pushstring(L, w2s(ctrl).c_str());
    int rel = lua_pcall(L, 1, 0, LuaApp._errorfunc);
    if (rel == -1) return -1;
    return (int)lua_tointeger(L, 1);
}


int LuaAppEngine::onTimer(int id)
{
    int self = LuaApp.GetFunc(":_onTimer");
    if (self == -1) return -1;

    lua_pushinteger(L, id);
    int rel = lua_pcall(L, self + 1, 0, LuaApp._errorfunc);
    if (rel == -1) return -1;
    return 0;
}
