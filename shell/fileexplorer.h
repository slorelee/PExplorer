#ifndef _INCLUDE_FILEEXPLORER_H
#define _INCLUDE_FILEEXPLORER_H

#include <ShObjIdl.h>

class CFileDialogEventHandler :
    public IFileDialogEvents
{
public:

    //
    // IUnknown methods
    //

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CFileDialogEventHandler, IFileDialogEvents),
            { 0 }
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&m_cRef);
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    //
    // IFileDialogEvents methods
    //
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*);
    IFACEMETHODIMP OnFolderChange(IFileDialog *pDlg);
    IFACEMETHODIMP OnFileOk(IFileDialog*);
    IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnTypeChange(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }
    CFileDialogEventHandler() : m_cRef(1), m_DialogInited(FALSE) { }

protected:

    ~CFileDialogEventHandler() { }
    long m_cRef;
    BOOL m_DialogInited;
};

HRESULT CFileDialogEventHandler_CreateInstance(REFIID riid, void **ppv);

#define FILEEXPLORERWINDOWCLASSNAME TEXT("FileExplorerClass")
struct FileExplorerWindow : public Window {
    typedef Window super;
    static HWND _hFrame;
    FileExplorerWindow(HWND hwnd);
    ~FileExplorerWindow();
    static HWND Create();
    static HWND Create(HWND hwnd, String path);
    static int OpenDialog(HWND hwnd, String path);
protected:
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};


#endif //_INCLUDE_FILEEXPLORER_H