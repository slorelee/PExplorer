
#include <string>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <Shlwapi.h>
#include "../vendor/json.h"
#include "jcfg.h"

using namespace std;
using namespace json;

#define CP_UNICODE -1

#define JCFG_MERGEFLAG_NONE 0
#define JCFG_MERGEFLAG_OVERWRITE 1

Object  g_JVARMap;
Object  g_JCfg;

#define DEF_TASKBARHEIGHT 40
int g_JCfg_taskbar_iconsize = 24;
int g_JCfg_taskbar_startmenu_iconsize = 24;
int g_JCfg_DPI_SX = 96;
int g_JCfg_DPI_SY = 96;
HBRUSH g_JCfg_taskbar_bkbrush = NULL;
COLORREF g_JCfg_taskbar_textcolor = 0;
string_t g_JCfg_taskbar_themestyle = TEXT("dark");

//default jcfg data
const wstring def_jcfg = L"{\"JS_SYSTEMINFO\":{\"langid\":\"0\"},"
                         L"\"JS_VERBMENUNAME\":{\"2052\":{\"refresh\":\"Refresh(&E)\",\"rename\":\"Rename(&M)\"}},"
                         L"\"JS_FILEEXPLORER\":{\"3rd_filename\":\"\"},"
                         L"\"JS_THEMES\":{"
                         L"\"default\":{\"taskbar\":{\"bkcolor\":[0,0,0],\"task_line_color\":[238,238,238],\"textcolor\":\"0xffffff\"}},"
                         L"\"blue\":{\"taskbar\":{\"bkcolor\":[0,120,215],\"task_line_color\":[176,176,176],\"textcolor\":\"0xffffff\"}},"
                         L"\"dark\":{\"taskbar\":{\"bkcolor\":[0,0,0],\"task_line_color\":[238,238,238],\"textcolor\":\"0xffffff\"}},"
                         L"\"light\":{\"taskbar\":{\"style\":\"light\",\"bkcolor\":[238,238,238],\"task_line_color\":[0,120,215],\"textcolor\":\"0x000000\"}}"
                         L"},"
                         L"\"JS_DESKTOP\":{"
                         L"\"bkcolor\":[0,0,0],\"wallpaperstyle\":0,"
                         L"\"wallpaper\":\"\","
//                         L"\"cascademenu\":{\"WinXNew\":\"Directory\\Background\\shell\\WinXNew\"},"
                         L"\"dummy\":0"
                         L"},"
                         L"\"JS_TASKBAR\":{\"notaskbar\":false,\"theme\":\"dark\",\"bkcolor\":[0,0,0],\"bkcolor2\":[0,122,204],\"textcolor\":\"0xffffff\","
                         L"\"userebar\":false,\"rebarlock\":false,\"padding-top\":0,"
                         L"\"smallicon\":false,\"height\":40,\"icon_size\":32,\"*x600\":{\"height\":32,\"icon_size\":16}},"
                         L"\"JS_STARTMENU\":{\"text\":\"\"},"
                         L"\"JS_QUICKLAUNCH\":{\"3rd_startup_arguments\":\"\",\"maxiconsinrow\":8},"
                         L"\"JS_NOTIFYAREA\":{\"notifyicon_size\":16,\"padding-left\":20,\"padding-right\":20}}";


/*
    string_t s1, s2;
    StringCodeChange(s1.c_str(), CP_ACP, s2, CP_UTF8);
*/
inline void
StringCodeChange(LPCCH src, UINT _srcCode, std::string &dest, UINT _destCode)
{
    int len = 0;
    WCHAR *srcTemp = (WCHAR *)src;
    CHAR *destTemp = NULL;

    if (_srcCode != CP_UNICODE) {
        len = MultiByteToWideChar(_srcCode , 0, (LPCCH)src, -1, NULL, 0);
        srcTemp = new TCHAR[len];
        MultiByteToWideChar(_srcCode , 0, (LPCCH)src, -1, srcTemp, len);
    }

    len = WideCharToMultiByte(_destCode, 0, srcTemp, -1, NULL, 0, NULL, NULL);
    destTemp = new CHAR[len];
    WideCharToMultiByte(_destCode, 0, srcTemp, -1, destTemp, len, NULL, NULL);

    dest = destTemp;

    if (_srcCode != CP_UNICODE) delete []srcTemp;
    delete []destTemp;
}

inline void
StringCodeChange(LPCCH src, UINT _srcCode, std::wstring &dest, UINT _destCode)
{
    int len = 0;
    WCHAR *destTemp = (WCHAR *)src;

    if (_srcCode != CP_UNICODE) {
        len = MultiByteToWideChar(_srcCode, 0, (LPCCH)src, -1, NULL, 0);
        destTemp = new WCHAR[len];
        MultiByteToWideChar(_srcCode, 0, (LPCCH)src, -1, destTemp, len);
    }
    dest = destTemp;
    if (_srcCode != CP_UNICODE) delete[]destTemp;
}

