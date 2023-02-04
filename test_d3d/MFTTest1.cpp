
#include <Windows.h>
#include <winerror.h>

#include <combaseapi.h>
#include <atlbase.h>

// ヘッダでのGUID定義のために必要
// ここで定義すべきかは不明
#define MF_INIT_GUIDS

// mfplat.lib
#include <mfapi.h>
#include <mfidl.h>

__forceinline void MesExit(LPCWSTR message, int exitCode)
{
    MessageBoxW(NULL, message, L"WindowTemplate", MB_OK);
    PostQuitMessage(exitCode);
}

__inline void HRESULT_exit(HRESULT hr, LPCWSTR errorAt)
{
    if (FAILED(hr))
    {
        WCHAR mes[128];
        wsprintfW(mes, L"HRESULT %x at %s", hr, errorAt);
        MesExit(mes, hr);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    HRESULT_exit(MFStartup(MF_VERSION), L"MFStartup");

    IMFActivate **apMFTActivate;
    UINT32 num_apMFTActivate;
    HRESULT_exit(
        MFTEnumEx(
            MFT_CATEGORY_VIDEO_DECODER,
            MFT_ENUM_FLAG_HARDWARE          // 含有フラグ ハードウェアコーデックはレジストリキーで潰されていることがあるらしい
                | MFT_ENUM_FLAG_FIELDOFUSE, // 除外フラグ
            NULL, NULL,
            &apMFTActivate, &num_apMFTActivate),
        L"MFTEnumEx");
    {
        WCHAR mes[128];
        wsprintfW(mes, L"found %d Hardware notLocked VideoDecoder MFTActivate", num_apMFTActivate);
        MessageBoxW(NULL, mes, L"num_apMFTActivate", MB_OK);
    }
    for (UINT32 index = 0; index < num_apMFTActivate; index++)
    {
        // MFTの名前を取得
        UINT32 mftName_stringEnd;
        const UINT32 mftName_arraySize=128;
        WCHAR mftName[mftName_arraySize];
        HRESULT_exit(
            apMFTActivate[index]->GetString(
                MFT_FRIENDLY_NAME_Attribute, mftName, mftName_arraySize, &mftName_stringEnd),
            L"IMFActive::GetString");

        // MFTを取得
        CComPtr<IMFTransform> m_mft;
        HRESULT_exit(
            apMFTActivate[index]->ActivateObject(IID_PPV_ARGS(&m_mft)),
            L"IMFActive::ActivateObject");
        // IMFAttributesを取得
        CComPtr<IMFAttributes> m_attr;
        HRESULT_exit(
            m_mft->GetAttributes(&m_attr),
            L"IMFTransform::GetAttributes");
        // MF_SA_D3D11_AWAREを取得
        UINT32 isD3D11Aware;
        HRESULT_exit(
            m_attr->GetUINT32(MF_SA_D3D11_AWARE, &isD3D11Aware),
            L"IMFAttributes::GetUINT32");
        {
            WCHAR mes[1024];
            wsprintfW(mes,L"%s\n\nMF_SA_D3D11_AWARE :%d",mftName,isD3D11Aware);
            MessageBoxW(NULL, mes, L"Report", MB_OK);
        }
        HRESULT_exit(
            apMFTActivate[index]->ShutdownObject(),
            L"IMFActivate::ShutdownObject");
    }

    CoTaskMemFree(apMFTActivate); // MFTEnumExで与えられる配列はコード側に開放責任がある
    HRESULT_exit(MFShutdown(), L"MFShutdown");
}
