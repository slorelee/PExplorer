
#include <io.h>        // for dup2()
#include <fcntl.h>    // for _O_RDONLY
#include <Windows.h>
#include <tchar.h>
#include "../utility/utility.h"

static FILE *console_log = NULL;

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

#ifdef _DEBUG
    OutputDebugString(msg);
#endif
}

void _logA_(LPCSTR txt)
{
    FmtStringA msg("%s\n", txt);

    if (console_log)
        fputs(msg.c_str(), console_log);

#ifdef _DEBUG
    OutputDebugStringA(msg.c_str());
#endif
}


