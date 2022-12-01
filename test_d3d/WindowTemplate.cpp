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
    exit(exitCode);
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
        WCHAR mes[128];
        wsprintf(mes, L"CUresult %i at %s", cr, errorAt);
        MesExit(mes, cr);
    }
}

#define CHECK_ZERO_GetLastError(expr, errorAt)                         \
    {                                                                  \
        if ((expr) == 0)                                               \
        {                                                              \
            DWORD hr = HRESULT_FROM_WIN32(GetLastError());             \
            WCHAR mes[128];                                            \
            wsprintfW(mes, L"HRESULT %x at %s", hr, errorAt);          \
            MessageBoxW(NULL, mes, L"CHECK_ZERO_GetLastError", MB_OK); \
            exit(hr);                                                  \
        }                                                              \
    }

#define CHECK_HRESULT(expr, errorAt)                               \
    {                                                              \
        HRESULT hr = (expr);                                       \
        if (FAILED(hr))                                            \
        {                                                          \
            WCHAR mes[128];                                        \
            wsprintfW(mes, L"HRESULT %x(hex) at %s", hr, errorAt); \
            MessageBoxW(NULL, mes, L"CHECK_HRESULT", MB_OK);       \
            exit(hr);                                              \
        }                                                          \
    }

// 本来ならcuGetErrorNameとcuGetErrorStringを使用すべきだが
// mbsへの変換が面倒なので割愛
#define CHECK_CUresult(expr, errorAt)                                   \
    {                                                                   \
        CUresult cr = (expr);                                           \
        if (cr != 0)                                                    \
        {                                                               \
            WCHAR mes[128];                                             \
            wsprintfW(mes, L"CUResult %i(decimal) at %s", cr, errorAt); \
            MessageBoxW(NULL, mes, L"CHECK_CUresult", MB_OK);           \
            exit(cr);                                                   \
        }                                                               \
    }

__forceinline void Templated_Init(HWND hwnd, IDXGIFactory *pDXGIFactory);

LRESULT CALLBACK WindowProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
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
    CHECK_ZERO_GetLastError(RegisterClassW(&wc), L"RegisterClass");
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
    CHECK_CUresult(cuInit(0), L"cuInit");

    // ここからDXGI周り
    // CComPtrの使用にはatlsd.libを足すこと
    CHECK_HRESULT(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED), L"CoInitialize");
    CComPtr<IDXGIFactory> m_DXGIFactory;
    CHECK_HRESULT(CreateDXGIFactory(IID_PPV_ARGS(&m_DXGIFactory)), L"CreateDXGIFactory");

    // ここまでCOM周り
    Templated_Init(hwnd, m_DXGIFactory);

    SetWindowTextW(hwnd, L"Template Window Class");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}