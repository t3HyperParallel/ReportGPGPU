// とりあえず画面に何か出す
// ほぼサンプル通り
// 参考：https://learn.microsoft.com/ja-jp/windows/win32/learnwin32/learn-to-program-for-windows

#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <winerror.h>

#define CHECK_NOTNULL_MESSAGE_HWND(value, customMessage)           \
    {                                                              \
        if ((value) == 0)                                          \
        {                                                          \
            DWORD atom = GetLastError();                           \
            DWORD hr = HRESULT_FROM_WIN32(atom);                   \
            WCHAR mes[128];                                        \
            wsprintf(mes, L"HRESULT %x at %s", hr, customMessage); \
            MessageBox(NULL, mes, L"h", MB_OK);                    \
            return (hr);                                           \
        }                                                          \
    }

const wchar_t CLASS_NAME[] = L"Displayer";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        {
            // いらないので
            /*
            PAINTSTRUCT ps;
            HDC hdc=BeginPaint(hwnd,&ps);
            FillRect(hdc,&ps.rcPaint,(HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd,&ps);*/
        }
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // ! ここのかっこ初期化は必須らしい
    WNDCLASSW wc={};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    CHECK_NOTNULL_MESSAGE_HWND(RegisterClassW(&wc), L"RegisterClass");
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"nan ka tek ito",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);
    CHECK_NOTNULL_MESSAGE_HWND(hwnd, L"CreateWindowEx");

    // ShowWindowについて
    //  nCmdShowについて：
    //   （中略）初めて呼び出す場合、値は、その nCmdShow パラメーターの
    //   WinMain 関数によって取得された値である必要があります。
    //  返り値について
    //   ウィンドウが以前に非表示になっていた場合、戻り値は 0 です。
    ShowWindow(hwnd, nCmdShow);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}