string
ReadTextFile(string_t filename)
{
    string ansi_filename;
#ifdef UNICODE
    StringCodeChange((LPCCH)filename.c_str(), CP_UNICODE, ansi_filename, CP_ACP);
#else
    ansi_filename = filename;
#endif
    ifstream t(ansi_filename);
    stringstream buffer;
    buffer << t.rdbuf();
    string contents(buffer.str());
    return contents;
}

void Merge_JCfg(Object *dst, Object *src, UINT flags)
{
    for (Object::ValueMap::iterator it = src->begin(); it != src->end(); ++it) {
        if (dst->find(it->first) == dst->end()) {
            (*dst)[it->first] = it->second;
        } else {
            if (it->second.GetType() == ObjectVal) {
                Merge_JCfg((*dst)[it->first].RefObject(), it->second.RefObject(), flags);
            } else {
                if (flags & JCFG_MERGEFLAG_OVERWRITE) {
                    (*dst)[it->first] = it->second;
                }
            }
        }
    }
}

static Object
GetKeyAliasNameList(Object *jcfg)
{
    Object KeyAliasList;
    if (jcfg->HasKey(TEXT("JS_JMACRO")) == false) return Object();
    Value jmacro_val = (*jcfg)[TEXT("JS_JMACRO")];
    if (jmacro_val.GetType() != ObjectVal) return Object();
    Object jmacro_obj = jmacro_val.ToObject();
    if (jmacro_obj.HasKey(TEXT("JKEYNAME")) == false) return Object();
    Value jkeyname_val = jmacro_obj[TEXT("JKEYNAME")];
    if (jkeyname_val.GetType() != ArrayVal) return Object();
    Array jkeyname_arr = jkeyname_val.ToArray();
    for (Array::ValueVector::iterator it = jkeyname_arr.begin(); it != jkeyname_arr.end(); ++it) {
        if (it->GetType() != ArrayVal || it->ToArray().size() < 2) return Object();
        Array jkeynamemap = it->ToArray();
        if (jkeynamemap[0].GetType() != StringVal) return Object();
        if (jkeynamemap[1].GetType() != StringVal) return Object();
        if (jkeynamemap[1].ToString().length() != 0) {
            KeyAliasList[jkeynamemap[1].ToString()] = jkeynamemap[0];
        }
    }
    return KeyAliasList;
}

static void
ReplaceKeyAliasName(Object *src, const Object *alias)
{
    Array removelist;
    for (Object::ValueMap::iterator it = src->begin(); it != src->end(); ++it) {
        if (it->second.GetType() == ObjectVal) {
            ReplaceKeyAliasName((*src)[it->first].RefObject(), alias);
        }
        if (alias->HasKey(it->first)) {
            (*src)[(*alias)[it->first]] = it->second;
            //(*src)[it->first].Clear();
            removelist.push_back(it->first);
        }
    }
    for (int i = 0; i < removelist.size(); i++) {
        src->Remove(removelist[i]);
    }
}

static void
Update_KeyName(Object *jcfg)
{
    Object KeyAliasList = GetKeyAliasNameList(jcfg);
    ReplaceKeyAliasName(jcfg, &KeyAliasList);
    return;
}

bool JCfg_GetDesktopBarUseSmallIcon();

static void
JCfg_init() {
    /* init taskbar background brush */
    g_JCfg_taskbar_bkbrush = CreateSolidBrush(TASKBAR_BKCOLOR());
    g_JCfg_taskbar_textcolor = TASKBAR_GETTEXTCOLOR();
    g_JCfg_taskbar_themestyle = TASKBAR_GETTHEMESTYLE();
    JCfg_GetDesktopBarUseSmallIcon();

    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen != NULL) {
        g_JCfg_DPI_SX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
        g_JCfg_DPI_SY = GetDeviceCaps(hdcScreen, LOGPIXELSY);
        ReleaseDC(NULL, hdcScreen);
    }
}

Object
Load_JsonCfg(string_t filename)
{
    string istr = ReadTextFile(filename);
    string_t cstr = TEXT("");
    const char *pstr = istr.c_str();

    Object jcfg;

    if (strlen(pstr) > 3) {
        if ((unsigned char)pstr[0] == (unsigned char)0xEF &&
            (unsigned char)pstr[1] == (unsigned char)0xBB &&
             (unsigned char)pstr[2] == (unsigned char)0xBF) {
            pstr += 3;
        }
        StringCodeChange((LPCCH)pstr, CP_UTF8, cstr, CP_ACP);
        jcfg = Deserialize(cstr).ToObject();
        Update_KeyName(&jcfg);
    }
    return jcfg;
}


Object
Load_JCfg(string_t filename)
{
    string_t defstr = TEXT("");
    StringCodeChange((LPCCH)def_jcfg.c_str(), CP_UNICODE, defstr, CP_ACP);
    Object def_config = Deserialize(defstr).ToObject();
    Object jcfg = Load_JsonCfg(filename);
    Merge_JCfg(&jcfg, &def_config, JCFG_MERGEFLAG_NONE);
    g_JCfg = jcfg;
    JCfg_init();
    return jcfg;
}


