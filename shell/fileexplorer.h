#ifndef _INCLUDE_FILEEXPLORER_H
#define _INCLUDE_FILEEXPLORER_H

#include <ShObjIdl.h>

enum OPEN_WINDOW_MODE {
    OWM_EXPLORE = 1, /// window in explore mode
    OWM_ROOTED = 2, /// "rooted" window with special shell namespace root
    OWM_DETAILS = 4, /// view files in detail mode
    OWM_PIDL = 8,   /// path is given as PIDL, otherwise as LPCTSTR
    OWM_SEPARATE = 16 /// open separate subfolder windows
};

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
    IFACEMETHODIMP OnSelectionChange(IFileDialog *pDlg);
    IFACEMETHODIMP OnTypeChange(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }
    CFileDialogEventHandler() : m_cRef(1), m_DialogInited(FALSE) { }
    TCHAR *m_pSelectFile;
protected:

    ~CFileDialogEventHandler() { }
    long m_cRef;
    BOOL m_DialogInited;
};

HRESULT CFileDialogEventHandler_CreateInstance(REFIID riid, void **ppv, TCHAR *path);

#define FILEEXPLORERWINDOWCLASSNAME TEXT("FileExplorerClass")
struct FileExplorerWindow : public Window {
    typedef Window super;
    static HWND _hFrame;
    static HHOOK HookHandle;
    FileExplorerWindow(HWND hwnd);
    ~FileExplorerWindow();
    static void ReleaseHook();
    static UINT uOption;
    static HWND Create();
    static HWND Create(HWND hwnd, String path, UINT option);
    static int OpenDialog(HWND hwnd, String path);
protected:
    LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};


#endif //_INCLUDE_FILEEXPLORER_H