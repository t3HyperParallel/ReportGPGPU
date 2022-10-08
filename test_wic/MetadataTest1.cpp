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

#define FILENAME_IN L"sample1.MPO"

#define CHECK_HRESULT(formula)  \
    {                           \
        HRESULT hr = (formula); \
        if (FAILED(hr))         \
            exit(hr);           \
    }

#define CHECK_MSG_HRESULT(formula, msg)                   \
    {                                                     \
        HRESULT hr = (formula);                           \
        if (FAILED(hr))                                   \
        {                                                 \
            MessageBox(NULL, msg, L"ReuseStream", MB_OK); \
            exit(hr);                                     \
        }                                                 \
    }

#define EXIT_MSG(msg)                                  \
    {                                                  \
        MessageBoxW(NULL, msg, L"ReuseStream", MB_OK); \
        exit(-1);                                      \
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
    // SOIを検出する
    {
        DEF_GET_buffer(m_fs, 2);
        if (!(buffer[0] == 0xFF && buffer[1] == 0xD8))
        {
            exit(-1);
        }
    }
    // APP2 を探す
    while (TRUE)
    {
        // マーカーセグメントのヘッダについて
        // byte 0,1 マーカー
        // byte 2,3 マーカーセグメント長（ビッグエンディアン）
        DEF_GET_buffer(m_fs, 4);
        // マーカーと思われる先頭byteがFFで始まらないでない場合はエラー
        if (buffer[0] != 0xFF)
            EXIT_MSG(L"Invalid Marker");
        // APP2なら離脱
        if ((MatchDoubleBytes(buffer, 0xFF, 0xE2)))
            break;
        // APP2以外はスキップ、次のマーカーセグメントへ
        int segmentLength = UnmarshalDoubleByteToLong(buffer + 2, MARSHAL_BE); // これは常にビッグエンディアン
        LARGE_INTEGER dLibMove;
        dLibMove.QuadPart = segmentLength - 2; // マーカーはサイズに含まない
        ULARGE_INTEGER libNewPosition;
        CHECK_HRESULT(m_fs->Seek(dLibMove, SEEK_CUR, &libNewPosition));
    }
    // APP2の検証
    {
        DEF_GET_buffer(m_fs, 4);
        // APP2がMPFでなければ弾く
        if (!MatchQuadBytes(buffer, 'M', 'P', 'F', 0))
            EXIT_MSG(L"APP2 is not \"MPF \".");
    }

    ULARGE_INTEGER startOfMPHeader;
    CHECK_HRESULT(m_fs->Seek({0}, SEEK_CUR, &startOfMPHeader));
    // MPヘッダを読む
    BOOL isBigEndian; // エンディアンはMPヘッダに含まれる
    {
        // MPヘッダについて
        // MPヘッダはTIFFヘッダと互換性がある
        // 4byte エンディアン情報
        // 4byte 先頭IDFへのオフセット（開始位置はMPヘッダの起点）
        // MPFではエンディアンを4byteとしているが、準拠元のTIFFでは3,4byte目はバージョン情報
        DEF_GET_buffer(m_fs, 8);

        // エンディアンを検証
        if (buffer[0] == 0x49 && buffer[1] == 0x49 && buffer[2] == 0x2A && buffer[3] == 0x00)
            isBigEndian = FALSE;
        else if (buffer[0] == 0x4D && buffer[1] == 0x4D && buffer[2] == 0x00 && buffer[3] == 0x2A)
            isBigEndian = TRUE;
        else
            EXIT_MSG(L"Invalid Endian info");

        // 最初のIDFへ移動
        LONG offsetToFirstIDF = UnmarshalQuadByteToLong(buffer + 4, isBigEndian);
        LARGE_INTEGER dLibMove;
        dLibMove.QuadPart = offsetToFirstIDF - 8; // ヘッダは8byteなので
        CHECK_HRESULT(m_fs->Seek(dLibMove, SEEK_CUR, NULL));
    }

    // MPインデックスIDFのカウントを読む
    ULONG MPIndexIDFCount;
    {
        DEF_GET_buffer(m_fs, 2);
        MPIndexIDFCount = UnmarshalDoubleByteToLong(buffer, isBigEndian);
    }

    // MPインデックス情報を読む
    ULONG numberOfImages = 0;
    for (size_t indexNumber = 0; indexNumber < MPIndexIDFCount; indexNumber++)
    {
        // MPインデックス情報について
        // byte 0,1 タグ（フィールドの種類を示す）
        // byte 2,3 タイプ（値の型を示す）
        // byte 4,5,6,7 カウント（何をカウントしているかは不明）
        // byte 8,9,10,11 値
        DEF_GET_buffer(m_fs, 12);
        ULONG tag = UnmarshalDoubleByteToLong(buffer, isBigEndian);
        ULONG count = UnmarshalQuadByteToLong(buffer + 4, isBigEndian);
        if (tag == 0xB000) // MPフォーマットバージョン、多分必ずある
        {
            if (!MatchDoubleBytes(buffer + 2, 0x00, 0x07))
                EXIT_MSG(L"MPFVersion : Type is not UNDEFINED(0x0007)");
            if (count != 4)
                EXIT_MSG(L"MPFVersion : Count is not 4");
            if (MatchQuadBytes(buffer + 8, '0', '1', '0', '0'))
                ;
            else
                EXIT_MSG(L"MPFVersion : Unknown Version");
        }
        else if (tag == 0xB001) // 記録画像数、多分必ずある
        {
            if (!MatchDoubleBytes(buffer + 2, 0x00, 0x04))
                EXIT_MSG(L"NumberOfImages : Type is not LONG(0x0004)");
            if (count != 1)
                EXIT_MSG(L"NumberOfImages : Count is not 1");
            numberOfImages = UnmarshalQuadByteToLong(buffer + 8, isBigEndian);
        }
        else if (tag == 0xB002) // MPエントリ、多分必ずある
        {
            if (!MatchDoubleBytes(buffer + 2, 0x00, 0x07))
                EXIT_MSG(L"MPEntry : Type is not UNDEFINED(0x0007)");
            if (count != numberOfImages * 16) // エントリは1個16byteだかららしい
                EXIT_MSG(L"MPEntry : Count is not NumberOfImages*16");
        }
        else if (tag == 0xB003) // 個別画像ユニークIDリスト、ない場合もある
        {
            if (!MatchDoubleBytes(buffer + 2, 0x00, 0x07))
                EXIT_MSG(L"ImageUIDList : Type is not UNDEFINED(0x0007)");
            if (count != numberOfImages * 33) // IDタグは1個33byteだかららしい
                EXIT_MSG(L"ImageUIDList : Count is not NumberOfImages*33");
        }
        else if (tag == 0xB004) // 撮影時総コマ数、ない場合もある
        {
            if (!MatchDoubleBytes(buffer + 2, 0x00, 0x04))
                EXIT_MSG(L"TotalFrames : Type is not LONG(0x04)");
            if (count != 1)
                EXIT_MSG(L"TotalFrames : Count is not 1");
        }
        else
            EXIT_MSG(L"Unknown MPIndex information Tag");
    }
    if (numberOfImages == 0)
        EXIT_MSG(L"field \"NumberOfImages\"(0xB001) is unfounded or 0");

    // 次のIDFまでのインデックスを読む
    ULONG MPIndexInformation_OffsetOfNextIDF;
    {
        DEF_GET_buffer(m_fs, 4);
        MPIndexInformation_OffsetOfNextIDF = UnmarshalQuadByteToLong(buffer, isBigEndian);
    }

    // MPエントリを読んで全ての画像の開始位置を推定する
    auto imageLocates = std::make_unique<LARGE_INTEGER[]>(numberOfImages);
    for (size_t indexNumber = 0; indexNumber < numberOfImages; indexNumber++)
    {
        DEF_GET_buffer(m_fs, 16);
        // 配置は以下の通り
        // byte 0,1,2,3 個別画像種別管理情報
        // byte 4,5,6,7 個別画像サイズ
        // byte 8,9,10,11 個別画像データオフセット
        //   起点はMPヘッダの開始位置
        //   符号なしなので1枚目の画像の開始位置は表現不能、代わりに0が入っている
        // byte 12,13 と 14,15 従属画像1,2のエントリ番号
        ULONG offset = UnmarshalQuadByteToLong(buffer + 8, isBigEndian);
        imageLocates[indexNumber].QuadPart = startOfMPHeader.QuadPart + offset;
    }

    // 2枚目の画像の推定開始位置にSOIがあるか検証
    CHECK_HRESULT(m_fs->Seek(imageLocates[1], SEEK_SET, NULL));
    {
        DEF_GET_buffer(m_fs, 4);
        if (MatchQuadBytes(buffer, 0xFF, 0xD8, 0xFF, 0xE1))
        {
            EXIT_MSG(L"Collect Locate of 2nd Image");
        }
        else
        {
            WCHAR msg[128];
            wsprintf(msg, TEXT("Miss Locate at %I64x"), imageLocates[1].QuadPart);
            MessageBoxW(NULL, msg, L"Info", MB_OK);
        }
    }
}