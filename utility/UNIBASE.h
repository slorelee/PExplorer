
#include <TChar.h>
#include <String.h>
#include <Windows.h>
#include <ShLwApi.h>

#define _WINSTR

//#ifndef WINCE
#define _SHLSTR
//#endif

// Type
typedef int              INT, *PINT;

// Function
#define StrCopy         lstrcpy
#define StrCat          lstrcat
#define StrToInt        StrToIntImpl
#define StrToStr(w, t, n)        StrCpyN(w, t, n)

#ifndef UAPI
#define UAPI __inline
#endif

UAPI INT StrToIntImpl(PCTSTR ptzStr)
{
#ifdef _SHLSTR
    INT i = 0;
    StrToIntEx(ptzStr, STIF_SUPPORT_HEX, &i);
    return i;
#else
    return _wtoi(pwzStr);
#endif
}

UAPI BOOL DirExist(PCTSTR ptzPath)
{
    WIN32_FILE_ATTRIBUTE_DATA fa;
    return GetFileAttributesEx(ptzPath, GetFileExInfoStandard, &fa) && (fa.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

UAPI BOOL DirCreate(PTSTR ptzDir)
{
    for (PTSTR p = ptzDir; p = StrChr(p, TEXT('\\')); *p++ = TEXT('\\')) {
        *p = 0;
        if (!DirExist(ptzDir)) {
            CreateDirectory(ptzDir, NULL);
        }
    }
    return TRUE;
}

UAPI BOOL DirDelete(PCTSTR ptzDir)
{
    return RemoveDirectory(ptzDir);
}

UAPI PTSTR DirMakePath(PTSTR ptzDir, PCTSTR ptzSub)
{
    PTSTR p = StrChr(ptzDir, 0);
    if (p[-1] != TEXT('\\')) {
        *p++ = TEXT('\\');
    }
    StrCopy(p, ptzSub);
    return ptzDir;
}

UAPI PTSTR DirSplitPath(PTSTR ptzPath)
{
    PTSTR p = StrRChr(ptzPath, NULL, TEXT('\\'));
    if (p) {
        *p = 0;
        return p + 1;
    }

    return ptzPath;
}
