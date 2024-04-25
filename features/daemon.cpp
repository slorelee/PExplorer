

#include <Windows.h>
#include <oleacc.h>
#include "../utility/utility.h"
#include "../utility/window.h"
#include "../jconfig/jcfg.h"
#include "../globals.h"

extern ExplorerGlobals g_Globals;

void update_property_handler();

extern void CreateBrightnessLayer(HINSTANCE hInstance);

#define WM_CLOCKAREA_EVENT (WM_USER + 100)
#define HM_CLOCKAREA_CLICKED 1

#define CLOCKAREA_CLICK_TIMER 1001
#define DISABLE_SHOWDESKTOP_TIMER 1002
#define CAPSLOCK_DOUBLE_TIMER 1010
#define CAPSLOCK_RESET_TIMER 1011

void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                             LONG idObject, LONG idChild,
                             DWORD dwEventThread, DWORD dwmsEventTime);

extern UINT *pUWM_HOOKMESSAGE;
extern void EnableShowDesktop(HWND hwnd, UINT tid);
extern void RemoveShowDesktopHookEntry();

// Global variable.
static HWINEVENTHOOK g_evthook = NULL;
static HWND g_daemon = NULL;
static HWND g_clockarea = NULL;

void InstallEventHook()
{
    BOOL rc = FALSE;
    HWND hObjWnd = NULL;
    DWORD dwObjProcessId = 0;
    //Progman Shell_TrayWnd TrayClockWClass
    hObjWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (!hObjWnd) return;
    hObjWnd = FindWindowEx(hObjWnd, 0, TEXT("TrayNotifyWnd"), NULL);
    if (!hObjWnd) return;
    hObjWnd = FindWindowEx(hObjWnd, 0, TEXT("TrayClockWClass"), NULL);
    if (!hObjWnd) return;
    GetWindowThreadProcessId(hObjWnd, &dwObjProcessId);

    if (g_evthook != NULL) {
        UnhookWinEvent(g_evthook);
        g_evthook = NULL;
    }
    g_clockarea = hObjWnd;

    if (dwObjProcessId != 0) {
        g_evthook = SetWinEventHook(
            EVENT_SYSTEM_CAPTUREEND, EVENT_SYSTEM_CAPTUREEND,  // only EVENT_SYSTEM_CAPTUREEND event (9).
            NULL,                                          // Handle to DLL.
            HandleWinEvent,                                // The callback.
            dwObjProcessId, 0,              // Process and thread IDs of interest (0 = all)
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS); // Flags.
    }
}

// Callback function that handles events.
//
void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                             LONG idObject, LONG idChild,
                             DWORD dwEventThread, DWORD dwmsEventTime)
{
#ifdef _DEBUG
    char buff[100];
    sprintf_s(buff, "event=0x%x, hwnd=0x%Ix, id=0x%x\r\n", event, (UINT_PTR)hwnd, idChild);
    OutputDebugStringA(buff);
#endif // _DEBUG
    if (hwnd != g_clockarea) return;
    SendMessage(g_daemon, WM_CLOCKAREA_EVENT, HM_CLOCKAREA_CLICKED, 0);
}

// Ctrl+Alt+Del handler
// Ctrl+Shift+Esc handler
// CapsLock handler
static BOOL handle_CAD_press = TRUE;
static BOOL handle_CAPS_double = FALSE;
static int nCAPSPressed = 0;

