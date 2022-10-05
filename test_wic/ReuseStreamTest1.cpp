#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <winerror.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include <combaseapi.h>

#include <wincodec.h>

#include <stdlib.h>

#define CHECK_HRESULT(formula)  \
    {                           \
        HRESULT hr = (formula); \
        if (FAILED(hr))         \
            exit(hr);           \
    }

#define CHECK_MSG_HRESULT(formula, msg)                         \
    {                                                     \
        HRESULT hr = (formula);                           \
        if (FAILED(hr))                                   \
        {                                                 \
            MessageBox(NULL, msg, L"ReuseStream", MB_OK); \
            exit(hr);                                     \
        }                                                 \
    }

#define FILENAME_IN L"sample1.MPO"

#define FILENAME_OUT1 L"ReuseStreamTest1_out_1.png"
#define FILENAME_OUT2 L"ReuseStreamTest1_out_2.png"

void save(IWICImagingFactory2 *factory,IWICBitmapFrameDecode *src,LPCWSTR name)
{
    ComPtr<IWICStream> m_fs_out;
    CHECK_HRESULT(factory->CreateStream(&m_fs_out));
    CHECK_HRESULT(m_fs_out->InitializeFromFilename(name,GENERIC_WRITE));

    ComPtr<IWICBitmapEncoder> m_enc;
    CHECK_HRESULT(factory->CreateEncoder(
        GUID_ContainerFormatPng,NULL,&m_enc));
    CHECK_HRESULT(m_enc->Initialize(m_fs_out.Get(),WICBitmapEncoderNoCache));

    ComPtr<IWICBitmapFrameEncode> m_frame;
    CHECK_HRESULT(m_enc->CreateNewFrame(&m_frame,NULL));
    CHECK_HRESULT(m_frame->Initialize(NULL));
    UINT width,height;
    CHECK_HRESULT(src->GetSize(&width,&height));
    CHECK_HRESULT(m_frame->SetSize(width,height));
    WICPixelFormatGUID fmt;
    CHECK_HRESULT(src->GetPixelFormat(&fmt));
    CHECK_HRESULT(m_frame->SetPixelFormat(&fmt));
    CHECK_HRESULT(m_frame->WriteSource(src,NULL));

    CHECK_HRESULT(m_frame->Commit());
    CHECK_HRESULT(m_enc->Commit());
    CHECK_HRESULT(m_fs_out->Commit(STGC_DEFAULT));
}

// ? ストリーム使いまわしてもいいの？
// * 使った後のストリームのシークは可能
// * ストリームを再使用することはできる

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    CHECK_HRESULT(CoInitializeEx(NULL, COINIT_MULTITHREADED));
    ComPtr<IWICImagingFactory2> m_WICFactory;
    CHECK_HRESULT(CoCreateInstance(
        CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_WICFactory)));

    ComPtr<IWICStream> m_fs_in;
    CHECK_HRESULT(m_WICFactory->CreateStream(&m_fs_in));
    CHECK_HRESULT(m_fs_in->InitializeFromFilename(FILENAME_IN, GENERIC_READ));

    // デコード1回目
    ComPtr<IWICBitmapDecoder> m_dec1;
    CHECK_HRESULT(m_WICFactory->CreateDecoder(
        GUID_ContainerFormatJpeg, NULL, &m_dec1));
    CHECK_HRESULT(m_dec1->Initialize(m_fs_in.Get(), WICDecodeMetadataCacheOnDemand));

    ComPtr<IWICBitmapFrameDecode> m_frame1;
    CHECK_HRESULT(m_dec1->GetFrame(0, &m_frame1));

    save(m_WICFactory.Get(),m_frame1.Get(),FILENAME_OUT1);

    // なんとなくリード
    {
        BYTE buf[2];
        ULONG cbRead;
        CHECK_HRESULT(m_fs_in->Read(buf,2,&cbRead));
        if(cbRead==0)MessageBox(NULL,L"cbRead is 0",L"ReuseStream",MB_OK);
    }

    // シーク
    {
        LARGE_INTEGER distance;
        distance.QuadPart = 0x950E;
        ULARGE_INTEGER result;
        CHECK_HRESULT(m_fs_in->Seek(distance, STREAM_SEEK_SET, &result));
    }

    // デコード2回目
    ComPtr<IWICBitmapDecoder> m_dec2;
    CHECK_HRESULT(m_WICFactory->CreateDecoder(
        GUID_ContainerFormatJpeg, NULL, &m_dec2));
    CHECK_MSG_HRESULT(
        m_dec2->Initialize(m_fs_in.Get(), WICDecodeMetadataCacheOnDemand),
        L"dec2 Initialize failed");

    ComPtr<IWICBitmapFrameDecode> m_frame2;
    CHECK_HRESULT(m_dec2->GetFrame(0, &m_frame2));

    save(m_WICFactory.Get(),m_frame2.Get(),FILENAME_OUT2);


    // 完了
    MessageBox(NULL,L"Success",L"ReuseStream",MB_OK);
    exit(0);
}