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
#define FILENAME_OUT L"MPOLoadTest1_out.png"
#define MSG_BOX(message) MessageBox(NULL, message, TEXT("MPOLoadTest1"), MB_OK)

void makeErrorMessageBox(HRESULT hr, LPCWSTR name)
{
    WCHAR msg[128];
    wsprintf(msg, TEXT("HRESULT %x in %s"), hr, name);
    MSG_BOX(msg);
}

// 参考文献 https://docs.microsoft.com/ja-jp/windows/win32/wic/-wic-codec-jpegmetadataencoding
// 参考文献をベースに冒頭をストリームからの取得にした

// MPOから1枚だけJpegを切り出してみる
// そしてPNGで保存してみる

// m_ というプレフィックスを用いているが、恐らくmanagedの略

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

    // ファイルのロードとデコード
    ComPtr<IWICBitmapFrameDecode> m_FrameDecode;
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
        ComPtr<IWICBitmapDecoder> m_Decoder;
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

        // フレームの取得
        {
            HRESULT hr = m_Decoder->GetFrame(0, &m_FrameDecode);
            if (FAILED(hr))
            {
                makeErrorMessageBox(hr, TEXT("GetFrame"));
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
        HRESULT hr = m_fsWrite->InitializeFromFilename(FILENAME_OUT, GENERIC_WRITE);
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

    // エンコーダにデータをコピーしてコミット
    {
        // エンコーダにフレームを生成
        ComPtr<IWICBitmapFrameEncode> m_FrameEncode;
        {
            HRESULT hr=m_Encoder->CreateNewFrame(&m_FrameEncode,NULL);
            if(FAILED(hr))
            {
                makeErrorMessageBox(hr,TEXT("CreateNewFrame"));
                return -1;
            }
        }
        // 初期化
        {
            HRESULT hr=m_FrameEncode->Initialize(NULL);
            if(FAILED(hr))
            {
                makeErrorMessageBox(hr,TEXT("Initialize (of FrameEncode)"));
                return -1;
            }
        }

        // サイズをコピー
        {
            // 取得
            UINT width,height;
            {
                HRESULT hr=m_FrameDecode->GetSize(&width,&height);
                if(FAILED(hr))
                {
                    makeErrorMessageBox(hr,TEXT("GetSize"));
                    return -1;
                }
            }
            // 設定
            {
                HRESULT hr=m_FrameEncode->SetSize(width,height);
                if(FAILED(hr))
                {
                    makeErrorMessageBox(hr,TEXT("SetSize"));
                }
            }
        }
        // 解像度情報をコピー
        {
            //! このサンプルでは不要なのでパス
        }
        // ピクセルフォーマットをコピー
        {
            // 取得
            WICPixelFormatGUID fmtGUID;
            {
                HRESULT hr=m_FrameDecode->GetPixelFormat(&fmtGUID);
                if(FAILED(hr))
                {
                    makeErrorMessageBox(hr,TEXT("GetPixelFormat"));
                    return -1;
                }
            }
            // 設定
            {
                HRESULT hr=m_FrameEncode->SetPixelFormat(&fmtGUID);
                if(FAILED(hr))
                {
                    makeErrorMessageBox(hr,TEXT("GetPixelFormat"));
                    return -1;
                }
            }
        }
        // ピクセルデータのコピー
        {
            HRESULT hr=m_FrameEncode->WriteSource(m_FrameDecode.Get(),NULL);
            if(FAILED(hr))
            {
                makeErrorMessageBox(hr,TEXT("WriteSource"));
                return -1;
            }
        }

        // フレームのコミット
        {
            HRESULT hr=m_FrameEncode->Commit();
            if(FAILED(hr))
            {
                makeErrorMessageBox(hr,TEXT("Commit (of FrameEncode)"));
                return -1;
            }
        }
    }

    // エンコーダのコミット
    {
        HRESULT hr=m_Encoder->Commit();
        if(FAILED(hr))
        {
            makeErrorMessageBox(hr,TEXT("Commit (of Encoder)"));
            return -1;
        }
    }
    // ストリームのコミット
    {
        HRESULT hr=m_fsWrite->Commit(STGC_DEFAULT);
        if(FAILED(hr))
        {
            makeErrorMessageBox(hr,TEXT("Commit (of Stream out)"));
            return -1;
        }
    }

    // 全て成功
    MSG_BOX(TEXT("全て成功"));
    return 0;
}