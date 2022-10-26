
#include <string>
#include "..\vendor\json.h"

using namespace std;
using namespace json;

extern Object   g_JVARMap;
extern Object   g_JCfg;

extern int g_JCfg_taskbar_iconsize;
extern int g_JCfg_taskbar_startmenu_iconsize;

extern int g_JCfg_DPI_SX;
extern int g_JCfg_DPI_SY;

extern HBRUSH g_JCfg_taskbar_bkbrush;
extern COLORREF g_JCfg_taskbar_textcolor;
extern string_t g_JCfg_taskbar_themestyle;

extern Object Load_JsonCfg(string_t filename);
extern Object Load_JCfg(string_t filename);
extern COLORREF JValueToColor(Value val);

extern Value JCfg_GetValue(Object *jcfg, string_t key1, Value defval);
extern Value JCfg_GetValue(Object *jcfg, string_t key1, string_t key2, Value defval);
extern Value JCfg_GetValue(Object *jcfg, string_t key1, string_t key2, string_t key3, Value defval);
extern Value JCfg_GetValue(Object *jcfg, string_t key1, string_t key2, string_t key3, string_t key4, Value defval);

extern int JCfg_GetDesktopBarHeight();
extern bool JCfg_TaskThumbnailEnabled();

#define FILE_JCFG1(fileobj, key1) JCfg_GetValue(&fileobj, TEXT(key1), Value())
#define FILE_JCFG1_DEF(fileobj, key1, defval) JCfg_GetValue(&fileobj, TEXT(key1), Value(defval))
#define FILE_JCFG2_DEF(fileobj, key1, key2, defval) JCfg_GetValue(&fileobj, TEXT(key1), key2, Value(defval))

#define FILE_SET_JCFG1(fileobj, key1) fileobj[TEXT(key1)]
#define FILE_SET_JCFG2(fileobj, key1, key2) fileobj[TEXT(key1)][TEXT(key2)]

#define JVAR(key) (g_JVARMap[TEXT(key)])
#define JCFG1(key1) JCfg_GetValue(&g_JCfg, TEXT(key1), Value())
#define JCFG2(key1, key2) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2), Value())
#define JCFG3(key1, key2, key3) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2), TEXT(key3), Value())

#define JCFG2_DEF(key1, key2, defval) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2), Value((defval)))
#define JCFG3_DEF(key1, key2, key3, defval) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2), TEXT(key3), Value((defval)))

#define JCFG2U_DEF(key1, key2, defval) JCfg_GetValue(&g_JCfg, TEXT(key1), (key2), Value((defval)))
#define JCFG3U_DEF(key1, key2, key3, defval) JCfg_GetValue(&g_JCfg, TEXT(key1), TEXT(key2), (key3), Value((defval)))

#define JCFG_CMD(key1, key2, key3, defval) JCfg_GetValue(&g_JCfg, TEXT(key1), key2, TEXT(key3), Value((defval)))
#define JCFG_CMDW(key1, key2, key3, defval) JCfg_GetValue(&g_JCfg, key1, key2, TEXT(key3), Value((defval)))

#define SET_JCFG1(key1) g_JCfg[TEXT(key1)]
#define SET_JCFG2(key1, key2) g_JCfg[TEXT(key1)][TEXT(key2)]
#define SET_JCFG3(key1, key2, key3) g_JCfg[TEXT(key1)][TEXT(key2)][TEXT(key3)]

#define JCFG_VMN(key) (JCfg_GetValue(&g_JCfg, TEXT("JS_VERBMENUNAME"), g_Globals._langID, TEXT(key), Value()))
#define JCFG_VMC(key1, key2) (JCfg_GetValue(&g_JCfg, TEXT("JS_VERBMENUCOMMAND"), TEXT(key1), TEXT(key2), Value()))
#define JCFG_SMC(key1, key2) (JCfg_GetValue(&g_JCfg, TEXT("JS_STARTMENU"), TEXT("commands"), TEXT(key1), TEXT(key2), Value()))
#define JCFG_SMC_DEF(key1, key2, defval) (JCfg_GetValue(&g_JCfg, TEXT("JS_STARTMENU"), TEXT("commands"), TEXT(key1), TEXT(key2), Value((defval))))

#define JCFG_TB(n, ...) (JCFG##n("JS_TASKBAR", __VA_ARGS__))
#define JCFG_QL(n, ...) (JCFG##n("JS_QUICKLAUNCH", __VA_ARGS__))

#define JCFG_TB_SET(n, ...) (SET_JCFG##n("JS_TASKBAR", __VA_ARGS__))
#define JCFG_QL_SET(n, ...) (SET_JCFG##n("JS_QUICKLAUNCH", __VA_ARGS__))

#define JCFG_THEME_DEF(key1, key2, key3, defval) (JCfg_GetValue(&g_JCfg, TEXT("JS_THEMES"), JCFG2(key1, "theme"), TEXT(key2), TEXT(key3), defval))
#define JCFG_THEME_COLOR(key1, key2, key3) (JCFG_THEME_DEF(key1, key2, key3, Value(0)))

#define DESKTOP_BKCOLOR() (JValueToColor(JCFG2("JS_DESKTOP", "bkcolor")))
#define TASKBAR_BKCOLOR() (JValueToColor(JCFG_THEME_COLOR("JS_TASKBAR", "taskbar", "bkcolor")))
#define TASKBAR_GETTEXTCOLOR() (JValueToColor(JCFG_THEME_COLOR("JS_TASKBAR", "taskbar", "textcolor")))
#define TASKBAR_TASKLINECOLOR() (JValueToColor(JCFG_THEME_COLOR("JS_TASKBAR", "taskbar", "task_line_color")))

#define TASKBAR_GETTHEMESTYLE() (JCFG_THEME_DEF("JS_TASKBAR", "taskbar", "style", TEXT("dark")))

#define TASKBAR_BRUSH() (g_JCfg_taskbar_bkbrush)
#define TASKBAR_TEXTCOLOR() (g_JCfg_taskbar_textcolor)
#define TASKBAR_THEMESTYLE() (g_JCfg_taskbar_themestyle)
#define CLOCK_TEXT_COLOR() (g_JCfg_taskbar_textcolor)

#define DESKTOPBARBAR_HEIGHT    DPI_SY(JCfg_GetDesktopBarHeight()) //(GetSystemMetrics(SM_CYSIZE) + 5 * GetSystemMetrics(SM_CYEDGE))
#define REBARBAND_HEIGHT        ((DESKTOPBARBAR_HEIGHT) - 2) //(GetSystemMetrics(SM_CYSIZE) + 3 * GetSystemMetrics(SM_CYEDGE))

#define DPI_SX(x) (MulDiv((x),g_JCfg_DPI_SX, 96))
#define DPI_SY(y) (MulDiv((y),g_JCfg_DPI_SY, 96))

#define STARTMENUROOT_ICON_SIZE     DPI_SX(g_JCfg_taskbar_startmenu_iconsize)
#define TASKBAR_ICON_SIZE           g_JCfg_taskbar_iconsize

