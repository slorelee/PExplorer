
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include "../utility/utility.h"
#include "../utility/window.h"
#include "../jconfig/jcfg.h"
#include "../globals.h"

extern ExplorerGlobals g_Globals;

#define EmbeddinglWindowClass TEXT("WinXShell_EmbeddinglWindow")
struct EmbeddinglWindow : public Window {
    typedef Window super;
    EmbeddinglWindow(HWND hwnd);
    static HWND Create();
};

EmbeddinglWindow::EmbeddinglWindow(HWND hwnd)
    : super(hwnd)
{
}

HWND EmbeddinglWindow::Create()
{
    static WindowClass wcDesktopShellWindow(EmbeddinglWindowClass);
    HWND hwnd = Window::Create(WINDOW_CREATOR(EmbeddinglWindow),
                               WS_EX_NOACTIVATE, wcDesktopShellWindow, TEXT(""), WS_POPUP,
                               0, 0, 0, 0, 0);
    return hwnd;
}

void send_wxs_protocol_url(PWSTR pszName)
{
    if (g_Globals._lua) {
        string_t url = pszName;
        string_t dmy = TEXT("");
        g_Globals._lua->call(":WxsProtocol", url, dmy);
    }
}

#ifdef HANDLE_PROTOCOL_URL
extern int handle_wxs_protocol_url();
#endif
int embedding_entry()
{
    if (JCFG2_DEF("JS_DAEMON", "handle_ms-settings_url", true).ToBool() != FALSE) {
        if (FindWindow(EmbeddinglWindowClass, NULL)) return 0;
        EmbeddinglWindow::Create();
#ifdef HANDLE_PROTOCOL_URL
        handle_wxs_protocol_url();
#endif
        return 0;
    }
    return 0;
}
