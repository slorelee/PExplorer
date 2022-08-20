#include "precomp.h"

#include "LuaEngine.h"

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


EXTERN_C {

    int lua_taskbar_call(lua_State* L, const char *funcname, int top, int base)
    {
        int ret = 0;
        Token v = { TOK_UNSET };

        std::string func = funcname;
        if (func == "taskbar::changenotify") {
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
