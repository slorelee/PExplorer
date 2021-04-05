
#include<stdio.h>
#include<windows.h>

#define MASKLAYERWINDOW_TITLE TEXT("wxsBrightnessMaskLayerWindow")
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#define GWL_USERDATA        (-21)
#define SWITCHTOTOP_TIMER_ID 10001

HWND hBrightnessWindow = NULL;

HWND CreateMaskLayerWindow(HINSTANCE hInstance)
{
    HWND hWnd = 0;
    TCHAR lpszClassName[] = MASKLAYERWINDOW_TITLE;

    WNDCLASS wc;
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
    wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = lpszClassName;

    RegisterClass(&wc);

    hWnd = CreateWindow(lpszClassName, MASKLAYERWINDOW_TITLE,
        WS_POPUP, 0, 0, 8192, 8192,
        NULL, NULL, hInstance, NULL);

    LONG nRet = ::GetWindowLong(hWnd, GWL_EXSTYLE);
    nRet = nRet | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED;
    ::SetWindowLong(hWnd, GWL_EXSTYLE, nRet);
    //00 1A 33 4C 66 80 9A B3 CC E6 FF
    ::SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);
    ::SetWindowLong(hWnd, GWL_USERDATA, (LONG)10);
    ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    // BringWindowToTop(hWnd);

    ::ShowWindow(hWnd, SW_SHOWNORMAL);
    ::UpdateWindow(hWnd);

    return hWnd;
}

void CreateBrightnessLayer(HINSTANCE hInstance)
{
    HWND hWnd = NULL;
    hWnd = FindWindow(MASKLAYERWINDOW_TITLE, NULL);
    if (!hWnd) {
        hWnd = CreateMaskLayerWindow(hInstance);
        if (hWnd) SetTimer(hWnd, SWITCHTOTOP_TIMER_ID, 2000, NULL);
    }
    hBrightnessWindow = hWnd;
}

int GetScreenBrightness()
{
    HWND hWnd = FindWindow(MASKLAYERWINDOW_TITLE, NULL);
    if (hWnd) {
        return ::GetWindowLong(hWnd, GWL_USERDATA) * 10;
    }
    return 100;
}

int SetScreenBrightness(int brightness)
{
    HWND hWnd = FindWindow(MASKLAYERWINDOW_TITLE, NULL);
    if (!hWnd) hWnd = FindWindow(TEXT("FadeLensScrClass"), NULL);
    if (hWnd) {
        SendMessage(hWnd, WM_COMMAND, 1000 + (int)(brightness / 10), 0);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND lastTopWindow = 0;
    int bn_arr[11] = { 0xFF, 0xE6, 0xCC, 0xB3, 0x9A, 0x80, 0x66, 0x4C, 0x33, 0x1A, 0 };

#ifdef _DEBUG
    static int test_n = 2;
#endif

    switch (msg) {
    case WM_COMMAND: {
        // 20% - 100%
        if (wParam >= 1002 && wParam <= 1010) {
            ::SetLayeredWindowAttributes(hWnd, 0, bn_arr[wParam - 1000], LWA_ALPHA);
            ::SetWindowLong(hWnd, GWL_USERDATA,(LONG)(wParam - 1000));
        }
    }
    break;
    case WM_TIMER:{
        if (wParam == SWITCHTOTOP_TIMER_ID) {
            HWND nowTop = GetTopWindow(GetDesktopWindow());
            if (nowTop != lastTopWindow) {
#ifdef _DEBUG
                char buff[100];
                sprintf_s(buff, 100, "TopWindow: %d --- %d \n", nowTop, hWnd);
                OutputDebugStringA(buff);
                SendMessage(hWnd, WM_COMMAND, 1000 + test_n, 0);
                test_n++;
                if (test_n > 10) test_n = 2;
#endif // _DEBUG
                ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                //::BringWindowToTop(hWnd);
                lastTopWindow = nowTop;
            }
        }
    }

    break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
