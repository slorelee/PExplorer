#pragma once

extern void CloseShellProcess();
extern void ChangeUserProfileEnv();

extern void OpenContainingFolder(LPTSTR pszCmdline);
extern void UpdateSysColor(LPTSTR pszCmdline);
extern void RegistAppPath();

extern void handle_console(FILE *log);
extern void handle_log(HANDLE log);
extern int daemon_entry(int standalone);
extern HWND create_daemonwindow();
extern void update_property_handler();

extern int embedding_entry();

#define WINXSHELL_SHELLWINDOW TEXT("WinXShell_ShellWindow")

extern BOOL hasMSExplorer();
extern BOOL isWinXShellAsShell();
extern String getShellTheme();
extern void wxsOpen(LPTSTR cmd);
