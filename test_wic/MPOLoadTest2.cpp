#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>

#include <winerror.h>
#include <wrl.h>
#include <combaseapi.h>
#include <Shlwapi.h>

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

// ? ストリームに何度も読み込みを掛ける方法で達成可能なのか検証
// * 1回読み込んだ後のストリームでは2枚目以降は取得できない
// ! なんかReadに失敗してる

// ? 2枚目の開始位置までシークしてあるストリームを渡すとどうなる？
//  （サンプルの1枚目の終了位置は0x1114）
//   (サンプルMPOの2枚目の開始位置は0x950E)

// ? ストリームのクローンをとったらどうなる？
// ! そもそも1回目からクローンがサポートされてない

// * フレーム数確認したらちゃんと1枚だった

// ? SHCreateStreamOnFileExを使用する
// ! 関係なさそう
// ! Cloneも失敗した

// ? 待つとどうなる？
// ! 5秒Sleepしても駄目だった

// 1のロードとデコード周りに手を加える
// 引数は使わないのでオミット

int wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // COMの初期化
    IF_FAILED_MESSAGE_RETURN(
        CoInitializeEx(NULL, COINIT_MULTITHREADED),
        "CoInitializeEx");

    // IWICImagingFactoryの生成
    ComPtr<IWICImagingFactory2> m_WICImagingFactory;
    IF_FAILED_MESSAGE_RETURN(
        CoCreateInstance(
            CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER,
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
    // ファイルサイズ取得
    ULONGLONG filesize;
    {
        STATSTG stat;
        IF_FAILED_MESSAGE_RETURN(
            m_fsRead->Stat(&stat, STATFLAG_NONAME),
            "Stat");
        filesize = stat.cbSize.QuadPart;
    }

    int foundCount = 0;

    for (size_t i = 0; i < 80000; i++)
    {
        // 開始位置を探索
        byte buffer[2];
        ULONG cbRead;
        IF_FAILED_MESSAGE_RETURN(
            m_fsRead->Read(buffer, 2, &cbRead),
            "Read");
        if (cbRead != 2)
        {
            ULARGE_INTEGER position;
            m_fsRead->Seek({0}, STREAM_SEEK_CUR, &position);
            if (cbRead == 0)
            {
                WCHAR msg[128];
                wsprintf(msg, TEXT("2byte読み込みに失敗\n position= %I64x\n buffer= %2x%2x"), position.QuadPart, buffer[0], buffer[1]);
                MSG_BOX(msg);
                return -1;
            }
            else if (cbRead == 1)
            {
                LARGE_INTEGER one;
                one.QuadPart = 1;
                IF_FAILED_MESSAGE_RETURN(
                    m_fsRead->Seek(one, STREAM_SEEK_CUR, NULL),
                    "Seek (reposition of a bit)");
            }
        }
        // EOSかどうかチェック
        if (buffer[0] == 0xff && buffer[1] == 0xd8)
        {
            // 検出したらシークポインタをReadで動いた分戻す
            {
                LARGE_INTEGER dLibMove;
                dLibMove.QuadPart = -2;
                IF_FAILED_MESSAGE_RETURN(
                    m_fsRead->Seek(dLibMove, STREAM_SEEK_CUR, NULL),
                    "Seek");
            }

            // 出力先ファイルの生成・ストリーム取得
            ComPtr<IWICStream> m_fsWrite;
            IF_FAILED_MESSAGE_RETURN(
                m_WICImagingFactory->CreateStream(&m_fsWrite),
                "CreateStream (of out)");
            // 初期化
            {
                WCHAR filename[128];
                wsprintf(filename, FILENAME_OUT_COUNTED, foundCount);
                IF_FAILED_MESSAGE_RETURN(
                    m_fsWrite->InitializeFromFilename(filename, GENERIC_WRITE),
                    "InitializeFromFilename (of out)");
            }

            // デコードフレームの取得
            ComPtr<IWICBitmapFrameDecode> m_FrameDecode;
            {
                // デコーダーの取得
                ComPtr<IWICBitmapDecoder> m_Decoder;
                IF_FAILED_MESSAGE_RETURN(
                    m_WICImagingFactory->CreateDecoder(
                        GUID_ContainerFormatJpeg, NULL, &m_Decoder),
                    "CreateDecoder");
                // 初期化してストリームを読ませる
                IF_FAILED_MESSAGE_RETURN(
                    m_Decoder->Initialize(m_fsRead.Get(), WICDecodeMetadataCacheOnDemand),
                    "Initialize (of Decoder)");
                // 1枚目のフレームを取得
                IF_FAILED_MESSAGE_RETURN(
                    m_Decoder->GetFrame(0, &m_FrameDecode),
                    "GetFrame");
            }

            // エンコーダの生成
            ComPtr<IWICBitmapEncoder> m_Encoder;
            IF_FAILED_MESSAGE_RETURN(
                m_WICImagingFactory->CreateEncoder(
                    GUID_ContainerFormatPng, NULL, &m_Encoder),
                "CreateEncoder");
            // 初期化
            IF_FAILED_MESSAGE_RETURN(
                m_Encoder->Initialize(m_fsWrite.Get(), WICBitmapEncoderNoCache),
                "Initialize (of Encoder)");

            // フレームのエンコード処理
            {
                // フレームを生成
                ComPtr<IWICBitmapFrameEncode> m_FrameEncode;
                IF_FAILED_MESSAGE_RETURN(
                    m_Encoder->CreateNewFrame(
                        &m_FrameEncode, NULL),
                    "CreateNewFrame");
                // 初期化
                IF_FAILED_MESSAGE_RETURN(
                    m_FrameEncode->Initialize(NULL),
                    "Initialize (of FrameEncode)");

                // データコピー
                // 画像サイズ
                {
                    UINT width, height;
                    IF_FAILED_MESSAGE_RETURN(
                        m_FrameDecode->GetSize(&width, &height),
                        "GetSize");
                    IF_FAILED_MESSAGE_RETURN(
                        m_FrameEncode->SetSize(width, height),
                        "SetSize");
                }
                // ピクセルフォーマット
                {
                    WICPixelFormatGUID fmtGUID;
                    IF_FAILED_MESSAGE_RETURN(
                        m_FrameDecode->GetPixelFormat(&fmtGUID),
                        "GetPixelFormat");
                    IF_FAILED_MESSAGE_RETURN(
                        m_FrameEncode->SetPixelFormat(&fmtGUID),
                        "SetPixelFormat");
                }
                // 画素データ
                IF_FAILED_MESSAGE_RETURN(
                    m_FrameEncode->WriteSource(m_FrameDecode.Get(), NULL),
                    "WriteSource");

                // コミット
                IF_FAILED_MESSAGE_RETURN(
                    m_FrameEncode->Commit(),
                    "Commit (of FrameEncode)")
            }

            // エンコーダのコミット
            IF_FAILED_MESSAGE_RETURN(
                m_Encoder->Commit(),
                "Commit (of Encoder)");
            // ストリームのコミット
            IF_FAILED_MESSAGE_RETURN(
                m_fsWrite->Commit(STGC_DEFAULT),
                "Commit (of out)");

            foundCount++;
        }
        else
            continue;
    }

    // すべて終了
    {
        WCHAR msg[128];
        wsprintf(msg, TEXT("終了 検出数: %d"), foundCount);
        MSG_BOX(msg);
        return 0;
    }
}