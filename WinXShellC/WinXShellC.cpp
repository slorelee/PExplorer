#include "stdafx.h"
#include <windows.h>

int StartWinXShellProcess(int argc, TCHAR *argv[]) {
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    TCHAR newcmd[4096] = { 0 };
    TCHAR appname[MAX_PATH] = { 0 };
    LPTSTR cmdline = _tcsdup(GetCommandLine());
    LPTSTR args = NULL;
    _tcsncpy_s(appname, cmdline, MAX_PATH); /* lower appname */
    _tcslwr_s(appname, MAX_PATH);
    args = _tcsstr(appname, TEXT("winxshellc.exe"));
    if (args != NULL) {
        args = cmdline + int(args - appname);
        ZeroMemory(appname, MAX_PATH);
        _tcsncpy_s(appname, cmdline, args - cmdline);
        _tcscat_s(appname, TEXT("WinXShell.exe"));
        args += 14;
        if (args[0] == TEXT('\"')) {
            args++;
            _tcscat_s(appname, TEXT("\""));
        }
        _stprintf_s(newcmd, TEXT("%s -cmd%s"), appname, args);
    } else {
        _tprintf(TEXT("CreateProcess failed (Error = %d, cmd = %s).\n"), GetLastError(), newcmd);
        return 1;
    }

    si.cb = sizeof(si);
    if (!CreateProcess(NULL, newcmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        _tprintf(TEXT("CreateProcess failed (Error = %d, cmd = %s).\n"), GetLastError(), newcmd);
        return 1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

int CatPipeFile(TCHAR *file) {
    FILE *fp = NULL;
    errno_t err;
    int file_size = 0;
    char path[MAX_PATH] = { 0 };
    char *tmp = NULL;
    WideCharToMultiByte(CP_ACP, 0, file, -1, path, MAX_PATH, NULL, false);
    err = fopen_s(&fp, path, "r");
    if (err) return 1;

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    tmp = (char *)malloc(file_size * sizeof(char));
    memset(tmp, '\0', file_size * sizeof(char));
    fseek(fp, 0, SEEK_SET);
    fread(tmp, sizeof(char), file_size, fp);
    fclose(fp);

    printf("%s\n", tmp);
    return 0;
}

int _tmain(int argc, TCHAR *argv[])
{
    TCHAR szBuff[MAX_PATH*2] = TEXT("");
    TCHAR szTempFileName[MAX_PATH] = { 0 };
    GetTempPath(MAX_PATH, szBuff);
    _stprintf_s(szTempFileName, TEXT("%sWinXShellC.%d.log"), szBuff, GetCurrentProcessId());
    SetEnvironmentVariable(TEXT("WINXSHELL_STDOUT"), szTempFileName);
    if (StartWinXShellProcess(argc, argv) == 1) return 1;
    CatPipeFile(szTempFileName);
    DeleteFile(szTempFileName);
    return 0;
}
