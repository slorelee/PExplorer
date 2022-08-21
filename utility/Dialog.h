#pragma once

#include<Windows.h>

class CDialog {
public:
    static CDialog *Init();
    CDialog() {};
    ~CDialog() {};
    DWORD CDialog::OpenSaveFile(int mode, DWORD options, const TCHAR *title, const TCHAR *filters, const TCHAR *dir);
    DWORD OpenFile(const TCHAR *title, const TCHAR *filters = NULL, const TCHAR *dir = NULL);
    DWORD SaveFile(const TCHAR *title, const TCHAR *filters = NULL, const TCHAR *dir = NULL);
    DWORD BrowseFolder(const TCHAR *title, int csidl = -1);

    TCHAR SelectedFileName[MAX_PATH];
    TCHAR SelectedFolderName[MAX_PATH];
};

extern CDialog *Dialog;
