
#include <io.h>        // for dup2()
#include <fcntl.h>    // for _O_RDONLY
#include <Windows.h>
#include <tchar.h>
#include "../utility/utility.h"

static HANDLE file_log = NULL;

static FILE *console_log = NULL;


void handle_log(HANDLE log)
{
    file_log = log;
}

static void write_log(LPCWSTR msg)
{
    DWORD dwWrite = 0;
    if (file_log) {
        WriteFile(file_log, msg, _tcslen(msg) * sizeof(TCHAR), &dwWrite, NULL);
    }
}

static void write_logA(LPCSTR msg)
{
    DWORD dwWrite = 0;
    if (file_log) {
        WriteFile(file_log, msg, strlen(msg), &dwWrite, NULL);
    }
}

void handle_console(FILE *log)
{
    AllocConsole();

    _dup2(_open_osfhandle((intptr_t)GetStdHandle(STD_INPUT_HANDLE), _O_RDONLY), 0);
    _dup2(_open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), 0), 1);
    _dup2(_open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), 0), 2);

    log = _tfdopen(1, TEXT("w"));
    setvbuf(log, 0, _IONBF, 0);
    console_log = log;
}


void _log_(LPCTSTR txt)
{
    FmtString msg(TEXT("%s\n"), txt);

    if (console_log)
        _fputts(msg, console_log);

    if (file_log)
        write_log(msg);

#ifdef _DEBUG
    OutputDebugString(msg);
#endif
}

#define ENDMARK_NEWLINE 0
#define ENDMARK_NONE 1
#define ENDMARK_TAB 2
#define ENDMARK_SPACE 3

void _logA_(LPCSTR txt, char endmark = '\n')
{
    FmtStringA msg("%s\n", txt);

    if (endmark == '\0') {
        msg = txt;
    } else {
        msg = FmtStringA("%s%c", txt, endmark);
    }

    if (console_log)
        fputs(msg.c_str(), console_log);

    if (file_log)
        write_logA(msg.c_str());

#ifdef _DEBUG
    OutputDebugStringA(msg.c_str());
#endif
}

extern std::string w2s(const std::wstring& wstr);
void _logU2A_(LPCWSTR txt)
{
    wstring wstr = txt;
    FmtStringA msg("%s\n", w2s(wstr).c_str());
    if (console_log)
        fputs(msg.c_str(), console_log);

    if (file_log)
        write_logA(msg.c_str());

#ifdef _DEBUG
    OutputDebugStringA(msg.c_str());
#endif
}


