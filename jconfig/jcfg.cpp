
#include <string>
#include <fstream>
#include <sstream>
#include <windows.h>
#include "..\vendor\json.h"
#include "jcfg.h"

using namespace std;
using namespace json;

#define CP_UNICODE -1

#define JCFG_MERGEFLAG_NONE 0
#define JCFG_MERGEFLAG_OVERWRITE 1

Object  g_JVARMap;
Object  g_JCfg;

#define DEF_TASKBARHEIGHT 40
int g_JCfg_taskbar_iconsize = 32;
int g_JCfg_taskbar_startmenu_iconsize = 32;

//default jcfg data
const wstring def_jcfg = L"{\"JS_SYSTEMINFO\":{\"langid\":\"0\"},"
                         L"\"JS_VERBMENUNAME\":{\"2052\":{\"rename\":\"重命名(&M)\",\"cmdhere\":\"在此处打开命令窗口(&W)\"}},"
                         L"\"JS_VERBMENUCOMMAND\":{\"cmdhere\":{\"command\":\"cmd.exe\",\"parameters\":\"/k \\\"CD /D %s\\\"\"}},"
                         L"\"JS_FILEEXPLORER\":{\"3rd_filename\":\"\"},"
                         L"\"JS_DESKTOP\":{\"bkcolor\": [0,0,0],\"wallpaperstyle\":0,\"wallpaper\":\"##{JVAR_MODULEPATH}\\\\wallpaper.bmp\"},"
                         L"\"JS_TASKBAR\":{\"notaskbar\":false,\"theme\":\"dark\",\"bkcolor\":[0,0,0],\"bkcolor2\":[0,122,204],\"textcolor\":\"0xffffff\","
                         L"\"userebar\":false,\"rebarlock\":false,\"padding-top\":0,"
                         L"\"smallicon\":false,\"height\":40,\"icon_size\":32,\"*x600\":{\"height\":32,\"icon_size\":16}},"
                         L"\"JS_STARTMENU\":{\"text\":\"\"},"
                         L"\"JS_QUICKLAUNCH\":{\"3rd_startup_arguments\":\"\",\"maxiconsinrow\":8},"
                         L"\"JS_NOTIFYAREA\":{\"notifyicon_size\":16,\"padding-left\":20,\"padding-right\":20}}";

std::string
ReadTextFile(string filename)
{
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string contents(buffer.str());
    return contents;
}

/*
    string s1, s2;
    StringCodeChange(s1.c_str(), CP_ACP, s2, CP_UTF8);
*/
inline void
StringCodeChange(LPCTSTR src, UINT _srcCode, string &dest, UINT _destCode)
{
    int len = 0;
    WCHAR *srcTemp = (WCHAR *)src;
    char *destTemp = NULL;
    if (_srcCode != CP_UNICODE) {
        len = MultiByteToWideChar(_srcCode , 0, src, -1, NULL, 0);
        srcTemp = new WCHAR[len];
        MultiByteToWideChar(_srcCode , 0, src, -1, srcTemp, len);
    }

    len = WideCharToMultiByte(_destCode, 0, srcTemp, -1, NULL, 0, NULL, NULL);
    destTemp = new char[len];
    WideCharToMultiByte(_destCode, 0, srcTemp, -1, destTemp, len, NULL, NULL);

    dest = destTemp;

    if (_srcCode != CP_UNICODE) delete []srcTemp;
    delete []destTemp;
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

Object
Load_JCfg(string filename)
{
    string istr = ReadTextFile(filename);
    string defstr = TEXT("");
    string cstr = TEXT("");
    const char *pstr = istr.c_str();

    StringCodeChange((LPCTSTR)def_jcfg.c_str(), CP_UNICODE, defstr, CP_ACP);
    Object def_config = Deserialize(defstr).ToObject();
    Object jcfg = def_config;

    if (strlen(pstr) > 3) {
        if ((unsigned char)pstr[0] == (unsigned char)0xEF &&
            (unsigned char)pstr[1] == (unsigned char)0xBB &&
             (unsigned char)pstr[2] == (unsigned char)0xBF) {
            StringCodeChange(pstr + 3, CP_UTF8, cstr, CP_ACP);
        } else {
             StringCodeChange(pstr, CP_UTF8, cstr, CP_ACP);
        }
        jcfg = Deserialize(cstr).ToObject();
        Update_KeyName(&jcfg);
        Merge_JCfg(&jcfg, &def_config, JCFG_MERGEFLAG_NONE);
    }
    Object test = jcfg["JS_DESKTOP"].ToObject();
    g_JCfg = jcfg;
    return jcfg;
}

inline void
string_replace(string &s1, const string &s2, const string &s3)
{
    string::size_type pos = 0;
    string::size_type a = s2.size();
    string::size_type b = s3.size();
    while ((pos = s1.find(s2, pos)) != string::npos) {
        s1.replace(pos, a, s3);
        pos += b;
    }
}

void
ExpendJString(Value *v)
{
    string val = v->ToString();
    if (val.length() < 3) return;
    if (val[0] != TEXT('#')) return;
    val = val.substr(1);
    for (Object::ValueMap::iterator it = g_JVARMap.begin(); it != g_JVARMap.end(); ++it) {
        string exp = TEXT("#{") + it->first + TEXT("}");
        if (val.find(TEXT("#{")) != string::npos) {
            string_replace(val, exp, it->second.ToString());
        }
    }
    *v = val;
}

Value
JCfg_GetValue(Object *jcfg, string key1, Value defval)
{
    if (jcfg->HasKey(key1) == false) return defval;
    Value v = (*jcfg)[key1];
    if (v.GetType() == StringVal) {
        ExpendJString(&v);
    }
    return v;
}

Value
JCfg_GetValue(Object *jcfg, string key1, string key2, Value defval)
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
JCfg_GetValue(Object *jcfg, string key1, string key2, string key3, Value defval)
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
JCfg_GetValue(Object *jcfg, string key1, string key2, string key3, string key4, Value defval)
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
        string color_str = val.ToString();
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
