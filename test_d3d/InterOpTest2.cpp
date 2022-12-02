#include "WindowTemplate.cpp"

#include <wincodec.h>
#include <memory>

LPCWSTR SAMPLE_FILENAME = L"sample1.MPO";

inline void LoadSampleImage(IWICBitmapFrameDecode **ppFrame)
{
    CComPtr<IWICImagingFactory2> m_factory;
    HRESULT_exit(
        CoCreateInstance(
            CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_factory)),
        L"CoCreateInstance IWICImagingFactory");
    CComPtr<IWICStream> m_fs;
    HRESULT_exit(
        m_factory->CreateStream(&m_fs),
        L"CreateStream");
    HRESULT_exit(
        m_fs->InitializeFromFilename(SAMPLE_FILENAME, GENERIC_READ),
        L"InitializeFromFilename");
    CComPtr<IWICBitmapDecoder> m_decoder;
    HRESULT_exit(
        m_factory->CreateDecoder(
            GUID_ContainerFormatJpeg, NULL,
            &m_decoder),
        L"CreateDecoder");
    HRESULT_exit(
        m_decoder->Initialize(m_fs, WICDecodeMetadataCacheOnDemand),
        L"m_decoder::Initialize");

    HRESULT_exit(
        m_decoder->GetFrame(0, ppFrame),
        L"GetFrame");
}

inline void GetFirstCuDevice(IDXGIFactory *pFactory, IDXGIAdapter **ppAdapter, CUdevice *pDevice)
{
    for (UINT idx = 0;; idx++)
    {
        CComPtr<IDXGIAdapter> m_tmpAdapter;
        HRESULT hr = pFactory->EnumAdapters(idx, &m_tmpAdapter);
        switch (hr)
        {
        case S_OK:
        {
            CUresult cr = cuD3D11GetDevice(pDevice, m_tmpAdapter);
            switch (cr)
            {
            case CUDA_SUCCESS:
                *ppAdapter = m_tmpAdapter.Detach();
            case CUDA_ERROR_NO_DEVICE:
                break;
            default:
                CUresult_exit(cr, L"cuD3D11GetDevice");
            }
            break;
        }
        case DXGI_ERROR_INVALID_CALL:
            break;
        default:
            HRESULT_exit(hr, L"GetFirstCuDevice");
        }
    }
    MesExit(L"compatible device is not found", -1);
}

// ほぼ参考資料通り
// https://learn.microsoft.com/ja-jp/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-how-to

void Templated_Init(HWND hwnd, IDXGIFactory *pDXGIFactory)
{

    // 画像データを取得する
    CComPtr<IWICBitmapFrameDecode> m_frame;
    LoadSampleImage(&m_frame);
    UINT siWidth, siHeight; // si: Sample Image
    {
        HRESULT_exit(
            m_frame->GetSize(&siWidth, &siHeight),
            L"GetSize");
    }
    std::unique_ptr<BYTE[]> m_buffer = std::make_unique<BYTE[]>(siWidth * siHeight*4);
    {
        HRESULT_exit(
            m_frame->CopyPixels(
                NULL,
                siWidth * 4, // ストライド：1行あたりのデータ量(byte)
                siWidth * siHeight *4,
                m_buffer.get()),
            L"CopyPixels");
    }

    CComPtr<IDXGIAdapter> m_adapter;
    CUdevice cudaDevice;
    GetFirstCuDevice(pDXGIFactory, &m_adapter, &cudaDevice);

    // D3D11CreateDeviceAndSwapChainする
    CComPtr<IDXGISwapChain> m_swapChain;
    CComPtr<ID3D11Device> m_D3DDevice;
    D3D_FEATURE_LEVEL featureLevels;
    CComPtr<ID3D11DeviceContext> m_context;
    {
        // DXGI_SWAP_CHAIN_DESCを作成
        DXGI_SWAP_CHAIN_DESC scDesc = {0};
        scDesc.BufferDesc.Width = SAMPLE_X;
        scDesc.BufferDesc.Height = SAMPLE_Y;
        // scDesc.BufferDesc.RefreshRate
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        // scDesc.BufferDesc.ScanlineOrdering
        // scDesc.BufferDesc.Scaling
        scDesc.SampleDesc.Count = 1;
        scDesc.SampleDesc.Quality = 0;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.BufferCount = 1;
        scDesc.OutputWindow = hwnd;
        scDesc.Windowed = TRUE;
        // scDesc.SwapEffect
        // scDesc.Flags

        HRESULT_exit(
            D3D11CreateDeviceAndSwapChain(
                m_adapter,
                D3D_DRIVER_TYPE_HARDWARE, NULL,
                D3D11_CREATE_DEVICE_DEBUG,
                NULL, 0,
                D3D11_SDK_VERSION,
                &scDesc,
                &m_swapChain,
                &m_D3DDevice,
                &featureLevels,
                &m_context),
            L"D3D11CreateDeviceAndSwapChain");
    }

    // CreateTexture2Dする
    CComPtr<ID3D11Texture2D> m_devMem;
    {
        D3D11_TEXTURE2D_DESC texDesc = {0};
        texDesc.Width = SAMPLE_X;
        texDesc.Height = SAMPLE_Y;
        texDesc.MipLevels = texDesc.ArraySize = 0;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 変わるかも
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;          // 変わるかも
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // わからん
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA subresource = {0};
        subresource.pSysMem = m_buffer.get();
        subresource.SysMemPitch = siWidth * 4; // 要はストライド
        // subresource.SysMemSlicePitch; // 3Dテクスチャで使われる

        HRESULT_exit(
            m_D3DDevice->CreateTexture2D(
                &texDesc, &subresource, &m_devMem),
            L"CreateTexture2D");
    }

    // スワップチェーンのバッファを取得
    CUgraphicsResource cudaSampleImage;
    {
        CUresult_exit(
            cuGraphicsD3D11RegisterResource(
                &cudaSampleImage, m_devMem, CU_GRAPHICS_REGISTER_FLAGS_NONE),
            L"cuGraphicsD3D11RegisterResource for source");
    }

    // 
}