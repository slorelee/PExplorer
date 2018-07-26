#pragma once

extern void CloseShellProcess();
extern void ChangeUserProfileEnv();

extern void OpenContainingFolder(LPTSTR pszCmdline);
extern void UpdateSysColor(LPTSTR pszCmdline);

extern void handle_console(FILE *log);
extern int daemon_entry();
extern HWND create_daemonwindow();
extern void update_property_handler();

extern int embedding_entry();
