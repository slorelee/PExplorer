

#include <Windows.h>
#include <oleacc.h>
#include "../utility/utility.h"
#include "../utility/window.h"
#include "../jconfig/jcfg.h"
#include "../globals.h"

extern ExplorerGlobals g_Globals;

#define WM_CLOCKAREA_EVENT (WM_USER + 100)
#define HM_CLOCKAREA_CLICKED 1

void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                             LONG idObject, LONG idChild,
                             DWORD dwEventThread, DWORD dwmsEventTime);

// Global variable.
static HWINEVENTHOOK g_evthook = NULL;
static HWND g_daemon = NULL;
static HWND g_clockarea = NULL;

void InstallEventHook(HWND hwnd)
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

    g_daemon = hwnd;
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
}


HWND WinXShell_DaemonWindow::Create()
{
    static WindowClass wcDaemonWindow(TEXT("WINXSHELL_DAEMONWINDOW"));
    HWND hwnd = Window::Create(WINDOW_CREATOR(WinXShell_DaemonWindow),
                               WS_EX_NOACTIVATE, wcDaemonWindow, TEXT("WINXSHELL_DAEMONWINDOW"), WS_POPUP,
                               0, 0, 0, 0, 0);
    return hwnd;
}

#define CLOCKAREA_CLICK_TIMER 1001

static void ClockArea_OnClick(HWND hwnd, int isDbClick)
{
    if (hwnd) KillTimer(hwnd, CLOCKAREA_CLICK_TIMER);
    if (g_Globals._lua) {
        string_t btn = TEXT("tray_clockarea");
        if (isDbClick) btn = TEXT("tray_clockarea(double)");
        if (g_Globals._lua->onClick(btn) == 0) return;
    }
    if (isDbClick) {
        CommandHook(hwnd, TEXT("clockarea_dbclick"), TEXT("JS_DAEMON"));
    } else {
        CommandHook(hwnd, TEXT("clockarea_click"), TEXT("JS_DAEMON"));
    }
}

LRESULT WinXShell_DaemonWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
    static int isDbClick = 0;
    if (nmsg == WM_TASKBARCREATED) {
        InstallEventHook(_hwnd);
    } else if (nmsg == WM_CLOCKAREA_EVENT) {
#ifdef _DEBUG
        PrintMessage(0, nmsg, wparam, lparam);
#endif
        if (wparam == HM_CLOCKAREA_CLICKED) {
            //MessageBox(NULL, TEXT("HM_CLOCKAREA_CLICKED"), TEXT(""), 0);
            SetTimer(_hwnd, CLOCKAREA_CLICK_TIMER, 500, NULL);
            isDbClick++;
        }
        return S_OK;
    } else if (nmsg == WM_TIMER) {
        if (wparam == CLOCKAREA_CLICK_TIMER) {
            ClockArea_OnClick(_hwnd, (isDbClick>1) ? 1 : 0);
            isDbClick = 0;
            return S_OK;
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

int daemon_entry()
{
    // Initializes COM
    CoInitialize(NULL);
    HWND daemon = create_daemonwindow();
    g_Globals._hwndDaemon = daemon;
    if (g_Globals._lua) g_Globals._lua->onDaemon();
    bool instHook = false;
    //default install the hook if running in WinPE
    TCHAR drv[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("SystemDrive"), drv, MAX_PATH);
    if (drv[0] == TEXT('x') || drv[0] == TEXT('X')) instHook = true;

    //hijack clockarea click event
    if (JCFG2_DEF("JS_DAEMON", "handle_clockarea_click", instHook).ToBool() != FALSE) {
        // sets up the event hook.
        InstallEventHook(daemon);
    }
    update_property_handler();
    Window::MessageLoop();
    CoUninitialize();
    return 0;
}
