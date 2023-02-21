#include "WindowTemplate.cpp"

#define MF_INIT_GUIDS
#include <mfapi.h>
#include <mfidl.h>

// Media Foundationで再生…？

const LPCWSTR sampleFile = L"../test_movie/DSCF3039.AVI"; // 後で用意する

void MakeSourceTopoNode(IMFTopologyNode **ppNode)
{
    // > ソース リゾルバーは URL またはバイト ストリームを受け取って、そのコンテンツに対応する適切なメディア ソースを作成します。
    // （「ソースリゾルバーの使用」より）
    MF_OBJECT_TYPE objType;
    CComPtr<IMFMediaSource> m_source;
    {
        CComPtr<IMFSourceResolver> m_resolver;
        HRESULT_exit(
            MFCreateSourceResolver(&m_resolver),
            L"MFCreateSourceResolver");
        // なぜ一回IUnknownを経由する必要があるのかは不明
        CComPtr<IUnknown> m_source_IUnknown;
        HRESULT_exit(
            m_resolver->CreateObjectFromURL(
                sampleFile, MF_RESOLUTION_MEDIASOURCE, NULL,
                &objType, &m_source_IUnknown),
            L"IMFSourceResolver::CreateObjectFromURL");
        HRESULT_exit(
            m_source_IUnknown->QueryInterface(IID_PPV_ARGS(&m_source)),
            L"IUnknown::QueryInterFace for IMFMediaSource");
    }
    // ここまでが「ソースリゾルバーの使用」に相当

    CComPtr<IMFPresentationDescriptor> m_presentationDesc;
    HRESULT_exit(
        m_source->CreatePresentationDescriptor(&m_presentationDesc),
        L"IMFMediaSource::CreatePresentationDescriptor");

    // ビデオストリームを1個取得してノードを作成
    DWORD streamDescriptorCount;
    HRESULT_exit(
        m_presentationDesc->GetStreamDescriptorCount(&streamDescriptorCount),
        L"IMFPresentationDescriptor::GetStreamDescriptorCount");
    for (DWORD index = 0; index < streamDescriptorCount; index++)
    {
        // 規定で選択されているストリームを取得
        BOOL isSelected;
        CComPtr<IMFStreamDescriptor> m_streamDesc;
        HRESULT_exit(
            m_presentationDesc->GetStreamDescriptorByIndex(
                index, &isSelected, &m_streamDesc),
            L"GetStreamDescriptorByIndex");
        if (!isSelected)
            continue;
        // メディアタイプハンドラを取得、チェック、Video以外はDeselectStream
        CComPtr<IMFMediaTypeHandler> m_mediaTypeHandler;
        HRESULT_exit(
            m_streamDesc->GetMediaTypeHandler(&m_mediaTypeHandler),
            L"GetMediaTypeHandler");
        GUID majorType;
        HRESULT_exit(
            m_mediaTypeHandler->GetMajorType(&majorType),
            L"GetMajorType");
        if (majorType != MFMediaType_Video)
        {
            HRESULT_exit(
                m_presentationDesc->DeselectStream(index),
                L"DeselectNode");
            continue;
        };
        // ノードを作成、ソースを登録
        CComPtr<IMFTopologyNode> m_node;
        HRESULT_exit(
            MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &m_node),
            L"MFCreateTopology for source");
        HRESULT_exit(
            m_node->SetUnknown(MF_TOPONODE_SOURCE, m_source),
            L"SetUnknown as media source");
        HRESULT_exit(
            m_node->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, m_presentationDesc),
            L"SetUnknown as presentation desc");
        HRESULT_exit(
            m_node->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, m_streamDesc),
            L"SetUnknown as stream desc");
        (*ppNode) = m_node.Detach();
        return;
    }

stream_was_not_found:
    MesExit(L"No stream desc was found", -1);
}

