
#include <precomp.h>
#include "FolderOptions.h"
#include <tchar.h>
#include <Shlobj.h>

CFolderOptions *FolderOptions = NULL;
static CFolderOptions *FolderOptionsPtr = CFolderOptions::Init();

CFolderOptions *CFolderOptions::Init()
{
    if (FolderOptions) return FolderOptions;
    FolderOptions = new CFolderOptions();
    return FolderOptions;
}

/*
-- Opt = 
--   'ShowAll'     - Show the hidden files / folders
--   'ShowExt'     - Show the known extension
--   'ShowSuperHidden' - Show the hidden system files / folders
*/
DWORD CFolderOptions::Get(const TCHAR *key)
{
    SHELLSTATE ss = { 0 };
    DWORD mask = SSF_SHOWALLOBJECTS | SSF_SHOWEXTENSIONS | SSF_SHOWSUPERHIDDEN;
    SHGetSetSettings(&ss, mask, FALSE);
    if (_tcsicmp(key, TEXT("ShowAll")) == 0) {
        return ss.fShowAllObjects;
    } else if (_tcsicmp(key, TEXT("ShowExt")) == 0) {
        return ss.fShowExtensions;
    } else if (_tcsicmp(key, TEXT("ShowSuperHidden")) == 0) {
        return ss.fShowSuperHidden;
    }
    return 0;
}

void CFolderOptions::Set(const TCHAR *key, DWORD val)
{
    DWORD mask = 0;
    if (_tcsicmp(key, TEXT("ShowAll")) == 0) {
        mask = SSF_SHOWALLOBJECTS;
    } else if (_tcsicmp(key, TEXT("ShowExt")) == 0) {
        mask = SSF_SHOWEXTENSIONS;
    } else if (_tcsicmp(key, TEXT("ShowSuperHidden")) == 0) {
        mask = SSF_SHOWSUPERHIDDEN;
    } else {
        return;
    }
    Set(mask, val);
}

void CFolderOptions::Set(DWORD key, DWORD val)
{
    SHELLSTATE ss = { 0 };
    // SHGetSetSettings(&ss, key, FALSE);
    if (key == SSF_SHOWALLOBJECTS) {
        ss.fShowAllObjects = val;
    } else if (key == SSF_SHOWEXTENSIONS) {
        ss.fShowExtensions = val;
    } else if (key == SSF_SHOWSUPERHIDDEN) {
        ss.fShowSuperHidden = val;
    } else {
        return;
    }
    SHGetSetSettings(&ss, key, TRUE);
}

