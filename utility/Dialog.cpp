
#include <precomp.h>
#include "Dialog.h"
#include <tchar.h>
#include <Shlobj.h>

#define MAX_FILTER_NUM 10
extern vector<string_t> split(string_t str, string_t pattern);

CDialog *Dialog = NULL;
static CDialog *DialogPtr = CDialog::Init();

CDialog *CDialog::Init()
{
    if (Dialog) return Dialog;
    Dialog = new CDialog();
    return Dialog;
}

DWORD CDialog::OpenFile(const TCHAR *title, const TCHAR *filters, const TCHAR *dir)
{
    HRESULT hr;
    IFileOpenDialog *pDlg = NULL;
    COMDLG_FILTERSPEC aFileTypes[MAX_FILTER_NUM] = {
        { L"All Files", L"*.*" }
    };
    // Create the file-open dialog COM object.
    hr = CoCreateInstance(CLSID_FileOpenDialog,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pDlg));

    if (FAILED(hr)) return 0;

    // SelectOption
    TCHAR *pSelectFile = NULL;


    if (title) pDlg->SetTitle(title);
    pDlg->SetOptions(FOS_NOVALIDATE | FOS_FILEMUSTEXIST | FOS_ALLNONSTORAGEITEMS | FOS_ALLOWMULTISELECT | FOS_NODEREFERENCELINKS);

    if (filters) {
        int n = 0;
        vector<string_t> arr = split(filters, _T("|"));
        for (int i = 0; i < arr.size(); i++) {
            aFileTypes[n].pszName = arr[i].c_str();
            aFileTypes[n].pszSpec = arr[++i].c_str();
            n++;
            if (n >= MAX_FILTER_NUM) break;
        }
        pDlg->SetFileTypes(n, aFileTypes);
    }

    if (dir) {
        IShellItem *psi = NULL;
        hr = SHCreateItemFromParsingName(dir, NULL, IID_PPV_ARGS(&psi));
        if (SUCCEEDED(hr)) {
            pDlg->SetFolder(psi);
            psi->Release();
        }
    }

    SelectedFileName[0] = '\0';

    // Show the dialog.
    hr = pDlg->Show(NULL);
    if (FAILED(hr)) {
        pDlg->Release();
        return S_FALSE;
    }

    IShellItem *pItem = NULL;
    hr = pDlg->GetResult(&pItem);
    if (FAILED(hr)) {
        pDlg->Release();
        return S_FALSE;
    }

    LPOLESTR pwsz = NULL;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);
    pItem->Release();
    pDlg->Release();

    if (SUCCEEDED(hr)) {
        TCHAR path[MAX_PATH] = { 0 };
        _tcscpy(SelectedFileName, pwsz);
        CoTaskMemFree(pwsz);
    }

    return 0;
}

DWORD CDialog::BrowseFolder(const TCHAR * title)
{
    BROWSEINFO bi = { 0 };

    bi.lpszTitle = title;
    bi.ulFlags = BIF_NEWDIALOGSTYLE;
    SelectedFolderName[0] = '\0';

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL)
    {
        TCHAR tszPath[MAX_PATH] = _T("\0");
        SHGetPathFromIDList(pidl, SelectedFolderName);
        CoTaskMemFree(pidl);
        return 0;
    }
    return 1;
}
