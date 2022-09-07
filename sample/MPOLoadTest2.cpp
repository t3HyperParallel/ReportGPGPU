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
#define FILENAME_OUT_COUNTED L"MPOLoadTest2_out_%d.png"

#define MSG_BOX(message) MessageBox(NULL, message, TEXT("MPOLoadTest2"), MB_OK)

void makeErrorMessageBox(HRESULT hr, LPCWSTR name)
{
    WCHAR msg[128];
    wsprintf(msg, TEXT("HRESULT %x in %s"), hr, name);
    MSG_BOX(msg);
}

void makeErrorMessageBox(HRESULT hr, LPCWSTR name, int count)
{
    WCHAR msg[128];
    wsprintf(msg, TEXT("HRESULT %x in %s on loop-idx %d"), hr, name, count);
    MSG_BOX(msg);
}

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

#define IF_FAILED_MESSAGE_RETURN_INDEXED(formula, quote, index) \
    {                                                           \
        HRESULT hr = formula;                                   \
        if (FAILED(hr))                                         \
        {                                                       \
            makeErrorMessageBox(hr, TEXT(quote), index);        \
            return hr;                                          \
        }                                                       \
    }

// ここからが本題
// MPOファイルには複数のjpegファイルが埋め込まれている。
// ストリームに何度も読み込みを掛ける方法で達成可能なのか検証
//? 1回読み込んだ後ストリームを操作しない->2枚目以降は取得できない

// 1のロードとデコード周りに手を加える
// 引数は使わないのでオミット

int wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // COMの初期化
    IF_FAILED_MESSAGE_RETURN(
        CoInitializeEx(NULL, COINIT_MULTITHREADED),
        "CoInitializedEx");

    // IWICImagingFactoryの生成
    ComPtr<IWICImagingFactory2> m_WICImagingFactory;
    IF_FAILED_MESSAGE_RETURN(
        CoCreateInstance(
            CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_WICImagingFactory)),
        "CoCreateInstance (with WICImagingFactory)");

    // 読み込みストリームの取得
    ComPtr<IWICStream> m_fsRead;
    IF_FAILED_MESSAGE_RETURN(
        m_WICImagingFactory->CreateStream(&m_fsRead),
        "CreateStream (of in)");
    // 初期化
    IF_FAILED_MESSAGE_RETURN(
        m_fsRead->InitializeFromFilename(FILENAME_IN, GENERIC_READ),
        "InitializeFromFileName (of Stream in)");

    // エラー出るまでストリームから取り出す
    // 不測の事態を避ける為に上限はつけておく
    for (size_t index = 0; index < 4; index++)
    {
        // ストリームからの読み取り
        ComPtr<IWICBitmapFrameDecode> m_FrameDecode;
        {
            // デコーダーの取得
            ComPtr<IWICBitmapDecoder> m_Decoder;
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_WICImagingFactory->CreateDecoder(
                    GUID_ContainerFormatJpeg, NULL,
                    &m_Decoder),
                "CreateDecoder", index);
            // 初期化、失敗したならJpeg検出失敗
            {
                HRESULT hr = m_Decoder->Initialize(
                    m_fsRead.Get(),
                    WICDecodeMetadataCacheOnDemand);
                if (FAILED(hr))
                {
                    if (index = 0)
                    {
                        MSG_BOX(TEXT("ストリームから1枚もJpegが検出されなかった"));
                        return hr;
                    }
                    else
                    {
                        MSG_BOX(TEXT("ストリームからJpegが検出されなくなった為停止"));
                        return 0;
                    }
                }
            }

            // フレームの取得
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_Decoder->GetFrame(0, &m_FrameDecode),
                "GetFrame", index);
        }

        // 書き込みストリームの生成
        ComPtr<IWICStream> m_fsWrite;
        IF_FAILED_MESSAGE_RETURN_INDEXED(
            m_WICImagingFactory->CreateStream(&m_fsWrite),
            "CreateStream", index);
        // 名前を生成して初期化
        {
            WCHAR filename[256];
            wsprintf(filename, FILENAME_OUT_COUNTED, index);
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_fsWrite->InitializeFromFilename(filename, GENERIC_WRITE),
                "InitializeFromFilename (of Stream)", index);
        }

        // エンコード操作
        {
            // エンコーダを生成
            ComPtr<IWICBitmapEncoder> m_Encoder;
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_WICImagingFactory->CreateEncoder(
                    GUID_ContainerFormatPng, NULL,
                    &m_Encoder),
                "CreateEncoder", index);
            // 初期化
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_Encoder->Initialize(m_fsWrite.Get(), WICBitmapEncoderNoCache),
                "Initialize (of Encoder)", index);

            // エンコーダにフレームを作ってデータをコピーしてフレームをコミット
            {
                //フレームを生成
                ComPtr<IWICBitmapFrameEncode> m_FrameEncode;
                IF_FAILED_MESSAGE_RETURN_INDEXED(
                    m_Encoder->CreateNewFrame(&m_FrameEncode, NULL),
                    "CreateNewFrame", index);
                // 初期化
                IF_FAILED_MESSAGE_RETURN_INDEXED(
                    m_FrameEncode->Initialize(NULL),
                    "Initialize (of FrameEncode)", index);

                // サイズをコピー
                {
                    // 取得
                    UINT width, height;
                    IF_FAILED_MESSAGE_RETURN_INDEXED(
                        m_FrameDecode->GetSize(&width, &height),
                        "GetSize", index);
                    // 設定
                    IF_FAILED_MESSAGE_RETURN_INDEXED(
                        m_FrameEncode->SetSize(width, height),
                        "SetSize", index);
                }
                // ピクセルフォーマットをコピー
                {
                    // 取得
                    WICPixelFormatGUID fmtGUID;
                    IF_FAILED_MESSAGE_RETURN_INDEXED(
                        m_FrameDecode->GetPixelFormat(&fmtGUID),
                        "GetPixelFormat", index);
                    // 設定
                    IF_FAILED_MESSAGE_RETURN_INDEXED(
                        m_FrameEncode->SetPixelFormat(&fmtGUID),
                        "SetPixelFormat", index);
                }
                // ピクセルデータのコピー
                IF_FAILED_MESSAGE_RETURN_INDEXED(
                    m_FrameEncode->WriteSource(m_FrameDecode.Get(), NULL),
                    "WriteSource", index);

                // フレームをコミット
                IF_FAILED_MESSAGE_RETURN_INDEXED(
                    m_FrameEncode->Commit(),
                    "Commit (of FrameEncode)", index)
            }

            // エンコーダのコミット
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_Encoder->Commit(),
                "Commit (of Encode)", index);
            // ストリームをコミット
            IF_FAILED_MESSAGE_RETURN_INDEXED(
                m_fsWrite->Commit(STGC_DEFAULT),
                "Commit (of Stream out)", index);
        }
    }
    // ループ終了
    MSG_BOX(TEXT("検出最大回数に到達"));
    return 0;
}