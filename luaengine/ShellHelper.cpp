#include "precomp.h"

#include "LuaEngine.h"

TCHAR *CompletePath(TCHAR *target, TCHAR *buff);

void ShellContextMenuVerb(const TCHAR *file, TCHAR *verb)
{
    // Search target
    TCHAR tzTarget[MAX_PATH] = { 0 };
    TCHAR *target = CompletePath((TCHAR *)file, tzTarget);
    if (!target) return;
    FakeExplorer();
    //CoInitialize(NULL);
    DoFileVerb(target, verb);
}
