/*
 * Copyright 2003, 2004, 2005, 2006 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


//
// Explorer clone
//
// explorer.cpp
//
// Martin Fuchs, 23.07.2003
//
// Credits: Thanks to Leon Finker for his explorer cabinet window example
//


#include "precomp.h"

#include "resource.h"

#include <locale.h>    // for setlocale()

#include <wincon.h>

#ifndef _WIN32_WINNT_WINBLUE
#define _WIN32_WINNT_WINBLUE                0x0603
#endif

#include <VersionHelpers.h>

//#include "dialogs/settings.h"    // for MdiSdiDlg

#include "services/shellservices.h"
#include "jconfig/jcfg.h"

#include "DUI/UIManager.h"
#include "utility/LuaAppEngine.h"
#include "features/features.h"

DynamicLoadLibFct<void(__stdcall *)(BOOL)> g_SHDOCVW_ShellDDEInit(TEXT("SHDOCVW"), 118);

boolean DebugMode = FALSE;

extern ExplorerGlobals g_Globals;
boolean SelectOpt = FALSE;

void UIProcess(HINSTANCE hInst, String cmdline);
extern void InstallCADHookEntry();

bool FileTypeManager::is_exe_file(LPCTSTR ext)
{
    static const LPCTSTR s_executable_extensions[] = {
        TEXT("COM"),
        TEXT("EXE"),
        TEXT("BAT"),
        TEXT("CMD"),
        TEXT("CMM"),
        TEXT("BTM"),
        TEXT("AWK"),
        0
    };

    TCHAR ext_buffer[_MAX_EXT];
    const LPCTSTR *p;
    LPCTSTR s;
    LPTSTR d;

    for (s = ext + 1, d = ext_buffer; (*d = toupper(*s)); s++)
        ++d;

    for (p = s_executable_extensions; *p; p++)
        if (!lstrcmp(ext_buffer, *p))
            return true;

    return false;
}


const FileTypeInfo &FileTypeManager::operator[](String ext)
{
    ext.toLower();

    iterator found = find(ext);
    if (found != end())
        return found->second;

    FileTypeInfo &ftype = super::operator[](ext);

    ftype._neverShowExt = false;

    HKEY hkey;
    TCHAR value[MAX_PATH], display_name[MAX_PATH];
    LONG valuelen = sizeof(value);

    if (!RegQueryValue(HKEY_CLASSES_ROOT, ext, value, &valuelen)) {
        ftype._classname = value;

        valuelen = sizeof(display_name);
        if (!RegQueryValue(HKEY_CLASSES_ROOT, ftype._classname, display_name, &valuelen))
            ftype._displayname = display_name;

        if (!RegOpenKey(HKEY_CLASSES_ROOT, ftype._classname, &hkey)) {
            if (!RegQueryValueEx(hkey, TEXT("NeverShowExt"), 0, NULL, NULL, NULL))
                ftype._neverShowExt = true;

            RegCloseKey(hkey);
        }
    }

    return ftype;
}

LPCTSTR FileTypeManager::set_type(Entry *entry, bool dont_hide_ext)
{
    LPCTSTR ext = _tcsrchr(entry->_data.cFileName, TEXT('.'));

    if (ext) {
        const FileTypeInfo &type = (*this)[ext];

        if (!type._displayname.empty())
            entry->_type_name = _tcsdup(type._displayname);

        // hide some file extensions
        if (type._neverShowExt && !dont_hide_ext) {
            intptr_t len = ext - entry->_data.cFileName;

            if (entry->_display_name != entry->_data.cFileName)
                free(entry->_display_name);

            entry->_display_name = (LPTSTR) malloc((len + 1) * sizeof(TCHAR));
            lstrcpyn(entry->_display_name, entry->_data.cFileName, (int)len + 1);
        }

        if (is_exe_file(ext))
            entry->_data.dwFileAttributes |= ATTRIBUTE_EXECUTABLE;
    }

    return ext;
}


#ifndef ROSSHELL

bool ExplorerCmd::ParseCmdLine(LPCTSTR lpCmdLine)
{
    bool ok = true;

    LPCTSTR b = lpCmdLine;
    LPCTSTR p = b;

    while (*b) {
        // remove leading space
        while (_istspace((unsigned)*b))
            ++b;

        p = b;

        bool quote = false;

        // options are separated by ','
        for (; *p; ++p) {
            if (*p == '"')    // Quote characters may appear at any position in the command line.
                quote = !quote;
            else if (*p == ',' && !quote)
                break;
        }

        if (p > b) {
            intptr_t l = p - b;

            // remove trailing space
            while (l > 0 && _istspace((unsigned)b[l - 1]))
                --l;

            if (!EvaluateOption(String(b, (int)l)))
                ok = false;

            if (*p)
                ++p;

            b = p;
        }
    }

    return ok;
}

bool ExplorerCmd::EvaluateOption(LPCTSTR option)
{
    String opt_str;

    // Remove quote characters, as they are evaluated at this point.
    for (; *option; ++option)
        if (*option != TEXT('"'))
            opt_str += *option;

    option = opt_str;

    if (option[0] == TEXT('/')) {
        ++option;

        // option /e for windows in explorer mode
        if (!_tcsicmp(option, TEXT("e")))
            _flags |= OWM_EXPLORE;
        // option /root for rooted explorer windows
        else if (!_tcsicmp(option, TEXT("root")))
            _flags |= OWM_ROOTED;
        // non-standard options: /mdi, /sdi
        else if (!_tcsicmp(option, TEXT("mdi")))
            _mdi = true;
        else if (!_tcsicmp(option, TEXT("sdi")))
            _mdi = false;
        else if (!_tcsicmp(option, TEXT("n"))) {
            // Do nothing
        } else if (!_tcsicmp(option, TEXT("select"))) {
            SelectOpt = TRUE;
        } else
            return false;

    } else {
        if (!_path.empty())
            return false;

        if ((SelectOpt == TRUE) && (PathFileExists(option))) {
            _tcscpy(szPath, option);
            PathRemoveBackslash(szPath);
            _path = szPath;
        } else {
            _tcscpy(szPath, opt_str);
            _path = szPath;
        }
    }
    _option = SelectOpt ? 1 : 0;
    return true;
}

bool ExplorerCmd::IsValidPath() const
{
    if (!_path.empty()) {
        DWORD attribs = GetFileAttributes(_path);

        if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY))
            return true;    // file system path
        else if (*_path == ':' && _path.at(1) == ':')
            return true;    // text encoded IDL
    }

    return false;
}

#else

void explorer_show_frame(int cmdShow, LPTSTR lpCmdLine)
{
    if (!lpCmdLine)
        lpCmdLine = TEXT("explorer.exe");

    launch_file(GetDesktopWindow(), lpCmdLine, cmdShow);
}

#endif


PopupMenu::PopupMenu(UINT nid)
{
    HMENU hMenu = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(nid));
    _hmenu = GetSubMenu(hMenu, 0);
    RemoveMenu(hMenu, 0, MF_BYPOSITION);
    DestroyMenu(hMenu);
}


/// "About Explorer" Dialog
struct ExplorerAboutDlg : public
    CtlColorParent <
    OwnerDrawParent<Dialog>
    > {
    typedef CtlColorParent <
    OwnerDrawParent<Dialog>
    > super;

    ExplorerAboutDlg(HWND hwnd)
        :    super(hwnd)
    {
        SetWindowIcon(hwnd, IDI_WINXSHELL);

        new FlatButton(hwnd, IDOK);

        _hfont = CreateFont(20, 0, 0, 0, FW_BOLD, TRUE, 0, 0, 0, 0, 0, 0, 0, TEXT("Sans Serif"));
        new ColorStatic(hwnd, IDC_PE_EXPLORER, RGB(32, 32, 128), 0, _hfont);

        new HyperlinkCtrl(hwnd, IDC_WWW);

        FmtString ver_txt(ResString(IDS_EXPLORER_VERSION_STR), (LPCTSTR)ResString(IDS_VERSION_STR));
        SetWindowText(GetDlgItem(hwnd, IDC_VERSION_TXT), ver_txt);

        HWND hwnd_winver = GetDlgItem(hwnd, IDC_WIN_VERSION);
        SetWindowText(hwnd_winver, get_windows_version_str());
        SetWindowFont(hwnd_winver, g_Globals._hDefaultFont, FALSE);

        CenterWindow(hwnd);
    }

    ~ExplorerAboutDlg()
    {
        DeleteObject(_hfont);
    }

    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
    {
        switch (nmsg) {
        case WM_PAINT:
            Paint();
            break;

        default:
            return super::WndProc(nmsg, wparam, lparam);
        }

        return 0;
    }

    void Paint()
    {
        PaintCanvas canvas(_hwnd);

        HICON hicon = (HICON) LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(IDI_WINXSHELL_BIG), IMAGE_ICON, 0, 0, LR_SHARED);

        DrawIconEx(canvas, 20, 10, hicon, 0, 0, 0, 0, DI_NORMAL);
    }

protected:
    HFONT    _hfont;
};

void explorer_about(HWND hwndParent)
{
    Dialog::DoModal(IDD_ABOUT_EXPLORER, WINDOW_CREATOR(ExplorerAboutDlg), hwndParent);
}


static void InitInstance(HINSTANCE hInstance)
{
    CONTEXT("InitInstance");

    setlocale(LC_COLLATE, "");    // set collating rules to local settings for compareName

#ifndef ROSSHELL
    // register frame window class
    g_Globals._hframeClass = IconWindowClass(CLASSNAME_FRAME, IDI_EXPLORER);

    // register child window class
    WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC | CS_DBLCLKS).Register();

    // register tree window class
    WindowClass(CLASSNAME_WINEFILETREE, CS_CLASSDC | CS_DBLCLKS).Register();
#endif

    g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}

extern void send_wxs_protocol_url(PWSTR pszName);

int explorer_main(HINSTANCE hInstance, LPTSTR lpCmdLine, int cmdShow)
{
    CONTEXT("explorer_main");
    int rc = 0;
    // initialize Common Controls library
    CommonControlInit usingCmnCtrl;

    try {
        InitInstance(hInstance);
    } catch (COMException &e) {
        HandleException(e, GetDesktopWindow());
        return -1;
    }

#ifndef ROSSHELL
    if (cmdShow != SW_HIDE) {
        /*    // don't maximize if being called from the ROS desktop
                if (cmdShow == SW_SHOWNORMAL)
                        ///@todo read window placement from registry
                    cmdShow = SW_MAXIMIZE;
        */

        // ms-settings:xxxx
        String cmd_str = lpCmdLine;
        if (cmd_str.find(TEXT("ms-settings:")) == String::npos) {
            rc = explorer_open_frame(cmdShow, lpCmdLine, EXPLORER_OPEN_NORMAL);
        } else {
            ExplorerCmd cmd;
            if (lpCmdLine) cmd.ParseCmdLine(lpCmdLine);
            if (cmd._path.find(TEXT("ms-settings:")) == 0) {
                send_wxs_protocol_url((PWSTR)(cmd._path.c_str()));
            } else {
                rc = explorer_open_frame(cmdShow, lpCmdLine, EXPLORER_OPEN_NORMAL);
            }
        }
    }
