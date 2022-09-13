#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <winerror.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include <combaseapi.h>

#include <wincodec.h>

#define FILENAME_IN L"sample1.MPO"

#define MSG_BOX(message) MessageBox(NULL, message, TEXT("WICStreamTest1"), MB_OK)

void makeErrorMessageBox(HRESULT hr, LPCWSTR name)
{
    WCHAR msg[128];
    wsprintf(msg, TEXT("HRESULT %x in %s"), hr, name);
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

// IWICStreamの検証

// ! ReadはISequentialStream::Read、SeekはIStream::Seekでありなぜかインターフェースが別
// * Readはカレントポインタを正常に移動させる
// * 一回もReadをせずにSeekを行うと異常な動作をする
// * Readのcbは0でもよい、つまりダミーのReadを行うことができる

// * ファイルサイズを取得するにはStatしてcbSizeを見る

int wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // COMの初期化
    IF_FAILED_MESSAGE_RETURN(
        CoInitializeEx(NULL, COINIT_MULTITHREADED),
        "CoInitializeEx");

    // ImagingFactoryの生成
    ComPtr<IWICImagingFactory2> m_WICImagingFactory;
    IF_FAILED_MESSAGE_RETURN(
        CoCreateInstance(
            CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_WICImagingFactory)),
        "CoCreateInstance (with WICImagingFactory)");

    // ストリームの取得
    ComPtr<IWICStream> m_fs;
    IF_FAILED_MESSAGE_RETURN(
        m_WICImagingFactory->CreateStream(&m_fs),
        "CreateStream");
    IF_FAILED_MESSAGE_RETURN(
        m_fs->InitializeFromFilename(FILENAME_IN, GENERIC_READ),
        "IWICStream::InitializeFromFilename");

    // メタデータの取得、表示
    {
        STATSTG statstg;
        IF_FAILED_MESSAGE_RETURN(
            m_fs->Stat(&statstg, STATFLAG_NONAME),
            "IStream::Stat");
        WCHAR msg[256];
        wsprintf(msg,TEXT("LENGTH: %d"),statstg.cbSize.QuadPart/1024);
        MSG_BOX(msg);
    }

    {
        // ダミーのRead
        byte buffer[0x1000];
        ULONG cbRead;
        IF_FAILED_MESSAGE_RETURN(
            m_fs->Read(buffer, 0, &cbRead),
            "ISequentialStream::Read");
    }

    // 動かしてみる
    LARGE_INTEGER libMove;
    libMove.QuadPart = 0x1000;
    ULARGE_INTEGER libNewPosition;
    IF_FAILED_MESSAGE_RETURN(
        m_fs->Seek(libMove, STREAM_SEEK_CUR, &libNewPosition),
        "IStream::Seek");

    // もう一回
    {
        // 読んでみる
        byte buffer[0x1000];
        ULONG cbRead;
        IF_FAILED_MESSAGE_RETURN(
            m_fs->Read(buffer, 0x1000, &cbRead),
            "ISequentialStream::Read");

        // 表示する
        WCHAR msg[128];
        wsprintf(msg, TEXT("Result of Read:\n At 0x0000, Value is %x\n At0x0800, Value is %x"), buffer[0], buffer[0x0800]);
        MSG_BOX(msg);
    }
}