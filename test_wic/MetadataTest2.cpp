#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <winerror.h>

#include <memory>

#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include <combaseapi.h>
#include <propapi.h>

#include <wincodec.h>

#include "../lib/mpo_wic.h"

#define FILENAME_IN L"sample1.MPO"

#define CHECK_HRESULT(formula)  \
    {                           \
        HRESULT hr = (formula); \
        if (FAILED(hr))         \
            exit(hr);           \
    }

#define CHECK_MSG_HRESULT(formula, msg)               \
    {                                                 \
        HRESULT hr = (formula);                       \
        if (FAILED(hr))                               \
        {                                             \
            MessageBox(NULL, msg, L"LibTest", MB_OK); \
            exit(hr);                                 \
        }                                             \
    }

#define EXIT_MSG(msg)                              \
    {                                              \
        MessageBoxW(NULL, msg, L"LibTest", MB_OK); \
        exit(-1);                                  \
    }

#define DEF_GET_buffer(stream, length) \
    BYTE buffer[length];               \
    CHECK_HRESULT(stream->Read(buffer, length, NULL));

// この記述方法であれば環境のエンディアンがどちらでも問題ない
ULONG UnmarshalQuadByteToLong(BYTE *bytes, BOOL isBigEndian)
{
    if (isBigEndian)
        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    else
        return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}
ULONG UnmarshalDoubleByteToLong(BYTE *bytes, BOOL isBigEndian)
{
    if (isBigEndian)
        return (bytes[0] << 8) | bytes[1];
    else
        return (bytes[1] << 8) | bytes[0];
}
#define MARSHAL_BE TRUE
#define MARSHAL_LE FALSE

// pBytesから4byteをチェックする
// 大きい数値型にするには再配置が必要になるのでこうしている
BOOL MatchQuadBytes(BYTE *pBytes, BYTE pattern0, BYTE pattern1, BYTE pattern2, BYTE pattern3)
{
    return (pBytes[0] == pattern0) && (pBytes[1] == pattern1) && (pBytes[2] == pattern2) && (pBytes[3] == pattern3);
}
BOOL MatchDoubleBytes(BYTE *pBytes, BYTE pattern0, BYTE pattern1)
{
    return (pBytes[0] == pattern0) && (pBytes[1] == pattern1);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    CHECK_HRESULT(CoInitializeEx(NULL, COINIT_MULTITHREADED));
    ComPtr<IWICImagingFactory2> m_WICFactory;
    CHECK_HRESULT(CoCreateInstance(
        CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_WICFactory)));

    ComPtr<IWICStream> m_fs;
    CHECK_HRESULT(m_WICFactory->CreateStream(&m_fs));
    CHECK_HRESULT(m_fs->InitializeFromFilename(FILENAME_IN, GENERIC_READ));

    // メタデータを読もう
    LARGE_INTEGER imageLocate;
    auto res = T4HP::MPO::GetSecondImageLocate(m_fs.Get(), &imageLocate, NULL);
    {
        WCHAR msg[128];
        wsprintf(msg, TEXT("LibError %d"), res);
        MessageBoxW(NULL, msg, L"Info", MB_OK);
    }
    {
        WCHAR msg[128];
        wsprintf(msg, TEXT("imageLocate is at %I64x"), imageLocate.QuadPart);
        MessageBoxW(NULL, msg, L"Info", MB_OK);
    }
}