#endif
    if (g_Globals._desktop_mode || rc == 1) {
        Window::MessageLoop();
    }
    return 1;
}

static bool SetShellReadyEvent(LPCTSTR evtName)
{
    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, evtName);
    if (!hEvent)
        return false;

    SetEvent(hEvent);
    CloseHandle(hEvent);

    return true;
}

EXTERN_C {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

EXTERN_C {
    extern int ShellHasBeenRun();
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
    CONTEXT("WinMain()");

    BOOL any_desktop_running = IsAnyDesktopRunning();

    BOOL startup_desktop = FALSE;

    // strip extended options from the front of the command line
    String ext_options;

    LPTSTR lpCmdLineOrg = lpCmdLine;
    while (*lpCmdLine == '-') {
        while (*lpCmdLine && !_istspace((unsigned)*lpCmdLine))
            ext_options += *lpCmdLine++;

        while (_istspace((unsigned)*lpCmdLine))
            ++lpCmdLine;
    }

    if (_tcsstr(ext_options, TEXT("-winpe"))) {
        g_Globals._isWinPE = TRUE;
        CloseShellProcess();
        any_desktop_running = FALSE;
    } else if (_tcsstr(ext_options, TEXT("-wes"))) {
        CloseShellProcess();
        any_desktop_running = FALSE;
    } else if (_tcsstr(ext_options, TEXT("-shell"))) {
        CloseShellProcess();
        any_desktop_running = FALSE;
    }

    // command line option "-install" to replace previous shell application with WinXShell
    if (_tcsstr(ext_options, TEXT("-install"))) {
        // install WinXShell into the registry
        TCHAR path[MAX_PATH];

        int l = GetModuleFileName(0, path, COUNTOF(path));
        if (l) {
            HKEY hkey;

            if (!RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hkey)) {

                ///@todo save previous shell application in config file

                RegSetValueEx(hkey, TEXT("Shell"), 0, REG_SZ, (LPBYTE)path, l * sizeof(TCHAR));
                RegCloseKey(hkey);
            }

            if (!RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hkey)) {

                ///@todo save previous shell application in config file

                RegSetValueEx(hkey, TEXT("Shell"), 0, REG_SZ, (LPBYTE)TEXT(""), l * sizeof(TCHAR));
                RegCloseKey(hkey);
            }
        }

        CloseShellProcess();

        startup_desktop = TRUE;
    } else {
        // create desktop window and task bar only, if there is no other shell and we are
        // the first explorer instance
        // MS Explorer looks additionally into the registry entry HKCU\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\shell,
        // to decide wether it is currently configured as shell application.
        startup_desktop = !any_desktop_running;
    }


    bool autostart = !any_desktop_running;

    // disable autostart if the SHIFT key is pressed
    if (GetAsyncKeyState(VK_SHIFT) < 0)
        autostart = false;