void FindDecoder(IMFActivate **ppActivate, IMFTransform **ppDecoder)
{
    IMFActivate **apDecoderActivates;
    UINT32 apDecoderActivates_length;
    HRESULT_exit(
        MFTEnumEx(
            MFT_CATEGORY_VIDEO_DECODER,
            MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_FIELDOFUSE,
            NULL, NULL,
            &apDecoderActivates, &apDecoderActivates_length),
        L"MFTEnumEx");
    for (UINT32 index = 0; index < apDecoderActivates_length; index++)
    {
        CComPtr<IMFTransform> m_decoder;
        HRESULT_exit(
            apDecoderActivates[index]->ActivateObject(IID_PPV_ARGS(&m_decoder)),
            L"ActivateObject in decoder func");
        CComPtr<IMFAttributes> m_attr;
        HRESULT_exit(
            m_decoder->GetAttributes(&m_attr),
            L"GetAttributes in decoder func");
        UINT32 isD3D11Aware;
        HRESULT_exit(
            m_attr->GetUINT32(MF_SA_D3D11_AWARE, &isD3D11Aware),
            L"GetUINT32 in decoder func");
        if (isD3D11Aware)
        {
            *ppActivate = apDecoderActivates[index];
            *ppDecoder = m_decoder.Detach();
            break;
        }
        else // 開放処理
        {
            HRESULT_exit(
                apDecoderActivates[index]->ShutdownObject(),
                L"ShutdownObject in decoder func");
        }
    }
    // 本来であれば例外でダウンした場合もこれを行う必要があるが、略している
    CoTaskMemFree(apDecoderActivates);
}

void EventHandler_WM_PAINT() {}

// 再生終了時にシャットダウンしないといけない

CComPtr<IMFActivate> m_decoder_act;
CComPtr<IMFMediaSession> m_session;

void Templated_Dispose()
{
    HRESULT_exit(
        m_decoder_act->ShutdownObject(), L"ShutdownObject of decoder activate");
    HRESULT_exit(
        m_session->Close(), L"Close of media session");
    for (BOOL isSessionClose = false; !isSessionClose;)
    {
        CComPtr<IMFMediaEvent> m_event;
        HRESULT_exit(
            m_session->GetEvent(0, &m_event), L"GetEvent");
        MediaEventType eventType;
        HRESULT_exit(
            m_event->GetType(&eventType), L"GetType");
        isSessionClose = (eventType == MESessionClosed);
    }
    HRESULT_exit(
        m_session->Shutdown(), L"Shutdown for session");
    HRESULT_exit(MFShutdown(), L"MFShutdown");
}

void Templated_Init(HWND hwnd, IDXGIFactory *pDXGIFactory)
{
    HRESULT_exit(
        MFStartup(MF_VERSION),
        L"MFStartup");

    // 映像ソースのノードを作成
    CComPtr<IMFTopologyNode> m_sourceNode;
    MakeSourceTopoNode(&m_sourceNode);

    // デコーダーのMFTを取得
    CComPtr<IMFTransform> m_decoder;
    FindDecoder(&m_decoder_act, &m_decoder);

    // デコーダーのノードを作成
    CComPtr<IMFTopologyNode> m_decoderNode;
    HRESULT_exit(
        MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &m_decoderNode),
        L"MFCreateTopologyNode for decoder");
    HRESULT_exit(
        m_decoderNode->SetObject(m_decoder),
        L"SetObject for decoder node");

    // シンクを作成
    CComPtr<IMFActivate> m_sink_act;
    HRESULT_exit(
        MFCreateVideoRendererActivate(hwnd, &m_sink_act),
        L"MFCreateVideoRendererActivate");
    CComPtr<IMFTopologyNode> m_sinkNode;
    HRESULT_exit(
        MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &m_sinkNode),
        L"MFCreateTopologyNode for sink");
    HRESULT_exit(
        m_sinkNode->SetObject(m_sink_act),
        L"SetObject of sink");

    // トポロジーを作成
    CComPtr<IMFTopology> m_topology;
    HRESULT_exit(
        MFCreateTopology(&m_topology),
        L"MFCreateTopology");

    HRESULT_exit(
        m_topology->AddNode(m_sourceNode),
        L"AddNode for source node");
    HRESULT_exit(
        m_topology->AddNode(m_decoderNode),
        L"AddNode for decoder node");
    HRESULT_exit(
        m_topology->AddNode(m_sinkNode),
        L"AddNode for sink node");

    // ノードを連結
    HRESULT_exit(
        m_sourceNode->ConnectOutput(0, m_decoderNode, 0),
        L"ConnectOutput of source to decoder");
    HRESULT_exit(
        m_decoderNode->ConnectOutput(0, m_sinkNode, 0),
        L"ConnectOutput of decoder to sink");

    // セッションを作成
    HRESULT_exit(
        MFCreateMediaSession(NULL, &m_session),
        L"MFCreateMediaSession");
    HRESULT_exit(
        m_session->SetTopology(MFSESSION_SETTOPOLOGY_NORESOLUTION, m_topology),
        L"SetTopology");

    // 再生開始
    PROPVARIANT start;
    PropVariantInit(&start);
    HRESULT_exit(
        m_session->Start(NULL, &start),
        L"Start session");
    PropVariantClear(&start);
}