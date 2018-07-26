

#include <Windows.h>
#include "../utility/utility.h"
#include "../utility/window.h"
#include "../jconfig/jcfg.h"
#include "../globals.h"

extern ExplorerGlobals g_Globals;

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

    /* for MessageHook */
    typedef BOOL(*pSetHook)(DWORD, int);
    typedef BOOL(*pRemoveHook)(void);

    pSetHook SetHook = NULL;
    pRemoveHook RemoveHook = NULL;
    UINT *pUWM_HOOKMESSAGE = 0;

}

extern void InstallHook(HWND hwnd, int reHook);
void InitHook(HWND hwnd)
{
#ifdef _WIN64
    TCHAR DllPath[] = TEXT("wxsStub.dll");
#else
    TCHAR DllPath[] = TEXT("wxsStub32.dll");
#endif
    HINSTANCE Hook_Dll = LoadLibrary(DllPath);
    if (Hook_Dll) {
        SetHook = (pSetHook)GetProcAddress(Hook_Dll, "SetHook");
        RemoveHook = (pRemoveHook)GetProcAddress(Hook_Dll, "RemoveHook");
        pUWM_HOOKMESSAGE = (UINT *)GetProcAddress(Hook_Dll, "UWM_HOOKMESSAGE");

        InstallHook(hwnd, 0);
    } else {
        MessageBox(NULL, TEXT("LoadLibrary Error"), TEXT("Error"), MB_ICONERROR);
    }
}

void InstallHook(HWND hwnd, int reHook)
{
    BOOL rc = FALSE;
    HWND hObjWnd = NULL;
    DWORD dwObjThreadId = 0;

    if (!SetHook) return;
    //Progman Shell_TrayWnd TrayClockWClass
    hObjWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (!hObjWnd) return;
    hObjWnd = FindWindowEx(hObjWnd, 0, TEXT("TrayNotifyWnd"), NULL);
    if (!hObjWnd) return;
    hObjWnd = FindWindowEx(hObjWnd, 0, TEXT("TrayClockWClass"), NULL);
    if (!hObjWnd) return;
    dwObjThreadId = GetWindowThreadProcessId(hObjWnd, NULL);

    if (dwObjThreadId != 0) {
        rc = SetHook(dwObjThreadId, reHook);
    }

    if (rc) {
        /*if (dwObjThreadId)
        MessageBox(NULL, "Thread Hook", "Success", MB_OK);
        else
        MessageBox(NULL, "System Hook", "Success", MB_OK);*/
        PostMessage(hObjWnd, *pUWM_HOOKMESSAGE, (WPARAM)hwnd, (LPARAM)hObjWnd);
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
    RemoveHook();
}


HWND WinXShell_DaemonWindow::Create()
{
    static WindowClass wcDaemonWindow(TEXT("WINXSHELL_DAEMONWINDOW"));
    HWND hwnd = Window::Create(WINDOW_CREATOR(WinXShell_DaemonWindow),
                               WS_EX_NOACTIVATE, wcDaemonWindow, TEXT("WINXSHELL_DAEMONWINDOW"), WS_POPUP,
                               0, 0, 0, 0, 0);
    return hwnd;
}

#define HM_CLOCKAREA_CLICKED 1
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
        InstallHook(_hwnd, 1);
    } else if (pUWM_HOOKMESSAGE && nmsg == *pUWM_HOOKMESSAGE) {
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
    HWND daemon = create_daemonwindow();
    g_Globals._hwndDaemon = daemon;
    if (g_Globals._lua) g_Globals._lua->call("ondaemon");
    bool instHook = false;
    //default install the hook if running in WinPE
    TCHAR drv[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("SystemDrive"), drv, MAX_PATH);
    if (drv[0] == TEXT('x') || drv[0] == TEXT('X')) instHook = true;

    //hijack clockarea click event
    if (JCFG2_DEF("JS_DAEMON", "handle_clockarea_click", instHook).ToBool() != FALSE) {
        InitHook(daemon);
    }
    update_property_handler();
    Window::MessageLoop();
    return 0;
}
