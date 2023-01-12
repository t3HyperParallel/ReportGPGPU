
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

    IMFActivate **apMFTActive;
    UINT32 num_apMFTActive;
    HRESULT_exit(
        MFTEnumEx(
            MFT_CATEGORY_VIDEO_DECODER,
            MFT_ENUM_FLAG_HARDWARE          // 含有フラグ ハードウェアコーデックはレジストリキーで潰されていることがあるらしい
                | MFT_ENUM_FLAG_FIELDOFUSE, // 除外フラグ
            NULL, NULL,
            &apMFTActive, &num_apMFTActive),
        L"MFTEnumEx");
    {
        WCHAR mes[128];
        wsprintfW(mes, L"found %d MFTActive", num_apMFTActive);
        MessageBoxW(NULL, mes, L"num_apMFTActive", MB_OK);
    }
    for (UINT32 index = 0; index < num_apMFTActive; index++)
    {
        WCHAR mes[512];
        UINT32 num_mes;
        HRESULT_exit(
            apMFTActive[index]->GetString(
                MFT_FRIENDLY_NAME_Attribute, mes, 512, &num_mes),
            L"IMFActive::GetString");
        MessageBoxW(NULL,mes,L"apMFActive",MB_OK);
    }

    CoTaskMemFree(apMFTActive);
    HRESULT_exit(MFShutdown(), L"MFShutdown");
}
