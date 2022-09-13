#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>

#include <winerror.h>
#include <wrl.h>
#include <combaseapi.h>

#include <wincodec.h>

using Microsoft::WRL::ComPtr;

#define FILENAME_IN L"sample1.MPO"

#ifdef SECOND_IMAGE
#define FILENAME_OUT L"MPOLoadTest1_2_out.png"
#define WINDOW_TITLE "MPOLoadTest1_SECOND_IMAGE"
#else
#define FILENAME_OUT L"MPOLoadTest1_out.png"
#define WINDOW_TITLE "MPOLoadTest1"
#endif

#define MSG_BOX(message) MessageBox(NULL, message, TEXT(WINDOW_TITLE), MB_OK)


// マクロの中にreturnを入れるのは恐らく悪教邪文なので真似しないように
#define IF_FAILED_MESSAGE_RETURN(formula, quote)  \
    {                                             \
        HRESULT hr = formula;                     \
        if (FAILED(hr))                           \
        {                                         \
            makeErrorMessageBox(hr, TEXT(quote)); \
            return hr;                            \
        }                                         \
    }

void makeErrorMessageBox(HRESULT hr, LPCWSTR name)
{
    WCHAR msg[128];
    wsprintf(msg, TEXT("HRESULT %x in %s"), hr, name);
    MSG_BOX(msg);
}

// 参考文献 https://docs.microsoft.com/ja-jp/windows/win32/wic/-wic-codec-jpegmetadataencoding
// 参考文献をベースに冒頭をストリームからの取得に改変したもの

// MPOから1枚だけJPEGを切り出してみる
// そしてPNGで保存してみる

// SECOND_IMAGEが定義されていると2枚目のJPEGを切り出す

// m_ というプレフィックスを用いているが、恐らくmanagedの略

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // COMの初期化
    IF_FAILED_MESSAGE_RETURN(
        CoInitializeEx(NULL, COINIT_MULTITHREADED),
        "CoInitializedEx");

    // IWICImagingFactoryの生成
    ComPtr<IWICImagingFactory2> m_WICImagingFactory;
    IF_FAILED_MESSAGE_RETURN(
        CoCreateInstance(
            CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_WICImagingFactory)),
        "CoCreateInstance (with WICImagingFactory)");

    // ファイルのロードとデコード
    ComPtr<IWICBitmapFrameDecode> m_FrameDecode;
    { // 読み込みストリームとデコーダーの生存期間管理用
        // 読み込みのストリームの生成
        ComPtr<IWICStream> m_fsRead;
        IF_FAILED_MESSAGE_RETURN(
            m_WICImagingFactory->CreateStream(&m_fsRead),
            "CreateStream (of in)");
        // 初期化
        IF_FAILED_MESSAGE_RETURN(
            m_fsRead->InitializeFromFilename(FILENAME_IN, GENERIC_READ),
            "InitializeFromFilename (of Stream in)");

#ifdef SECOND_IMAGE
        // 2枚目の開始位置までシーク
        {
            LARGE_INTEGER dLibMove;
            dLibMove.QuadPart = 0x950E;
            IF_FAILED_MESSAGE_RETURN(
                m_fsRead->Seek(dLibMove, STREAM_SEEK_SET, NULL),
                "Seek (to 2nd JPEG Container)");
        }
#endif

        // デコーダの生成
        ComPtr<IWICBitmapDecoder> m_Decoder;
        IF_FAILED_MESSAGE_RETURN(
            m_WICImagingFactory->CreateDecoder(
                GUID_ContainerFormatJpeg, NULL,
                &m_Decoder),
            "CreateDecoder");
        // 初期化
        IF_FAILED_MESSAGE_RETURN(
            m_Decoder->Initialize(m_fsRead.Get(), WICDecodeMetadataCacheOnDemand),
            "Initialize (of Decoder)");

        // フレームの取得
        // Jpegは常にフレームは1枚のみである（参考文献によると）
        IF_FAILED_MESSAGE_RETURN(
            m_Decoder->GetFrame(0, &m_FrameDecode),
            "GetFrame");
    }

    // 書き込みのストリームの生成
    ComPtr<IWICStream> m_fsWrite;
    IF_FAILED_MESSAGE_RETURN(
        m_WICImagingFactory->CreateStream(&m_fsWrite),
        "CreateStream (of out)");
    // 初期化
    IF_FAILED_MESSAGE_RETURN(
        m_fsWrite->InitializeFromFilename(FILENAME_OUT, GENERIC_WRITE),
        "InitializeFromFilename (of out)");

    // エンコーダの生成
    ComPtr<IWICBitmapEncoder> m_Encoder;
    IF_FAILED_MESSAGE_RETURN(
        m_WICImagingFactory->CreateEncoder(
            GUID_ContainerFormatPng, NULL,
            &m_Encoder),
        "CreateEncoder");
    // 初期化
    IF_FAILED_MESSAGE_RETURN(
        m_Encoder->Initialize(m_fsWrite.Get(), WICBitmapEncoderNoCache),
        "Initialize (of Encoder)");

    // エンコーダにフレームを作ってデータをコピーしてフレームをコミット
    {
        // エンコーダにフレームを生成
        ComPtr<IWICBitmapFrameEncode> m_FrameEncode;
        IF_FAILED_MESSAGE_RETURN(
            m_Encoder->CreateNewFrame(&m_FrameEncode, NULL),
            "CreateNewFrame");
        // 初期化
        IF_FAILED_MESSAGE_RETURN(
            m_FrameEncode->Initialize(NULL),
            "Initialize (of FrameEncode)");

        // サイズをコピー
        {
            // 取得
            UINT width, height;
            IF_FAILED_MESSAGE_RETURN(
                m_FrameDecode->GetSize(&width, &height),
                "GetSize");
            // 設定
            IF_FAILED_MESSAGE_RETURN(
                m_FrameEncode->SetSize(width, height),
                "SetSize");
        }
        // 解像度情報をコピー
        {
            //! このサンプルでは不要なのでパス
            // MPOのメタデータは1枚目のJPEGコンテナにのみ格納されるので注意が必要。
        }
        // ピクセルフォーマットをコピー
        {
            // 取得
            WICPixelFormatGUID fmtGUID;
            IF_FAILED_MESSAGE_RETURN(
                m_FrameDecode->GetPixelFormat(&fmtGUID),
                "GetPixelFormat");
            // 設定
            IF_FAILED_MESSAGE_RETURN(
                m_FrameEncode->SetPixelFormat(&fmtGUID),
                "GetPixelFormat");
        }
        // ピクセルデータのコピー
        IF_FAILED_MESSAGE_RETURN(
            m_FrameEncode->WriteSource(m_FrameDecode.Get(), NULL),
            "WriteSource");

        // フレームのコミット
        IF_FAILED_MESSAGE_RETURN(
            m_FrameEncode->Commit(),
            "Commit (of FrameEncode)");
    }

    // エンコーダのコミット
    IF_FAILED_MESSAGE_RETURN(
        m_Encoder->Commit(),
        "Commit (of Encoder)");
    // ストリームのコミット
    IF_FAILED_MESSAGE_RETURN(
        m_fsWrite->Commit(STGC_DEFAULT),
        "Commit (of Stream out)");

    // 全て成功
    MSG_BOX(TEXT("全て成功"));
    return 0;
}