// ウィンドウ出すテンプレ

#ifndef UNICODE
#define UNICODE
#endif

#pragma once

#include <Windows.h>
#include <winerror.h>

#include <combaseapi.h>
#include <atlbase.h>

// dxgi.lib
#include <dxgi.h>
// d3d11.lib
#include <d3d11.h>

// cuda.lib
#include <cuda.h>
#include <cudaD3D11.h>

#include <process.h>

__forceinline void MesExit(LPCWSTR message, int exitCode)
{
    MessageBoxW(NULL, message, L"WindowTemplate", MB_OK);
    PostQuitMessage(exitCode);
}

__inline void GetLastError_exit(LPCWSTR errorAt)
{
    DWORD w32r = GetLastError();
    if (w32r != ERROR_SUCCESS)
    {
        HRESULT hr = HRESULT_FROM_WIN32(w32r);
        WCHAR mes[128];
        wsprintf(mes, L"Win32 System Error %x\nas HRESULT %x at %s", w32r, hr, errorAt);
        MesExit(mes, hr);
    }
}

__inline void HRESULT_exit(HRESULT hr, LPCWSTR errorAt)
{
    if (FAILED(hr))
    {
        WCHAR mes[128];
        wsprintf(mes, L"HRESULT %x at %s", hr, errorAt);
        MesExit(mes, hr);
    }
}

__inline void CUresult_exit(CUresult cr, LPCWSTR errorAt)
{
    if (cr != CUDA_SUCCESS)
    {
        WCHAR errWStr[128];
        {
            const char *errStr;
            cuGetErrorString(cr, &errStr);
            MultiByteToWideChar(CP_ACP, 0, errStr, -1, errWStr, 128);
        }
        WCHAR errWName[128];
        {
            const char *errName;
            cuGetErrorName(cr, &errName);
            MultiByteToWideChar(CP_ACP, 0, errName, -1, errWName, 128);
        }
        WCHAR mes[512];
        wsprintf(mes, L"CUresult %i=%s\nat %s\n\n%s", cr,errWName, errorAt, errWStr);
        MesExit(mes, cr);
    }
}

__forceinline void Templated_Init(HWND hwnd, IDXGIFactory *pDXGIFactory);

__forceinline void Templated_Dispose();

__forceinline void EventHandler_WM_PAINT();

LRESULT CALLBACK WindowProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        EventHandler_WM_PAINT();
        break;
    default:
        break;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

const wchar_t CLASS_NAME[] = L"CLASS_RND_36589";

// signedかunsignedか不明なことがあるためマクロで用意
#define SAMPLE_X 640
#define SAMPLE_Y 480
#define SAMPLE_X_Y SAMPLE_X, SAMPLE_Y

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProcW;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    if (RegisterClassW(&wc))
        GetLastError_exit(L"RegisterClass");
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"オマチクタ゛サイ",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, SAMPLE_X, SAMPLE_Y,
        NULL,
        NULL,
        hInstance,
        NULL);
    ShowWindow(hwnd, nCmdShow);

    // cuInit
    CUresult_exit(cuInit(0), L"cuInit");

    // ここからDXGI周り
    // CComPtrの使用にはatlsd.libを足すこと
    HRESULT_exit(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED), L"CoInitialize");
    CComPtr<IDXGIFactory> m_DXGIFactory;
    HRESULT_exit(CreateDXGIFactory(IID_PPV_ARGS(&m_DXGIFactory)), L"CreateDXGIFactory");
    // ここまでCOM周り

    Templated_Init(hwnd, m_DXGIFactory);

    SetWindowTextW(hwnd, L"Template Window Class");

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 終了処理
    Templated_Dispose();
    CoUninitialize();

    return 0;
}