#ifdef _DEBUG    //MF: disabled for debugging
    autostart = false;
#endif

    _tsetlocale(LC_ALL, TEXT("")); //set locale for support multibyte character

    g_hInst = hInstance;
    g_Globals.init(hInstance); /* init icon_cache for UI process */

    g_Globals.read_persistent();
    g_Globals.get_modulepath();
    g_Globals.load_config();
    g_Globals.get_systeminfo();
    g_Globals._cmdline = lpCmdLineOrg;

    // for loading UI Resources, lua_helper
#ifndef _DEBUG
    SetCurrentDirectory(JVAR("JVAR_MODULEPATH").ToString().c_str());
#else
    if (_tcsstr(ext_options, TEXT("-cd"))) {
        SetCurrentDirectory(JVAR("JVAR_MODULEPATH").ToString().c_str());
    }
#endif

    // init default font
    if (JCFG2_DEF("JS_TASKBAR", "usesystemfont", true).ToBool() == FALSE) {
        g_Globals._hDefaultFont = GetStockFont(DEFAULT_GUI_FONT);
    } else {
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
        g_Globals._hDefaultFont = CreateFontIndirect(&(ncm.lfMessageFont));
    }

    if (_tcsstr(ext_options, TEXT("-console"))) {
        handle_console(g_Globals._log);
        LOG(TEXT("starting winxshell debug log\n"));
    }

    string_t file(_T("WinXShell.lua"));
    TCHAR luascript[MAX_PATH + 1] = { 0 };
    DWORD dw = GetEnvironmentVariable(TEXT("WINXSHELL_LUASCRIPT"), luascript, MAX_PATH);
    if (dw != 0) file = luascript;

