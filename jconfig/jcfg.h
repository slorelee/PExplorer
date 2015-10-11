
#include <string>
#include "..\vendor\json.h"

using namespace json;

extern Object	g_JVARMap;
extern Object	g_JCfg;

extern Object Load_JCfg(string filename);
extern COLORREF JValueToColor(Value val);

extern Value JCfg_GetValue(Object *jcfg, string key1);
extern Value JCfg_GetValue(Object *jcfg, string key1, string key2);
extern Value JCfg_GetValue(Object *jcfg, string key1, string key2, string key3);

#define JVAR(key) (g_JVARMap[TEXT(key)])
#define JCFG1(key1) JCfg_GetValue(&g_JCfg, TEXT(key1))
#define JCFG2(key1, key2) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2))
#define JCFG3(key1, key2, key3) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2), TEXT(key3))

#define JCFG_QL(n, ...) (JCFG##n("JS_QUICKLAUNCH", __VA_ARGS__))

#define DESKTOP_BKCOLOR() (JValueToColor(JCFG2("JS_DESKTOP", "bkcolor")))
#define TASKBAR_BKCOLOR() (JValueToColor(JCFG2("JS_TASKBAR", "bkcolor")))
#define TASKBAR_TEXTCOLOR() (JValueToColor(JCFG2("JS_TASKBAR", "textcolor")))
#define TASKBAR_BRUSH() (CreateSolidBrush(TASKBAR_BKCOLOR()))
#define CLOCK_TEXT_COLOR() TASKBAR_TEXTCOLOR()