inline void
string_replace(string_t &s1, const string_t &s2, const string_t &s3)
{
    string_t::size_type pos = 0;
    string_t::size_type a = s2.size();
    string_t::size_type b = s3.size();
    while ((pos = s1.find(s2, pos)) != string_t::npos) {
        s1.replace(pos, a, s3);
        pos += b;
    }
}

void
ExpendJString(Value *v)
{
    string_t val = v->ToString();
    if (val.length() < 3) return;
    if (val[0] != TEXT('#')) return;
    val = val.substr(1);
    for (Object::ValueMap::iterator it = g_JVARMap.begin(); it != g_JVARMap.end(); ++it) {
        string_t exp = TEXT("#{") + it->first + TEXT("}");
        if (val.find(TEXT("#{")) != string_t::npos) {
            string_replace(val, exp, it->second.ToString());
        }
    }
    *v = val;
}

Value
JCfg_GetValue(Object *jcfg, string_t key1, Value defval)
{
    if (jcfg->HasKey(key1) == false) return defval;
    Value v = (*jcfg)[key1];
    if (v.GetType() == StringVal) {
        ExpendJString(&v);
    }
    return v;
}

Value
JCfg_GetValue(Object *jcfg, string_t key1, string_t key2, Value defval)
{
    if (jcfg->HasKey(key1) == false) return defval;
    if ((*jcfg)[key1].GetType() != ObjectVal) return defval;
    if ((*jcfg)[key1].HasKey(key2) == false) return defval;
    Value v = (*jcfg)[key1][key2];
    if (v.GetType() == StringVal) {
        ExpendJString(&v);
    }
    return v;
}

Value
JCfg_GetValue(Object *jcfg, string_t key1, string_t key2, string_t key3, Value defval)
{
    Value v = JCfg_GetValue(jcfg, key1, key2, Value());
    if (v.GetType() != ObjectVal) return defval;
    if (v.HasKey(key3) == false) return defval;

    v = v.ToObject()[key3];
    if (v.GetType() == StringVal) {
        ExpendJString(&v);
    }
    return v;
}

Value
JCfg_GetValue(Object *jcfg, string_t key1, string_t key2, string_t key3, string_t key4, Value defval)
{
    Value v = JCfg_GetValue(jcfg, key1, key2, key3, Value());
    if (v.GetType() != ObjectVal) return defval;
    if (v.HasKey(key4) == false) return defval;

    v = v.ToObject()[key4];
    if (v.GetType() == StringVal) {
        ExpendJString(&v);
    }
    return v;
}

COLORREF
JValueToColor(Value val)
{
    if (val.GetType() == ArrayVal) {
        Array arr_rgb = val.ToArray();
        return RGB(arr_rgb[0].ToInt(), arr_rgb[1].ToInt(), arr_rgb[2].ToInt());
    } else if (val.GetType() == StringVal) {
        string_t color_str = val.ToString();
        return std::stol(color_str, nullptr, 0);
    } else if (val.GetType() == IntVal) {
        return (COLORREF)val.ToInt();
    }
    return 0;
}

int
JCfg_GetDesktopBarHeight()
{
    static int height = -1;
    if (height == -1) {
        height = DEF_TASKBARHEIGHT;
        Value v = JCFG2("JS_TASKBAR", "height");
        if (v.GetType() == IntVal) {
            height = v;
        }
    }
    return height;
}

bool
JCfg_GetDesktopBarUseSmallIcon()
{
    static bool inited = false;
    bool usesmallicon = false;
    if (!inited) {
        inited = true;
        Value v = JCFG2("JS_TASKBAR", "smallicon");
        if (v.GetType() == BoolVal) {
            usesmallicon = v;
            if (usesmallicon) {
                g_JCfg_taskbar_iconsize = 16;
                g_JCfg_taskbar_startmenu_iconsize = 16;
                JCFG_TB_SET(2, "height") = 32;
            }
        } else if (v.GetType() == IntVal) {
            g_JCfg_taskbar_iconsize = v;
            g_JCfg_taskbar_startmenu_iconsize = v;
        }

    }
    return usesmallicon;
}

int
JCfg_GetDesktopBarHeightWithDPI()
{
    return DESKTOPBARBAR_HEIGHT;
}

bool
JCfg_TaskThumbnailEnabled()
{
    bool thumbnail = JCFG2_DEF("JS_TASKBAR", "thumbnail", true).ToBool();

    if (thumbnail) {
        TCHAR sysPathBuff[MAX_PATH] = { 0 };
        GetWindowsDirectory(sysPathBuff, MAX_PATH);
        string_t dwmPath = sysPathBuff;
#ifdef _WIN64
        dwmPath.append(_T("\\System32\\dwm.exe"));
#else
        dwmPath.append(_T("\\SysNative\\dwm.exe"));
#endif
        if (!PathFileExists(dwmPath.c_str())) {
            thumbnail = false;
        }
    }
    return thumbnail;
}
