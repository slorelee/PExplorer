#pragma once

#include<Windows.h>

class CFolderOptions {
public:
    static CFolderOptions *Init();
    CFolderOptions() {};
    ~CFolderOptions() {};
    DWORD Get(const TCHAR *key);
    void Set(const TCHAR *key, DWORD val);
    void Set(DWORD key, DWORD val);
};

extern CFolderOptions *FolderOptions;
