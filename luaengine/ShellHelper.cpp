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

#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

static int FakedExplorer = 0;
int FakeExplorer()
{
    if (FakedExplorer == 1) return 0;

    DWORD dwCurrentProcessID;
    HANDLE hProcessThis;

    PROCESS_BASIC_INFORMATION pbi = { 0 };
    ULONG dwReturnLen;
    ULONG dwData = sizeof(PROCESS_BASIC_INFORMATION);
    dwCurrentProcessID = GetCurrentProcessId();

    hProcessThis = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwCurrentProcessID);
    NTSTATUS status = NtQueryInformationProcess(hProcessThis, ProcessBasicInformation, (PVOID)&pbi, dwData, &dwReturnLen);
    PPEB peb = pbi.PebBaseAddress;
    static PWSTR lpOldPath = (pbi.PebBaseAddress->ProcessParameters->ImagePathName.Buffer);

    static WCHAR buff[32] = { 0 };
    GetEnvironmentVariableW(TEXT("SystemDrive"), buff, 10);
    static WCHAR lpNewPath[] = TEXT("C:\\Windows\\explorer.exe");
    lpNewPath[0] = buff[0];
    StrCpy(buff, TEXT("explorer.exe"));

    //RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)peb->Reserved4[2] /* FastPebLock */);

    RtlInitUnicodeString(&pbi.PebBaseAddress->ProcessParameters->ImagePathName, lpNewPath);
    RtlInitUnicodeString(&pbi.PebBaseAddress->ProcessParameters->CommandLine, lpNewPath);

    LIST_ENTRY *entry_head, *current;
    entry_head = &(peb->Ldr->InMemoryOrderModuleList);
    current = entry_head->Flink;
    PLDR_DATA_TABLE_ENTRY pstEntry;
    while (current != entry_head) {
        pstEntry = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (pstEntry->DllBase == peb->Reserved3[1] /* ImageBaseAddress */) {
            RtlInitUnicodeString(&pstEntry->FullDllName, lpNewPath);
            RtlInitUnicodeString((UNICODE_STRING *)(pstEntry->Reserved4) /* BaseDllName */, buff);
            FakedExplorer = 1;
            break;
        }
        current = pstEntry->InMemoryOrderLinks.Flink;
    }

    //RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)peb->Reserved4[2] /* FastPebLock */);

    //CoInitialize(NULL);
    //extern HRESULT DoFileVerb(PCTSTR tzFile, PCTSTR verb);
    //DoFileVerb(TEXT("C:\\Windows\\System32\\taskmgr.exe"), TEXT("taskbarpin"));
    return 0;
}
