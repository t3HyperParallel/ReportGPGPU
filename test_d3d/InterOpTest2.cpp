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
            // DXGI_ERROR_NOT_FOUND(0x887A0002)が出力された場合、idxが範囲外
            HRESULT_exit(hr, L"EnumAdapters in GetFirstCuDevice");
        }
    }
    MesExit(L"compatible device is not found", -1);
}

// コンテキストを作成してPushする必要があるらしい
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
    std::unique_ptr<BYTE[]> m_buffer = std::make_unique<BYTE[]>(siWidth * siHeight * 4);
    {
        HRESULT_exit(
            m_frame->CopyPixels(
                NULL,
                siWidth * 4, // ストライド：1行あたりのデータ量(byte)
                siWidth * siHeight * 4,
                m_buffer.get()),
            L"CopyPixels");
    }

    CComPtr<IDXGIAdapter> m_adapter;
    CUdevice cu_device;
    GetFirstCuDevice(pDXGIFactory, &m_adapter, &cu_device);

    // よくわからない
    CUcontext cu_context;
    CreateCuContext(&cu_context, cu_device);

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
                m_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
                // pAdapterがnonnullのときはDriverTypeにHARDWAREを指定してはいけないらしい
                // どうやらpAdapter=NULLで適当に指定してくれるらしく、その補助に使うのだろうか
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
    CComPtr<ID3D11Texture2D> m_d_sampleImage;
    {
        D3D11_TEXTURE2D_DESC texDesc = {0};
        texDesc.Width = SAMPLE_X;
        texDesc.Height = SAMPLE_Y;
        texDesc.MipLevels = 1;                       // ミップマップをしないならレベルは1つ
        texDesc.ArraySize = 1;                       // よくわからないが最低は1らしい、名前的にそう
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 変わるかも
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;          // 変わるかも
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // 何かは設定しないといけないらしい
        texDesc.CPUAccessFlags = 0;                     // 変えると動かない
        texDesc.MiscFlags = 0;                          // 変えると動かない

        D3D11_SUBRESOURCE_DATA subresource = {0};
        subresource.pSysMem = m_buffer.get();
        subresource.SysMemPitch = siWidth * 4; // 要はストライド
        // subresource.SysMemSlicePitch; // 3Dテクスチャで使われる

        HRESULT_exit(
            m_D3DDevice->CreateTexture2D(
                &texDesc, &subresource, &m_d_sampleImage),
            L"CreateTexture2D");
    }

    CComPtr<ID3D11Texture2D> m_d_target;
    HRESULT_exit(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_d_target)),
        L"IDXGISwapChain::GetBuffer");

    // リソースの登録
    // SetMapFlagsはMapする前にやる必要がある
    // gi:Graphic Interoperating
    CUgraphicsResource cu_giResource_sampleImage, cu_giResource_target;
    CUresult_exit(
        cuGraphicsD3D11RegisterResource(
            &cu_giResource_sampleImage, m_d_sampleImage,
            CU_GRAPHICS_REGISTER_FLAGS_NONE),
        L"cuGraphicsD3D11RegisterResource for sample image");
    CUresult_exit(
        cuGraphicsResourceSetMapFlags(cu_giResource_sampleImage, CU_GRAPHICS_MAP_RESOURCE_FLAGS_READ_ONLY),
        L"cuGraphicsResourceSetMapFlags for sample image");
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
        cuGraphicsMapResources(1, &cu_giResource_sampleImage, NULL),
        L"cuGraphicsMapResources for sample image");
    CUresult_exit(
        cuGraphicsMapResources(1, &cu_giResource_target, NULL),
        L"cuGraphicsMapResources for target");

    // コピーしてみる
    {
        // 使うかもしれないので消さないでおく
        // CUdeviceptr cu_dpSampleImage, cu_dpTarget;

        CUarray cu_array_sampleImage, cu_array_target;
        size_t size_sample, size_target;
        // ! cuGraphicsResourceGetMappedPointerで止まる、siもtargetもダメ、リソースの作成レベルで問題？
        // ? cuGraphicsD3D11RegisterResourceについて読み直した
        // ?  "ID3D11Texture2D: individual subresources of the texture may be accessed via arrays"
        // ? つまり、デバイスポインタ経由では取得できないということ…？
        // * CUmipmappedArrayの取得に成功、で、どうやってアクセスするんだ？
        // * CUarrayを直接取得した方が便利そう
        CUresult_exit(
            cuGraphicsSubResourceGetMappedArray(
                &cu_array_sampleImage, cu_giResource_sampleImage, 0, 0),
            L"cuGraphicsResourceGetMipmappedArray for sample image");
        CUresult_exit(
            cuGraphicsSubResourceGetMappedArray(
                &cu_array_target, cu_giResource_target, 0, 0),
            L"cuGraphicsResourceGetMipmappedArray for target");
        // ! cuMemCopyAtoAは1次元Array用
        // * 2次元ArrayにはcuMemcpy2Dを使用
        {
            CUDA_MEMCPY2D cpyParams = {0};
            cpyParams.dstXInBytes; // オフセット
            cpyParams.dstY;        // オフセット
            cpyParams.dstMemoryType = CU_MEMORYTYPE_ARRAY;
            cpyParams.dstArray = cu_array_target;

            cpyParams.srcXInBytes; // オフセット
            cpyParams.srcY;        // オフセット
            cpyParams.srcMemoryType = CU_MEMORYTYPE_ARRAY;
            cpyParams.srcArray = cu_array_sampleImage;

            cpyParams.Height = SAMPLE_Y;           // コピー長
            cpyParams.WidthInBytes = SAMPLE_Y * 4; // コピー長

            CUresult_exit(
                cuMemcpy2D(&cpyParams),
                L"cuMemcpy2D");
        }
    }

    // unmap
    CUresult_exit(
        cuGraphicsUnmapResources(1, &cu_giResource_sampleImage, NULL),
        L"cuGraphicsUnmapResources for sample image");
    CUresult_exit(
        cuGraphicsUnmapResources(1, &cu_giResource_target, NULL),
        L"cuGraphicsUnmapResources for target");

    HRESULT_exit(
        m_swapChain->Present(0, 0),
        L"Present");
}