
#include <Windows.h>
#include <dwmapi.h>

#include <map>

extern int JCfg_GetDesktopBarHeightWithDPI();
extern "C" {
    void _log_(LPCTSTR txt);
}

WCHAR dwmWndClassName[] = TEXT("dwmTaskThumbnailWnd");

HWND tbhwnd = NULL;

using namespace std;
typedef map<HWND, HTHUMBNAIL> ThumbnailMap;


static int ThumbnailInited = 0;
static CRITICAL_SECTION cs;
static ThumbnailMap thumbnail_map;

static HWND _taskbar = NULL;
static HWND _toolbar = NULL;
static int currentThumbnailId = -1;

#define ID_TIMER_DESTORYTHUMBNAIL 101

void InitThumbnailWindow(HWND taskbar, HWND toolbar)
{
    if (!ThumbnailInited) {
        InitializeCriticalSection(&cs);
        ThumbnailInited = 1;
    }
    _taskbar = taskbar;
    _toolbar = toolbar;
    currentThumbnailId = -1;

}

void DestoryThumbnailWindow()
{
    if (!ThumbnailInited) return;
    EnterCriticalSection(&cs);
    for (ThumbnailMap::iterator it = thumbnail_map.begin();
        it != thumbnail_map.end(); ++it) {
        if (it->second) DwmUnregisterThumbnail(it->second);
        //ShowWindow(it->first, SW_HIDE);
        DestroyWindow(it->first);
    }
    thumbnail_map.clear();
    currentThumbnailId = -1;
    LeaveCriticalSection(&cs);
}

LRESULT CALLBACK ThumbnailProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
    TCHAR buff[255];
    _swprintf(buff, TEXT("ThumbnailProcedure: Message(%d)"), message);
    _log_(buff);
#endif
    switch (message) {
    case WM_MOUSEMOVE:
        KillTimer(_taskbar, ID_TIMER_DESTORYTHUMBNAIL);
        KillTimer(hwnd, ID_TIMER_DESTORYTHUMBNAIL);
        SetTimer(hwnd, ID_TIMER_DESTORYTHUMBNAIL, 500, NULL);
    case WM_TIMER:
        if (wParam == ID_TIMER_DESTORYTHUMBNAIL) {
            POINT pt;
            RECT rc;
            GetCursorPos(&pt);
            GetWindowRect(hwnd, &rc);
            if (pt.x < rc.left || pt.x > rc.right || pt.y < rc.top) {
                SetTimer(_taskbar, ID_TIMER_DESTORYTHUMBNAIL, 200, NULL);
            }
        }
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

HWND CreateThumbnailWindow(HINSTANCE instance, RECT rc, RECT btnrc)
{
    WNDCLASSEX wincl;
    ZeroMemory(&wincl, sizeof(WNDCLASSEX));

    // Register the window class
    wincl.hInstance = instance;
    wincl.lpszClassName = dwmWndClassName;
    wincl.lpfnWndProc = ThumbnailProcedure;
    wincl.cbSize = sizeof(WNDCLASSEX);
    //wincl.style = CS_DROPSHADOW;
    //wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    //wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);

    RegisterClassEx(&wincl);
    int h = rc.bottom - rc.top;
    int w = rc.right - rc.left;
    int scr_w = GetSystemMetrics(SM_CXSCREEN);
    int scr_h = GetSystemMetrics(SM_CYSCREEN);
    int tmp_w, tmp_h;
    float scr_v = scr_h / (scr_w + 0.0f);
    float v1 = h / (w + 0.0f);
    float v2 = w / (h + 0.0f);
    // 240*135  1/64
    if (v1 >= scr_v) {
        // h >> w
        tmp_h = (int)(scr_h / 8);
        tmp_w = (int)(tmp_h * v2);
    } else {
        tmp_w = (int)(scr_w / 8);
        tmp_h = (int)(tmp_w * v1);
    }
    //POINT p;
    //GetCursorPos(&p);

#ifdef _DEBUG
    TCHAR buff[255];
    _swprintf(buff, TEXT("ThumbnailWnd: Size(%zd) Rect:%d %d %d %d"), thumbnail_map.size(), w, h, tmp_w, tmp_h);
    _log_(buff);
#endif
    w = btnrc.left + ((btnrc.right - btnrc.left) / 2) - (int)(tmp_w / 2);
    h = scr_h - (tmp_h + 20) - JCfg_GetDesktopBarHeightWithDPI();
    HWND dwmThumbnailWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, dwmWndClassName,
        NULL, WS_POPUP, w, h, tmp_w + 20, tmp_h + 20, NULL, NULL, NULL, NULL);

    return dwmThumbnailWnd;
}

HRESULT BindThumbnailWindow(HWND hWnd, HTHUMBNAIL thumbnail) {
    HRESULT hr = S_OK;
    RECT rc;
    GetClientRect(hWnd, &rc);
    rc.left += 10;
    rc.right -= 10;
    rc.top += 10;
    rc.bottom -= 10;
    DWM_THUMBNAIL_PROPERTIES dskThumbProps;
    dskThumbProps.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;// | DWM_TNP_OPACITY;
    dskThumbProps.fVisible = TRUE;
    dskThumbProps.opacity = 255;
    dskThumbProps.rcDestination = rc;
    hr = DwmUpdateThumbnailProperties(thumbnail, &dskThumbProps);
    return hr;
}

int DrawThumbnailWindow(HINSTANCE hInstance, HWND hWndSrc,
    LPCTSTR lpClassName, LPCTSTR lpWindowName, int id)
{
    HRESULT hr = S_OK;
    HTHUMBNAIL thumbnail = NULL;
    HWND hWndThumb = NULL;
    WINDOWPLACEMENT wndpl;
    RECT rect;
    int left;
    if (currentThumbnailId == id) {
        return 0;
    }
    DestoryThumbnailWindow();
    if (!hWndSrc) {
        hWndSrc = FindWindow(lpClassName, lpWindowName);
        if (!hWndSrc) return 1;
    }
    if (!ThumbnailInited) return 0;
    EnterCriticalSection(&cs);
    GetWindowPlacement(hWndSrc, &wndpl);
    GetWindowRect(_toolbar, &rect);
    left = rect.left;
    SendMessage(_toolbar, TB_GETITEMRECT, id, (LPARAM)&rect);
    rect.left += left;
    rect.right += left;
    hWndThumb = CreateThumbnailWindow(hInstance, wndpl.rcNormalPosition, rect);
    if (!hWndThumb) return 1;
    hr = DwmRegisterThumbnail(hWndThumb, hWndSrc, &thumbnail);
    if (SUCCEEDED(hr)) {
        thumbnail_map.insert(make_pair(hWndThumb, thumbnail));
        BindThumbnailWindow(hWndThumb, thumbnail);
        ShowWindow(hWndThumb, SW_SHOWNOACTIVATE);
        currentThumbnailId = id;
    } else {
        thumbnail_map.insert(make_pair(hWndThumb, (HTHUMBNAIL)NULL));
    }
    LeaveCriticalSection(&cs);
    BOOL enabled = FALSE;
    hr = DwmIsCompositionEnabled(&enabled);
    return 0;
}