static HHOOK g_hKeyEventHook = NULL;
TCHAR TASKMANAGER[] = TEXT("TaskMgr.exe");
LRESULT CALLBACK HandleKeyEventProc(INT iCode, WPARAM wParam, LPARAM lParam)
{
    if (handle_CAD_press) {
        if ((iCode == HC_ACTION) && (wParam == WM_KEYDOWN) && (((LPKBDLLHOOKSTRUCT)lParam)->vkCode == VK_DELETE)) {
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
                Exec(TASKMANAGER, FALSE);
                return TRUE;
            }
        }  else if ((iCode == HC_ACTION) && (wParam == WM_KEYDOWN) && (((LPKBDLLHOOKSTRUCT)lParam)->vkCode == VK_ESCAPE)) {
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                Exec(TASKMANAGER, FALSE);
                return TRUE;
            }
        }
    }
    if (handle_CAPS_double) {
        if ((iCode == HC_ACTION) && (wParam == WM_KEYDOWN) && (((LPKBDLLHOOKSTRUCT)lParam)->vkCode == VK_CAPITAL)) {
            nCAPSPressed++;
            SetTimer(g_daemon, CAPSLOCK_RESET_TIMER, 500, NULL);
            if (nCAPSPressed >= 2) {
                nCAPSPressed = 0;
                SetTimer(g_daemon, CAPSLOCK_DOUBLE_TIMER, 100, NULL);
            }
        }
    }

    return CallNextHookEx(g_hKeyEventHook, iCode, wParam, lParam);
}

VOID InstallKeyEventHook(BOOL Install)
{
    if (Install) {
        if (!g_hKeyEventHook) {
            g_hKeyEventHook = SetWindowsHookEx(WH_KEYBOARD_LL, HandleKeyEventProc, g_hInst, 0);
        }
    } else {
        if (g_hKeyEventHook) {
            UnhookWindowsHookEx(g_hKeyEventHook);
            g_hKeyEventHook = NULL;
        }
    }
}



struct WinXShell_DaemonWindow : public Window {
    typedef Window super;
    WinXShell_DaemonWindow(HWND hwnd);
    ~WinXShell_DaemonWindow();
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
    static HWND Create();
protected:
    const UINT WM_TASKBARCREATED;
};


WinXShell_DaemonWindow::WinXShell_DaemonWindow(HWND hwnd)
    : super(hwnd), WM_TASKBARCREATED(RegisterWindowMessage(WINMSG_TASKBARCREATED))
{
}

WinXShell_DaemonWindow::~WinXShell_DaemonWindow()
{
    if (g_evthook) UnhookWinEvent(g_evthook);
    RemoveShowDesktopHookEntry();
}


HWND WinXShell_DaemonWindow::Create()
{
    static WindowClass wcDaemonWindow(TEXT("WINXSHELL_DAEMONWINDOW"));
    HWND hwnd = Window::Create(WINDOW_CREATOR(WinXShell_DaemonWindow),
                               WS_EX_NOACTIVATE, wcDaemonWindow, TEXT("WINXSHELL_DAEMONWINDOW"), WS_POPUP,
                               0, 0, 0, 0, 0);
    return hwnd;
}

static void TrayClock_OnClick(HWND hwnd, int isDblClick)
{
    if (hwnd) KillTimer(hwnd, CLOCKAREA_CLICK_TIMER);
    if (g_Globals._lua) {
        char *luaFunc = "TrayClock:onClick";
        if (isDblClick) luaFunc = "TrayClock:onDblClick";
        if (g_Globals._lua->call(luaFunc) == 0) return;
    }
    if (isDblClick) {
        CommandHook(hwnd, TEXT("clockarea_dbclick"), TEXT("JS_DAEMON"));
    } else {
        CommandHook(hwnd, TEXT("clockarea_click"), TEXT("JS_DAEMON"));
    }
}

static bool IsX()
{
    static int inited = -1;
    if (inited != -1) return inited == 1;
    inited = 0;
    //default install the hook if running in WinPE
    TCHAR drv[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("SystemDrive"), drv, MAX_PATH);
    if (drv[0] == TEXT('x') || drv[0] == TEXT('X')) inited = 1;

    return inited == 1;
}

static void InstallEventHookEntry()
{
    if (g_Globals._isShell) return;
    //hijack clockarea click event
    if (JCFG2_DEF("JS_DAEMON", "handle_clockarea_click", IsX()).ToBool() != FALSE) {
        // sets up the event hook.
        InstallEventHook();
    }
}

