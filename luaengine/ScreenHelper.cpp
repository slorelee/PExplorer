#include "precomp.h"

#include <sstream>
#include "LuaEngine.h"
#include "../systemsettings/MonitorAdapter.h"

extern int GetCurrentDPIScaling(int x);
extern int GetRecommendedDPIScaling();
extern void SetDpiScaling(int scale);

extern int GetScreenBrightness();
extern int SetScreenBrightness(int brightness);

template <class T>
string_t ConvertToString(T value) {
#if _UNICODE
    std::wstringstream ss;
#else
    std::stringstream ss;
#endif
    ss << value;
    return ss.str();
}

vector<string_t> split(string_t str, string_t pattern)
{
    string_t::size_type pos;
    vector<string_t> result;
    str += pattern; //À©Õ¹×Ö·û´®ÒÔ·½±ã²Ù×÷
    size_t size = str.size();

    for (size_t i = 0; i<size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            string_t s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}

Token GetResolutionList(int n)
{
    Token t = { TOK_UNSET };
    t.str = _T("");
    t.type = TOK_STRING;

    MonitorAdapter m_monitorAdapter;       //ÏÔÊ¾Æ÷
    VEC_MONITORMODE_INFO vecMointorListInfo;
    m_monitorAdapter.GetCurrentDisplayMode(vecMointorListInfo);
    if (vecMointorListInfo.size() == 0) {
        t.iVal = -1;
        return t;
    }

    t.type = TOK_STRARR;
    vector<MonitorModeInfo>::iterator itBeg = vecMointorListInfo[0].m_vecModeInfo.begin();
    for (int i = 0; i < vecMointorListInfo[0].m_vecModeInfo.size(); i++) {
        if (i > 0) t.str.append(_T("\n"));
        string_t buff = ConvertToString(vecMointorListInfo[0].m_vecModeInfo[i].m_nWidth)
            + _T("x") + ConvertToString(vecMointorListInfo[0].m_vecModeInfo[i].m_nHeight);
        t.str.append(buff);
        t.iVal = i + 1;
        if (n == t.iVal) break;
        itBeg++;
    }

    return t;
}

Token GetCurrentResolution()
{
    MonitorAdapter m_monitorAdapter;
    int w, h, f, b;
    m_monitorAdapter.GetCurrentResolution(w, h, f, b);
    Token t = { TOK_UNSET };
    t.type = TOK_STRING;
    t.str = ConvertToString(w) + _T("x") + ConvertToString(h);
    return t;
}

Token SetResolutionByStr(string_t wh) {
    Token t = { TOK_UNSET };
    t.type = TOK_INTEGER;
    vector<string_t> arr = split(wh, _T("x"));
    int width = _tstoi(arr[0].c_str());
    int height = _tstoi(arr[1].c_str());
    MonitorAdapter m_monitorAdapter;       //ÏÔÊ¾Æ÷
    VEC_MONITORMODE_INFO vecMointorListInfo;
    t.iVal = m_monitorAdapter.ChangeMonitorResolution(NULL, width, height);
    return t;
}

/*
Token SetResolution(SUIInterpreter *ipr)
{
    CDUIWindow *pFrame = ipr->m_pFrame;
    vector<Token *> *stack = &(ipr->m_Stack);

    int argn = PopInt(stack, pFrame);
    int width = 0;
    int height = 0;

    if (argn == 1) {
        Token t = Pop(stack, pFrame);
        t = ipr->GetValue(&t);
        if (t.type == TOK_STRING) {
            string_t wh = t.str;
            vector<string_t> arr = split(wh, _T("x"));
            width = _tstoi(arr[0].c_str());
            height = _tstoi(arr[1].c_str());
        }
    } else if (argn == 2) {
        width = PopInt(stack, pFrame);
        height = PopInt(stack, pFrame);
    }

    //TODO:argn > 2

    Token t = { TOK_UNSET };
    t.type = TOK_INTEGER;
    t.bVal = TRUE;
    MonitorAdapter m_monitorAdapter;       //ÏÔÊ¾Æ÷
    VEC_MONITORMODE_INFO vecMointorListInfo;
    m_monitorAdapter.ChangeMonitorResolution(NULL, width, height);
    return t;
}
*/

Token GetRotation()
{
    DWORD orientation = GetScreenRotation();
    Token t = { TOK_UNSET };
    t.type = TOK_INTEGER;
    t.iVal = (int)orientation;
    return t;
}

/*
Token SetRotation(SUIInterpreter *ipr)
{
    CDUIWindow *pFrame = ipr->m_pFrame;
    vector<Token *> *stack = &(ipr->m_Stack);
    int argn = PopInt(stack, pFrame);

    Token t = Pop(stack, pFrame);
    t = ipr->GetValue(&t);
    DWORD orientation = t.iVal;
    SetScreenRotation(orientation);

    Token r = { TOK_UNSET };
    r.type = TOK_INTEGER;
    r.bVal = TRUE;
    return r;
}
*/

EXTERN_C {
    int lua_screen_call(lua_State* L, const char *funcname, int top, int base) {
        int ret = 0;
        Token v = { TOK_UNSET };
        TCHAR method[255] = { 0 };
        std::string func = funcname;
        if (func == "screen::get") {
            MonitorAdapter m_monitorAdapter;
            int w, h, f, b;
            m_monitorAdapter.GetCurrentResolution(w, h, f, b);
            v.str = s2w(lua_tostring(L, base + 2));
            lstrcpy(method, v.str.c_str());
            v.str = _wcslwr(method);
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
            lstrcpy(method, v.str.c_str());
            v.str = _wcslwr(method);
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

}