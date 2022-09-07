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
#define MSG_BOX(message) MessageBox(NULL, message, TEXT("MPOLoadTest1"), MB_OK)

void makeErrorMessageBox(HRESULT hr, LPCWSTR name)
{
    WCHAR msg[128];
    wsprintf(msg, TEXT("HRESULT %x in %s"), hr, name);
    MSG_BOX(msg);
}

// MPOから1枚だけJpegを切り出してみる
// そしてPNGで保存してみる

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // COMの初期化
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
        {
            makeErrorMessageBox(hr, TEXT("CoInitializeEx"));
            return -1;
        }
    }

    // IWICImagingFactoryの生成
    ComPtr<IWICImagingFactory2> m_WICImagingFactory;
    {
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_WICImagingFactory));
        if (FAILED(hr))
        {
            makeErrorMessageBox(hr, TEXT("CoCreateInstance (of WICImagingFactory)"));
            return -1;
        }
    }

    // ファイルのロード
    ComPtr<IWICBitmapDecoder> m_Decoder;
    {
        // 読み込みのストリームの生成
        ComPtr<IWICStream> m_fsRead; // ブロックで生存期間管理
        {
            HRESULT hr = m_WICImagingFactory->CreateStream(&m_fsRead);
            if (FAILED(hr))
            {
                makeErrorMessageBox(hr, TEXT("CreateStream (of in)"));
                return -1;
            }
        }
        // 初期化
        {
            HRESULT hr = m_fsRead->InitializeFromFilename(FILENAME_IN, GENERIC_READ);
            if (FAILED(hr))
            {
                makeErrorMessageBox(hr, TEXT("InitializeFromFilename (of Stream in)"));
                return -1;
            }
        }

        // デコーダの生成
        {
            HRESULT hr = m_WICImagingFactory->CreateDecoder(
                GUID_ContainerFormatJpeg, NULL,
                &m_Decoder);
            if (FAILED(hr))
            {
                makeErrorMessageBox(hr, TEXT("CreateDecoder"));
                return -1;
            }
        }
        // 初期化
        {
            HRESULT hr = m_Decoder->Initialize(m_fsRead.Get(), WICDecodeMetadataCacheOnDemand);
            if (FAILED(hr))
            {
                makeErrorMessageBox(hr, TEXT("Initialize (of Decoder)"));
                return -1;
            }
        }
    }

    // 書き込みのストリームの生成
    ComPtr<IWICStream> m_fsWrite;
    {
        HRESULT hr = m_WICImagingFactory->CreateStream(&m_fsWrite);
        if (FAILED(hr))
        {
            makeErrorMessageBox(hr, TEXT("CreateStream (of out)"));
            return -1;
        }
    }
    // 初期化
    {
        HRESULT hr = m_fsWrite->InitializeFromFilename(TEXT("MPOLoadTest1_out.png"), GENERIC_WRITE);
        if (FAILED(hr))
        {
            makeErrorMessageBox(hr, TEXT("InitializeFromFilename (of out)"));
            return -1;
        }
    }

    // エンコーダの生成
    ComPtr<IWICBitmapEncoder> m_Encoder;
    {
        HRESULT hr = m_WICImagingFactory->CreateEncoder(
            GUID_ContainerFormatPng, NULL,
            &m_Encoder);
        if (FAILED(hr))
        {
            makeErrorMessageBox(hr, TEXT("CreateEncoder"));
            return -1;
        }
    }
    {
        HRESULT hr = m_Encoder->Initialize(m_fsWrite.Get(), WICBitmapEncoderNoCache);
        if (FAILED(hr))
        {
            makeErrorMessageBox(hr, TEXT("Initialize (of Encoder)"));
            return -1;
        }
    }

    // 全て成功
    MSG_BOX(TEXT("全て成功"));
    return 0;
}