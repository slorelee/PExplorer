
#include <precomp.h>
#include <string>
#include <Windows.h>
#include "LuaAppEngine.h"

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

        LuaAppEngine *lua = g_Globals._lua;

        if (func == "GetResolutionList") {
 
        } else if ((func == "SetTimer") || (func == "KillTimer")) {
            LuaAppWindow *frame = (LuaAppWindow *)lua->getFrame();
            if (func == "SetTimer") {
                SetTimer(frame->GetHWND(), (UINT_PTR)lua_tointeger(L, base + 2), (UINT)lua_tointeger(L, base + 3), NULL);
            } else {
                KillTimer(frame->GetHWND(), lua_tointeger(L, base + 2));
            }
        } else if (func == "GetTickCount") {
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
        } else if (func == "resstr") {
            v.str = s2w(lua_tostring(L, base + 2));
            resstr_expand(v.str);
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
        } else if (func == "band") {
            UINT s = (UINT)lua_tointeger(L, base + 2);
            UINT b = (UINT)lua_tointeger(L, base + 3);
            v.iVal = (s & b);
            PUSH_INT(v);
        } else {
            char buff[100];
            sprintf(buff, "error:fcuntion %s() is not implemented", func.c_str());
            LOGA(buff);
        }
        return ret;
    }

    int lua_app_info(lua_State* L)
    {
        int ret = 0;
        Token v = { TOK_UNSET };
        int base = 0;

        if (lua_type(L, 1) == LUA_TTABLE) {
            //skip self for app:call
            base++;
        }
        luaL_checktype(L, base + 1, LUA_TSTRING);
        std::string name = lua_tostring(L, base + 1);
        if (name == "cmdline") {
            v.str = g_Globals._cmdline;
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

static char *lua_built_code =
"\n\
suilib = app\n\
wxs = app\n\
\n\
function wxs:jcfg(...)\n\
  return app.jcfg(...)\n\
end\n\
\n\
function wxs:run(...)\n\
  app.call('run', ...)\n\
  \n\
end";

void LuaAppEngine::init(string_t& file)
{
    L = luaL_newstate();
    HWND hwnd = LuaAppWindow::Create();
    _frame = (void *)LuaAppWindow::get_window(hwnd);
    ((LuaAppWindow *)_frame)->_lua = this;

    luaL_openlibs(L);
    luaL_requiref(L, "app", luaopen_app_module, 1);

    luaL_dostring(L, built_code_errorhandle);
    luaL_dostring(L, lua_built_code);
    luaL_dofile(L, w2s(file).c_str());
}

static int reg_errorfunc(lua_State *L)
{
    int top = lua_gettop(L);
    lua_getglobal(L, "__G__TRACKBACK__");

    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find function <__G__TRACKBACK__>err");
        lua_settop(L, top);
        return MININT;
    }

    int errfunc = lua_gettop(L);
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
    int errfunc = reg_errorfunc(L);
    if (errfunc == MININT) return;

    lua_getglobal(L, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(buff);
        return;
    }
    int rel = lua_pcall(L, 0, 0, errfunc);
    if (rel == -1) return;
}

void LuaAppEngine::call(const char *funcname, string_t& p1, string_t& p2)
{
    char buff[1024] = { 0 };
    sprintf(buff, "[LUA ERROR] can't find %s() function", funcname);
    int errfunc = reg_errorfunc(L);
    if (errfunc == MININT) return;

    lua_getglobal(L, funcname);
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA(buff);
        return;
    }
    lua_pushstring(L, w2s(p1).c_str());
    lua_pushstring(L, w2s(p1).c_str());
    int rel = lua_pcall(L, 2, 0, errfunc);
    if (rel == -1) return;
}

void LuaAppEngine::onLoad()
{
    call("onload");
}

void LuaAppEngine::onFirstRun()
{
    call("onfirstrun");
}

void LuaAppEngine::onShell()
{
    call("onshell");
}

int LuaAppEngine::onClick(string_t& ctrl)
{
    int errfunc = reg_errorfunc(L);
    if (errfunc == MININT) return -1;

    lua_getglobal(L, "onclick");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find onclick() function");
        return -1;
    }
    lua_pushstring(L, w2s(ctrl).c_str());
    int rel = lua_pcall(L, 1, 0, errfunc);
    if (rel == -1) return -1;
    return (int)lua_tointeger(L, 1);
}

void LuaAppEngine::onTimer(int id)
{
    int errfunc = reg_errorfunc(L);
    if (errfunc == MININT) return;

    lua_getglobal(L, "ontimer");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        LOGA("[LUA ERROR] can't find ontimer() function");
        return;
    }
    lua_pushinteger(L, id);
    int rel = lua_pcall(L, 1, 0, errfunc);
    if (rel == -1) return;
}