#ifdef _DEBUG
    SetEnvironmentVariable(TEXT("WINXSHELL_DEBUG"), TEXT("1"));
#endif

    String mpath = JVAR("JVAR_MODULEPATH").ToString();
    SetEnvironmentVariable(TEXT("WINXSHELL_MODULEPATH"), mpath);
    if (_tcsstr(ext_options, TEXT("-ui")) == 0) {
#ifndef _DEBUG
        file = mpath + TEXT("\\") + file;
#endif
        g_Globals._lua = new LuaAppEngine(file);
    }

    TCHAR locale_buf[LOCALE_NAME_MAX_LENGTH];
    g_Globals._locale = _T("en-US");
    if (GetUserDefaultLocaleName(locale_buf, LOCALE_NAME_MAX_LENGTH) > 0) {
        g_Globals._locale = locale_buf;
    }
    SetEnvironmentVariable(TEXT("USERDEFAULT_LOCALENAME"), g_Globals._locale);

    CUIManager *pUIManager = NULL;
    if (_tcsstr(ext_options, TEXT("-uimgr"))) {
        HWND hwnd = CUIManager::GetUIManager();
        if (hwnd == NULL) {
            pUIManager = new CUIManager(hInstance, TRUE);
       }
    }

   if (g_Globals._lua) g_Globals._lua->onLoad();

    if (_tcsstr(ext_options, TEXT("-ui"))) {
#ifndef _DEBUG
        SetCurrentDirectory(JVAR("JVAR_MODULEPATH").ToString().c_str());
#endif
        g_Globals.get_uifolder();
        UIProcess(hInstance, lpCmdLineOrg);
        if (pUIManager) {
            Window::MessageLoop();
        }
        return g_Globals._exitcode;
    }

    // If there is given the command line option "-desktop", create desktop window anyways
    if (_tcsstr(ext_options, TEXT("-desktop")))
        startup_desktop = TRUE;
