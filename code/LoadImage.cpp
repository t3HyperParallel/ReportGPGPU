#pragma once
// MSVCでコンパイルされることがわかっている場合pragma onceはお作法として正しい

#include <wincodec.h>
#include <memory>

#include <winerror.h> //FAILED,SUCCESSED
#include <wrl.h> //ComPtr
#include <objidl.h> //IStream
#include <Shlwapi.h> //SHCreateStreamOnFileEx
#include <wincodec.h> //WIC関連


namespace HyperP::Imaging
{
    class DataArray;
    struct RawBitmapAndMetaData;
    bool GetTopLevelRawsFromMPOFileName(LPCWSTR filepath,DataArray **ppDataArray,int maxCount,Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory);
} // namespace HyperP::Imaging

bool HyperP::Imaging::GetTopLevelRawsFromMPOFileName(LPCWSTR filepath,DataArray **ppDataArray,int maxCount,Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory=nullptr)
{
    Microsoft::WRL::ComPtr<IStream> comPtr_stream;
    if (FAILED(
        SHCreateStreamOnFile(
            filepath,
            STGM_READ|STGM_SHARE_DENY_WRITE,
            &comPtr_stream)
    ))return false;
    if (wicFactory.Get()==nullptr)
    {
        if(FAILED(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory) )
        ))return false;
    }

    // MPOの解体
    std::unique_ptr<std::shared_ptr<RawBitmapAndMetaData>[]>
        rawMetaPtrArray(new std::shared_ptr<RawBitmapAndMetaData>[maxCount]);
    int foundCount=0;
    for (int count = 0; count < maxCount; count++)
    {
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> comPtr_decoder;
        if(FAILED(
            wicFactory->CreateDecoderFromStream(
                comPtr_stream.Get(),
                NULL,
                WICDecodeMetadataCacheOnDemand,
                &comPtr_decoder )
        )) break;
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> comPtr_decoded;
        if (FAILED(comPtr_decoder->GetFrame(0,&comPtr_decoded)))break;
        Microsoft::WRL::ComPtr<IWICFormatConverter> comPtr_converter;
        if(FAILED(wicFactory->CreateFormatConverter(&comPtr_converter)))break;
        if (FAILED(
            comPtr_converter->Initialize(
                comPtr_decoded.Get(),
                GUID_WICPixelFormat32bppBGRA,// CUDAで扱うにはBGRA順じゃないといけない
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeCustom
            )
        ))return false;// フォーマット方法は毎回固定なので一つ失敗したら全部失敗
        rawMetaPtrArray[foundCount]=std::make_shared<RawBitmapAndMetaData>(comPtr_converter.Get());
        foundCount++;
    }
    if(foundCount==0)return false;
    *ppDataArray=new DataArray(foundCount,rawMetaPtrArray.get());
}

class HyperP::Imaging::DataArray
{
    std::unique_ptr<std::shared_ptr<RawBitmapAndMetaData>[]> _bitmapRawMeta;
    size_t _count;
    public:
    bool GetWhereId(size_t id,std::shared_ptr<RawBitmapAndMetaData> *wpRaw)
    {
        if (0<=id && id<_count)
        {
            *wpRaw=_bitmapRawMeta[id];
            return true;
        }
        else
        {
            return false;
        }
    }
    DataArray(size_t count,std::shared_ptr<RawBitmapAndMetaData> *ppRawMeta)
    {
        _count=count;
        _bitmapRawMeta=std::make_unique<std::shared_ptr<RawBitmapAndMetaData>[]>(count);
        for (size_t index = 0; index < count; index++)
        {
            _bitmapRawMeta[index]=ppRawMeta[index];
        }
    }
};

struct HyperP::Imaging::RawBitmapAndMetaData
{
    UINT width,height,stride,dataSize;
    std::unique_ptr<BYTE[]> raw;
    RawBitmapAndMetaData(IWICBitmapSource *pWICBitmapSource)
    {
        pWICBitmapSource->GetSize(&width,&height);
        stride=(width*32+7)/8;
        dataSize=stride*height;
        raw=std::make_unique<BYTE[]>(dataSize);
        WICRect rect={0};
        rect.X=0;
        rect.Y=0;
        rect.Width=width;
        rect.Height=height;
        pWICBitmapSource->CopyPixels(
            &rect,stride,dataSize,raw.get()
        );
    }
    RawBitmapAndMetaData(){}//デフォコン
};