void InstallKeyEventHookEntry()
{
    handle_CAD_press = FALSE;
    handle_CAPS_double = FALSE;
    // Ctrl+Alt+Del handler
    if (JCFG2_DEF("JS_DAEMON", "handle_CAD_press", IsX()).ToBool() != FALSE) {
        handle_CAD_press = TRUE;
    }
    // CAPS Lock handler
    if (JCFG2_DEF("JS_DAEMON", "handle_CAPS_double", false).ToBool() != FALSE) {
        handle_CAPS_double = TRUE;
    }
    if (handle_CAD_press || handle_CAPS_double) {
        // sets up the KEYBOARD hook.
        InstallKeyEventHook(TRUE);
    }
}

LRESULT WinXShell_DaemonWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    static int isDblClick = 0;
    if (nmsg == WM_TASKBARCREATED) {
        InstallEventHookEntry();
        EnableShowDesktop(_hwnd, DISABLE_SHOWDESKTOP_TIMER);
    } else if (nmsg == WM_CLOCKAREA_EVENT) {
        if (g_Globals._isDebug) {
            PrintMessage(0, nmsg, wparam, lparam);
        }
        if (wparam == HM_CLOCKAREA_CLICKED) {
            //MessageBox(NULL, TEXT("HM_CLOCKAREA_CLICKED"), TEXT(""), 0);
            SetTimer(_hwnd, CLOCKAREA_CLICK_TIMER, 500, NULL);
            isDblClick++;
        }
        return S_OK;
    } else if (nmsg == WM_TIMER) {
        if (wparam == CLOCKAREA_CLICK_TIMER) {
            TrayClock_OnClick(_hwnd, (isDblClick>1) ? 1 : 0);
            isDblClick = 0;
            return S_OK;
        } else if (wparam == DISABLE_SHOWDESKTOP_TIMER) {
            HWND hObjWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
            if (hObjWnd) SendMessage(hObjWnd, WM_USER + 0x1BA, 1, 0);
            return S_OK;
        } else if (wparam == CAPSLOCK_DOUBLE_TIMER) {
            KillTimer(_hwnd, CAPSLOCK_DOUBLE_TIMER);
            if (g_Globals._lua) {
                string_t hotkey = TEXT("CAPSLOCK x 2");
                string_t dmy = TEXT("");
                g_Globals._lua->call("Shell:_onHotKey", hotkey, dmy);
            }
            return S_OK;
        } else if (wparam == CAPSLOCK_RESET_TIMER) {
            nCAPSPressed = 0;
            KillTimer(_hwnd, CAPSLOCK_DOUBLE_TIMER);
            KillTimer(_hwnd, CAPSLOCK_RESET_TIMER);
            return S_OK;
        }
    } else if (nmsg == WM_DISPLAYCHANGE) {
        // Desktop::UpdateWallpaper
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
        // Taskbar::ChangeNotify
        SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("TraySettings")));
        // Trigger Event Function
        if (g_Globals._lua) {
            g_Globals._lua->call("WxsHandler.DisplayChangedHandler");
        }
#if 0 
        // fixscreen
        HWND hObjWnd = NULL;
        SHChangeNotify(0x8000000, 0, 0, 0);
        SendMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
        hObjWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
        if (hObjWnd) {
            APPBARDATA apBar = { 0 };
            apBar.cbSize = sizeof(APPBARDATA);
            SHAppBarMessage(ABM_GETSTATE, &apBar);
        }
#endif
        // fallthough
    } else if (pUWM_HOOKMESSAGE && nmsg == *pUWM_HOOKMESSAGE) {
        switch (wparam) {
        case 1:
            g_Globals._desktop.ToggleMinimize();
            break;
        }
    }
    return super::WndProc(nmsg, wparam, lparam);
}

