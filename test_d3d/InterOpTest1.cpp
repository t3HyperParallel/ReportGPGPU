
#include <Windows.h>

#include <combaseapi.h>
#include <dxgi.h>

// cudaD3D11.hはd3d11.hに依存している
#include <d3d11.h>

#include <cuda.h>
#include <cudaD3D11.h>

#include <atlbase.h>

void MesExit(LPCWSTR message)
{
    MessageBoxW(NULL, message, L"Test", MB_OK);
    exit(0);
}

// CUDAデバイスっぽければTRUEを返す
BOOL isCUDADevice(IDXGIAdapter *pDxgiAdapter)
{
    CUdevice cudaDevice;
    switch (cuD3D11GetDevice(&cudaDevice, pDxgiAdapter))
    {
    case CUDA_SUCCESS:
        return TRUE;
    case CUDA_ERROR_NO_DEVICE:
        return FALSE;
    default:
        MesExit(L"Miss cuD3D11GetDevice");
    }
}

// DXGIアダプタとそのうちCUDAデバイスのものの数を教えてくれる
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    if (FAILED(CoInitialize(NULL)))
        MesExit(L"Miss CoInitialize");
    CComPtr<IDXGIFactory> m_factory;
    if (CreateDXGIFactory(IID_PPV_ARGS(&m_factory)))
        MesExit(L"Miss CreateDXGIFactory");

    if (cuInit(0) != CUDA_SUCCESS)
        MesExit(L"Miss cuInit");

    // DXGIアダプタの数
    int validAdapterCount = 0;
    // CUDAデバイスの数
    int cuCompatibleCount = 0;
    for (UINT idx = 0;; idx++)
    {
        CComPtr<IDXGIAdapter> m_adapter;
        switch (m_factory->EnumAdapters(idx, &m_adapter))
        {
        case DXGI_ERROR_NOT_FOUND:
            WCHAR mes[128];
            wsprintfW(
                mes,
                L"found %d CUDA devices in %d DXGI Adaptors",
                cuCompatibleCount,validAdapterCount);
            MessageBoxW(NULL, mes, L"DXGIEnumTest", MB_OK);
            exit(0);
        case S_OK:
            validAdapterCount++;
            if (isCUDADevice(m_adapter))
                cuCompatibleCount++;
            break;
        case DXGI_ERROR_INVALID_CALL:
            break;
        default:
            MesExit(L"Miss EnumAdapters");
        }
    }
}