#ifndef ROSSHELL
    else if (_tcsstr(ext_options, TEXT("-nodesktop")))
        startup_desktop = FALSE;

    // Don't display cabinet window in desktop mode
    if (startup_desktop && !_tcsstr(ext_options, TEXT("-explorer")))
        nShowCmd = SW_HIDE;
#endif

    if (_tcsstr(ext_options, TEXT("-noautostart")))
        autostart = false;
    else if (_tcsstr(ext_options, TEXT("-autostart")))
        autostart = true;

    if (startup_desktop) {
        if (IsWindowsVistaOrGreater()) {
            // for Vista later
            if (!SetShellReadyEvent(TEXT("ShellDesktopSwitchEvent")))
                SetShellReadyEvent(TEXT("Global\\ShellDesktopSwitchEvent"));
        } else {
            // hide the XP login screen (Credit to Nicolas Escuder)
            // another undocumented event: "Global\\msgina: ReturnToWelcome"
            if (!SetShellReadyEvent(TEXT("msgina: ShellReadyEvent")))
                SetShellReadyEvent(TEXT("Global\\msgina: ShellReadyEvent"));
        }
    }
#ifdef ROSSHELL
    else
        return g_Globals._exitcode;    // no shell to launch, so exit immediatelly
#endif

    if (_tcsstr(ext_options, TEXT("-debug"))) {
        DebugMode = true;
        Sleep(10000);
    }

    if (_tcsstr(ext_options, TEXT("-break"))) {
        LOG(TEXT("debugger breakpoint"));
#ifdef _MSC_VER
        DebugBreak();
#else
        asm("int3");
#endif
    }

    if (_tcsstr(ext_options, TEXT("-test"))) {
        exit(g_Globals._exitcode);
    }

    // initialize COM and OLE before creating the desktop window
    OleInit usingCOM;

    // init common controls library
    CommonControlInit usingCmnCtrl;

    // Initializes COM
    CoInitialize(NULL);

    if (_tcsstr(ext_options, TEXT("-color"))) {
        UpdateSysColor(lpCmdLineOrg);
    }

    if (_tcsstr(ext_options, TEXT("-regist"))) {
        RegistAppPath();
    }

    TCHAR *code_opt = NULL;
    if (_tcsstr(ext_options, TEXT("-luacode"))) {
        code_opt = TEXT("-luacode");
    } else if (_tcsstr(ext_options, TEXT("-code"))) {
        code_opt = TEXT("-code");
    }

    if (code_opt) {
        String code = GetParameter(lpCmdLineOrg, code_opt, TRUE);
        if (g_Globals._lua) {
            g_Globals._lua->RunCode(code);
        }
        return g_Globals._exitcode;
    }

    if (_tcsstr(ext_options, TEXT("-script"))) {
        String file = GetParameter(lpCmdLineOrg, TEXT("-script"), TRUE);
        if (PathFileExists(file.c_str())) {
            if (g_Globals._lua) {
                g_Globals._lua->LoadFile(file);
            } else {
                new LuaAppEngine(file);
            }
        }
        return g_Globals._exitcode;
    }

    if (_tcsstr(ext_options, TEXT("-noaction"))) {
        return g_Globals._exitcode;
    }

    if (_tcsstr(ext_options, TEXT("-settings"))) {
        return g_Globals._exitcode;
    }

    if (_tcsstr(ext_options, TEXT("-ocf"))) {
        OpenContainingFolder(lpCmdLineOrg);
        return g_Globals._exitcode;
    }

    // wxs-ui:xxxx
    String cmd_str = lpCmdLine;
    if (cmd_str.find(TEXT("wxs-ui:")) != String::npos) {
        ExplorerCmd cmd;
        if (lpCmdLine) cmd.ParseCmdLine(lpCmdLine);
        if (cmd._path.find(TEXT("wxs-ui:")) == 0) {
            if (g_Globals._lua) {
                string_t url = cmd._path.c_str();
                string_t dmy = TEXT("");
                g_Globals._lua->call("wxs_ui", url, dmy);
            }
        }
        return g_Globals._exitcode;
    }

    // wxs-open:xxxx
    if (cmd_str.find(TEXT("wxs-open:")) != String::npos) {
        ExplorerCmd cmd;
        if (lpCmdLine) cmd.ParseCmdLine(lpCmdLine);
        if (cmd._path.find(TEXT("wxs-open:")) == 0) {
            if (g_Globals._lua) {
                wxsOpen((LPTSTR)cmd._path.c_str());
            }
        }
        return g_Globals._exitcode;
    }

    if (_tcsstr(ext_options, TEXT("-daemon"))) {
        return daemon_entry(1);
    }

    if (_tcsstr(ext_options, TEXT("-Embedding"))) {
        return embedding_entry();
    }

    if (startup_desktop) {
        WaitCursor wait;
        g_Globals._isShell = TRUE;

        {
            static WindowClass wcWinXShellShellWindow(WINXSHELL_SHELLWINDOW);
            wcWinXShellShellWindow.Register();
            CreateWindowEx(WS_EX_NOACTIVATE, WINXSHELL_SHELLWINDOW, TEXT(""),
                WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_Globals._hInstance, 0);
        }

        if (g_Globals._lua) g_Globals._lua->preShell();

        if (!_tcsstr(ext_options, TEXT("-keep_userprofile"))) {
            ChangeUserProfileEnv();
        }

        //create a ApplicationManager_DesktopShellWindow window for ClassicShell startmenu
        AM_DesktopShellWindow::Create();
        g_Globals._hwndDesktop = DesktopWindow::Create();

        if (g_Globals._lua) g_Globals._lua->onShell();

        daemon_entry(0);

#ifdef _USE_HDESK
        g_Globals._desktops.get_current_Desktop()->_hwndDesktop = g_Globals._hwndDesktop;
#endif
    }

    if (!any_desktop_running) {
        // launch the shell DDE server
        if (g_SHDOCVW_ShellDDEInit)
            (*g_SHDOCVW_ShellDDEInit)(TRUE);
    }

    if (_tcsstr(ext_options, TEXT("-?"))) {
        MessageBoxA(g_Globals._hwndDesktop,
                    "\r\n"
                    "-?        display command line options\r\n"
                    "\r\n"
                    "-desktop        start in desktop mode regardless of an already running shell\r\n"
                    "-nodesktop    disable desktop mode\r\n"
                    "-explorer        display cabinet window regardless of enabled desktop mode\r\n"
                    "\r\n"
                    "-install        replace previous shell application with WinXShell\r\n"
                    "\r\n"
                    "-noautostart    disable autostarts\r\n"
                    "-autostart    enable autostarts regardless of debug build\r\n"
                    "\r\n"
                    "-console        open debug console\r\n"
                    "\r\n"
                    "-debug        activate GDB remote debugging stub\r\n"
                    "-break        activate debugger breakpoint\r\n",
                    "WinXShell - command line options", MB_OK);
    }

    Thread *pSSOThread = NULL;

    if (startup_desktop && JCFG_TB(2, "notaskbar").ToBool() == FALSE) {
        // launch SSO thread to allow message processing independent from the explorer main thread
        pSSOThread = new SSOThread;
        pSSOThread->Start();
    }


    /**TODO launching autostart programs can be moved into a background thread. */
    if (autostart) {
        const TCHAR *argv[] = {TEXT(""), TEXT("s")};    // call startup routine in SESSION_START mode
        startup(2, argv);
    }


    if (g_Globals._hwndDesktop) {
        g_Globals._desktop_mode = true;
        bool isfirstrun = (ShellHasBeenRun() == 0);
        if (isfirstrun && g_Globals._lua) g_Globals._lua->onFirstRun();
    }

    /* UIManager Process */
    if (!startup_desktop && pUIManager) {
        Window::MessageLoop();
        return g_Globals._exitcode;
    }

    int ret = explorer_main(hInstance, lpCmdLine, nShowCmd);


    // write configuration file
    //g_Globals.write_persistent();

    if (pSSOThread) {
        pSSOThread->Stop();
        delete pSSOThread;
    }

    FileExplorerWindow::ReleaseHook();

    if (!any_desktop_running) {
        // shutdown the shell DDE server
        if (g_SHDOCVW_ShellDDEInit)
            (*g_SHDOCVW_ShellDDEInit)(FALSE);
    }

    return ret;
}

void UIProcess(HINSTANCE hInst, String cmdline) {

    HWND hwnd = CUIManager::GetUIManager();
    if (hwnd == NULL) {
        CUIManager::CreateUI(hInst, cmdline);
    } else {
        SendMessage(hwnd, WM_UICREATE, 0, (LPARAM)(cmdline.c_str()));
    }
    return;
}
