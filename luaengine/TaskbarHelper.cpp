#include "precomp.h"

#include "WindowCompositionAttribute.h"
#include "LuaEngine.h"

pfnSetWindowCompositionAttribute setWindowCompositionAttribute = NULL;

void AutoHideTaskBar(BOOL bHide)
{
#ifndef   ABM_SETSTATE 
#define   ABM_SETSTATE             0x0000000A 
#endif
    LPARAM lParam = ABS_ALWAYSONTOP;
    if (bHide) lParam = ABS_AUTOHIDE;

    APPBARDATA apBar = { 0 };
    apBar.cbSize = sizeof(APPBARDATA);
    apBar.hWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (apBar.hWnd != NULL) {
        apBar.lParam = lParam;
        SHAppBarMessage(ABM_SETSTATE, &apBar);
    }
}

int TaskBarAutoHideState()
{
    UINT uState = 0;
    APPBARDATA apBar = { 0 };
    apBar.cbSize = sizeof(APPBARDATA);
    uState = (UINT)SHAppBarMessage(ABM_GETSTATE, &apBar);
    if (uState && ABS_AUTOHIDE) return 1;
    return 0;
}

BOOL SetWindowTransparency(HWND hwnd, const TCHAR *mode, UINT transparency, COLORREF color) {
    BOOL retVal = FALSE;
    if (setWindowCompositionAttribute == NULL) {
        HMODULE huser = GetModuleHandle(L"user32.dll");
        setWindowCompositionAttribute = (pfnSetWindowCompositionAttribute)
            GetProcAddress(huser, "SetWindowCompositionAttribute");
    }
    if (setWindowCompositionAttribute) {
        ACCENT_POLICY accent = { ACCENT_ENABLE_BLURBEHIND, 0, 0x00000000, 0 };
        WINDOWCOMPOSITIONATTRIBDATA data;
        if (_tcsicmp(mode, TEXT("acrylic")) == 0) {
            accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
        } else if (_tcsicmp(mode, TEXT("transparent")) == 0) {
            accent.AccentState = ACCENT_ENABLE_TRANSPARENTGRADIENT;
            accent.AccentFlags = 2;
        }
        if (transparency > 100) transparency = 100;
        transparency = (100 - transparency) * 255 / 100;
        accent.GradientColor = color | (transparency << 24);
        data.Attrib = WCA_ACCENT_POLICY;
        data.pvData = &accent;
        data.cbData = sizeof(accent);
        retVal = setWindowCompositionAttribute(hwnd, &data);
    }
    return retVal;
}

void TaskbarTransparency(HWND hwnd, const TCHAR *mode, UINT transparency, COLORREF color)
{
    HWND taskbar = hwnd;
    TCHAR sysPathBuff[MAX_PATH] = { 0 };
    GetWindowsDirectory(sysPathBuff, MAX_PATH);
    string_t dwmPath = sysPathBuff;
#ifdef _WIN64
    dwmPath.append(TEXT("\\System32\\dwm.exe"));
#else
    dwmPath.append(TEXT("\\SysNative\\dwm.exe"));
#endif
    if (!PathFileExists(dwmPath.c_str())) {
        return;
    }
    if (!taskbar)taskbar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (taskbar) SetWindowTransparency(taskbar, mode, transparency, color);
}

EXTERN_C {

    int lua_taskbar_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };
        HWND taskbar = NULL;
        std::string func = funcname;
        if (func == "taskbar::changenotify") {
            if (g_Globals._winvers[2] >= (DWORD)26100) {
                taskbar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
                if (taskbar) {
                    SendMessage(taskbar, 1464, 0, 0); //HandleDisplayChange
                    SendMessage(taskbar, 1361, 0, 0); //HandleChangeNotify
                }
                /* {
                    HWND progman = FindWindow(TEXT("ProgMan"), NULL);
                    if (progman) {
                        SendMessageTimeout(progman, WM_USER + SPI_SETFONTSMOOTHING, 0, 0, SMTO_NORMAL, 1000, NULL);//0x44B(1099) or try  WM_NCCREATE 
                    }
                } */

                /*
                BOOL isPrim = FALSE;
                MONITORINFO monitorInfo = {0};
                monitorInfo.cbSize = sizeof(MONITORINFO);
                GetMonitorInfo(MonitorFromWindow(NULL, MONITOR_DEFAULTTONEAREST), &monitorInfo);
                if (monitorInfo.dwFlags == MONITORINFOF_PRIMARY)
                {
                    isPrim = TRUE;
                }

                LOGA("Shell_TrayWnd");
                HWND hWndTray = FindWindow(TEXT("Shell_TrayWnd"), NULL);
                if (FALSE == isPrim) {
                    hWndTray = FindWindow(TEXT("Shell_SecondaryTrayWnd"), NULL);
                }
                if (hWndTray && !(::GetWindowLong(hWndTray, GWL_EXSTYLE) & WS_EX_TOPMOST))
                {
                    SetWindowPos(hWndTray, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                }
                if (hWndStart && !(::GetWindowLong(hWndStart, GWL_EXSTYLE) & WS_EX_TOPMOST))
                {
                    SetWindowPos(hWndStart, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                }
                */
            }
            // g_Globals._SHSettingsChanged(0, TEXT("TraySettings"));
            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("TraySettings")));
        } else if (func == "taskbar::autohide") {
            if (lua_isinteger(L, base + 2)) {
                int val = (int)lua_tointeger(L, base + 2);
                AutoHideTaskBar(val == 1);
            }
        } else if (func == "taskbar::autohidestate") {
            v.iVal = TaskBarAutoHideState();
            PUSH_INT(v);
        } else if (func == "taskbar::settransparency") {
            string_t mode = s2w(lua_tostring(L, base + 2));
            int val = (int)lua_tointeger(L, base + 3);
            TaskbarTransparency(NULL, mode.c_str(), val, 0x0);
        } else if (func == "taskbar::pin") {
            if (isWinXShellAsShell()) return ret;
            string_t str_file = s2w(lua_tostring(L, base + 2));
            ShellContextMenuVerb(str_file.c_str(), TEXT("taskbarpin"));
        } else if (func == "taskbar::unpin") {
            /*
            function Taskbar:UnPin(name)
            menu_pintotaskbar()
            local pinned_path = [[%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar\]]
            app:call('Taskbar::UnPin', .. name .. '.lnk')
            end
            */
            string_t name = s2w(lua_tostring(L, base + 2));
            string_t str_lnk = TEXT("%APPDATA%\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\") + name + TEXT(".lnk");
            varstr_expand(str_lnk);
            if (isWinXShellAsShell()) {
                DeleteFile(str_lnk.c_str());
                return ret;
            }
            FakeExplorer();
            DoFileVerb(str_lnk.c_str(), TEXT("taskbarunpin"));
            return ret;
        }
        return ret;
    }

}
