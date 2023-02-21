#include "WindowTemplate.cpp"

#include <wincodec.h>
#include <memory>

#include <builtin_types.h>

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
                return; // ここ忘れてた
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
            HRESULT_exit(hr, L"EnumAdapters in GetFirstCuDevice");
        }
    }
    MesExit(L"compatible device is not found", -1);
}

inline void CreateCuContext(CUcontext *pCtx, CUdevice dev)
{
    CUresult_exit(
        cuCtxCreate(pCtx, CU_CTX_SCHED_AUTO, dev),
        L"cuCtxCreate");
    CUresult_exit(
        cuCtxPushCurrent(*pCtx),
        L"cuPushCurrent");
}

void EventHandler_WM_PAINT()
{
}

void Templated_Dispose()
{
}

// フォーマット変換をCUDA側でやってみる

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
    const size_t buffer_memPitch = siWidth * 3;
    const size_t buffer_size = buffer_memPitch * siHeight;
    std::unique_ptr<BYTE[]> m_buffer = std::make_unique<BYTE[]>(buffer_size);
    {
        HRESULT_exit(
            m_frame->CopyPixels(
                NULL,
                buffer_memPitch, // ストライド：1行あたりのデータ量(byte)
                buffer_size,
                m_buffer.get()),
            L"CopyPixels");
    }

    CComPtr<IDXGIAdapter> m_adapter;
    CUdevice cu_device;
    GetFirstCuDevice(pDXGIFactory, &m_adapter, &cu_device);

    CUcontext cu_context;
    CreateCuContext(&cu_context, cu_device);

    // 画像データを転送
    CUdeviceptr cu_dpSampleImage;
    CUresult_exit(
        cuMemAlloc(&cu_dpSampleImage, buffer_size),
        L"cuMemAlloc");
    CUresult_exit(
        cuMemcpyHtoD(cu_dpSampleImage, m_buffer.get(), buffer_size),
        L"cuMemcpyHtoD");

    // D3D11CreateDeviceAndSwapChainする
    CComPtr<IDXGISwapChain> m_swapChain;
    CComPtr<ID3D11Device> m_D3DDevice;
    D3D_FEATURE_LEVEL featureLevels;
    CComPtr<ID3D11DeviceContext> m_context;
    {
        // DXGI_SWAP_CHAIN_DESCを作成
        DXGI_SWAP_CHAIN_DESC scDesc = {0};
        scDesc.BufferDesc.Width = siWidth;
        scDesc.BufferDesc.Height = siHeight;
        // scDesc.BufferDesc.RefreshRate
        scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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
                m_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
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

    CComPtr<ID3D11Texture2D> m_d_target;
    HRESULT_exit(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_d_target)),
        L"IDXGISwapChain::GetBuffer");

    // リソースの登録
    // gi:Graphic Interoperating
    CUgraphicsResource cu_giResource_target;
    CUresult_exit(
        cuGraphicsD3D11RegisterResource(
            &cu_giResource_target, m_d_target,
            CU_GRAPHICS_REGISTER_FLAGS_NONE),
        L"cuGraphicsD3D11RegisterResource for swap chain buffer");
    CUresult_exit(
        cuGraphicsResourceSetMapFlags(cu_giResource_target, CU_GRAPHICS_MAP_RESOURCE_FLAGS_WRITE_DISCARD),
        L"cuGraphicsResourceSetMapFlags for target");

    // リソースをデバイスにmap
    CUresult_exit(
        cuGraphicsMapResources(1, &cu_giResource_target, NULL),
        L"cuGraphicsMapResources for target");
    // CUarrayを取得
    CUarray cu_array_target;
    CUresult_exit(
        cuGraphicsSubResourceGetMappedArray(
            &cu_array_target, cu_giResource_target, 0, 0),
        L"cuGraphicsResourceGetMipmappedArray for target");
    // サーフェスオブジェクト化
    CUsurfObject cu_surf_target;
    {
        CUDA_RESOURCE_DESC resDesc;
        resDesc.resType = CU_RESOURCE_TYPE_ARRAY;
        resDesc.res.array.hArray = cu_array_target;
        resDesc.flags = 0; // must be set to zero らしい

        CUresult_exit(
            cuSurfObjectCreate(&cu_surf_target, &resDesc),
            L"cuSurfObjectCreate");
    }

    // カーネルのロード
    CUmodule cu_module;
    CUresult_exit(
        cuModuleLoad(&cu_module, "lenti.ptx"),
        L"cuModuleLoad");
    CUfunction cu_f_color24bitTo32bit;
    CUresult_exit(
        cuModuleGetFunction(&cu_f_color24bitTo32bit, cu_module, "color24bitTo32bit"),
        L"cuModuleGetFunction");

    // カーネル実行
    {
        CUlaunchConfig cu_launchConfig;
        cu_launchConfig.gridDimX = 8;
        cu_launchConfig.gridDimY = 512;
        cu_launchConfig.gridDimZ = 1;
        cu_launchConfig.blockDimX = 128;
        cu_launchConfig.blockDimY = 1;
        cu_launchConfig.blockDimZ = 1;
        cu_launchConfig.sharedMemBytes = 512; // 多めにしてある
        cu_launchConfig.hStream = NULL;
        cu_launchConfig.attrs = NULL;
        cu_launchConfig.numAttrs = 0;

        unsigned int _memPitch = buffer_memPitch;

        void *kernelArgs[] = {
            &cu_surf_target,
            &cu_dpSampleImage,
            &_memPitch,
            &siWidth,
            &siHeight};

        CUresult_exit(
            cuLaunchKernelEx(
                &cu_launchConfig,
                cu_f_color24bitTo32bit,
                kernelArgs,
                NULL),
            L"cuLaunchKernel");
    }

    // ターゲットバッファのCUDA側の破棄
    CUresult_exit(
        cuSurfObjectDestroy(cu_surf_target),
        L"cuSurfObjectDestroy");
    CUresult_exit(
        cuGraphicsUnmapResources(1, &cu_giResource_target, NULL),
        L"cuGraphicsUnmapResources for target");
    // memfree

    HRESULT_exit(
        m_swapChain->Present(0, 0),
        L"Present");
}