class CReg {
public:
    HKEY m_hkey;
    CReg::CReg(HKEY hKey, LPCTSTR lpSubKey)
    {
        m_Res = 0; m_hkey = NULL;
        m_Res = RegOpenKey(hKey, lpSubKey, &m_hkey);
    }
    CReg::~CReg() { if (m_hkey) RegCloseKey(m_hkey); }
    LSTATUS CReg::Write(TCHAR *value, TCHAR *data)
    {
        String buff = data;
        if (m_Res) return m_Res;
        return RegSetValueEx(m_hkey, value, 0, REG_SZ, (LPBYTE)buff.c_str(), (DWORD)(buff.length() * sizeof(TCHAR)));
    }
    LSTATUS CReg::Write(TCHAR *value, DWORD data)
    {
        if (m_Res) return m_Res;
        return RegSetValueEx(m_hkey, value, 0, REG_DWORD, (LPBYTE)(&data), sizeof(DWORD));
    }
private:
    LONG m_Res;
};

/*
;override system properties
[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties]
"Position"="Bottom"
@="@shell32.dll,-33555"
;@="&Property" I don't found out the resource with shortcut for every language now, use 4177 instead.

[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\shell\properties\command]
@="WinXShell.exe -ui -jcfg UI_SystemInfo\\main.jcfg"
*/
void update_property_handler()
{
    if (JCFG2_DEF("JS_DAEMON", "update_properties_name", true).ToBool() == FALSE) {
        return;
    }
    int mid = JCFG2_DEF("JS_DAEMON", "properties_menu", 220).ToInt();
    CReg reg_prop(HKEY_CLASSES_ROOT, TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shell\\properties"));
    if (!reg_prop.m_hkey) return;
    TCHAR namebuffer[MAX_PATH];
    HINSTANCE res = LoadLibrary(TEXT("shell32.dll"));
    if (!res) return;
    HMENU menu = LoadMenu(res, MAKEINTRESOURCE(mid));
    if (!menu) return;
    if (!GetMenuString(menu, 0, namebuffer, MAX_PATH, MF_BYCOMMAND)) {
        FreeLibrary(res);
        return;
    }
    FreeLibrary(res);
    reg_prop.Write(NULL, namebuffer);

    /*reg_prop.Write(TEXT("Position"), TEXT("Bottom"));

    CReg reg_prop_cmd(HKEY_CLASSES_ROOT, TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shell\\Property\\command"));
    reg_prop_cmd.Write(NULL, TEXT("WinXShell.exe -ui -jcfg UI_SystemInfo\\main.jcfg"));

    CReg reg_no_default_prop(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
    reg_no_default_prop.Write(TEXT("NoPropertiesMyComputer"), 1);*/
}

HWND create_daemonwindow()
{
     return WinXShell_DaemonWindow::Create();
}

int daemon_entry(int standalone)
{
    if (standalone != 0) {
        // Initializes COM
        CoInitialize(NULL);
    }
    HWND daemon = create_daemonwindow();
    g_Globals._hwndDaemon = daemon;
    g_daemon = daemon;

    if (standalone != 0) {
        if (g_Globals._lua) g_Globals._lua->onDaemon();
    }

    bool val = JCFG2_DEF("JS_DAEMON", "handle_showdesktop", (g_Globals._isWinPE == TRUE)).ToBool();
    JVAR("HandleShowDesktop") = val;
    val = JCFG2_DEF("JS_DAEMON", "disable_showdesktop", false).ToBool();
    JVAR("DisableShowDesktop") = val;

    InstallEventHookEntry();
    InstallKeyEventHookEntry();
    EnableShowDesktop(daemon, DISABLE_SHOWDESKTOP_TIMER);
    update_property_handler();

    int brightness = JCFG2_DEF("JS_DAEMON", "screen_brightness", 100).ToInt();
    TCHAR buff[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("SCREEN_BRIGHTNESS"), buff, MAX_PATH);
    if (buff[0] != '\0') {
        brightness = _tstoi(buff);
    }
    if (brightness > 1) {
        CreateBrightnessLayer(g_Globals._hInstance);
    }

    if (standalone != 0) {
        Window::MessageLoop();
        CoUninitialize();
    }
    return 0;
}
