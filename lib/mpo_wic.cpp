
#include "mpo_wic.h"
using T4HP::MPO::ReadMPOError;

#define CHECK_RETURN(formula, value) \
    {                                \
        HRESULT hr = (formula);      \
        if (FAILED(hr))              \
            return value;            \
    }

#define DEF_GET_buffer(stream, length) \
    BYTE buffer[length];               \
    CHECK_RETURN(stream->Read(buffer, length, NULL), ReadMPOError::StreamError);

ReadMPOError T4HP::MPO::ReadMPO_SearchMPF(IStream *fs)
{
    {
        DEF_GET_buffer(fs, 2);
        if (MATCH_2BYTE(buffer, 0xFF, 0xD8))
            return JpegSOIError;
    }
    while (TRUE)
    {
        DEF_GET_buffer(fs, 8);
        if (buffer[0] != 0xFF)
            return JpegInvalidMarkerError;
        if (buffer[1] != 0xE2)
        {
            LARGE_INTEGER dLibMove;
            dLibMove.QuadPart = UNMARSHAL_2BYTE(buffer + 2, MARSHAL_BE) - 6;
            CHECK_RETURN(fs->Seek(dLibMove, SEEK_CUR, NULL));
            continue;
        }
        if (MATCH_4BYTE(buffer + 4, 'M', 'P', 'F', 0))
            break;
        else
            return MPInvalidJpegApp2;
    }
    return OK;
}

ReadMPOError T4HP::MPO::ReadMPO_ReadMPF_Part1(
    IStream *fs,
    ULARGE_INTEGER *pStartOfMPHeader,
    BOOL *pIsBigEndian,
    ULONG *pNumberOfImages)
{
    CHECK_RETURN(fs->Seek({0}, SEEK_CUR, pStartOfMPHeader));
    {
        DEF_GET_buffer(fs, 8);
        if (MATCH_4BYTE(buffer, 0x49, 0x49, 0x2A, 0))
            *pIsBigEndian = FALSE;
        else if (MATCH_4BYTE(buffer, 0x4D, 0x4D, 0, 0x2A))
            *pIsBigEndian = TRUE;
        else
            return MPInvalidEndianToken;
        LARGE_INTEGER dLibMove;
        dLibMove.QuadPart = UNMARSHAL_4BYTE(buffer + 4, *pIsBigEndian) - 8;
        CHECK_RETURN(fs->Seek(dLibMove, SEEK_CUR, NULL));
    }
    int numberOfMPIndex;
    {
        DEF_GET_buffer(fs, 2);
        numberOfMPIndex = UNMARSHAL_2BYTE(buffer, *pIsBigEndian);
    }
    for (size_t idxNo = 0; idxNo < numberOfMPIndex; idxNo++)
    {
        DEF_GET_buffer(fs, 12);
        USHORT tag = UNMARSHAL_2BYTE(buffer, *pIsBigEndian);
        USHORT type = UNMARSHAL_2BYTE(buffer + 2, *pIsBigEndian);
        ULONG count = UNMARSHAL_4BYTE(buffer + 4, *pIsBigEndian);
        ULONG defaultValue = UNMARSHAL_4BYTE(buffer + 4, *pIsBigEndian);
        if ((tag < 0xB000) || (0xB004 < tag))
            return MPIndexInformationInvalidTag;
        if (tag != 0xB001)
            continue;
        if (type != 4)
            return MPIndexInformationInvalidTag;
        if (defaultValue != 1)
            return MPIndexInformationMismatchDefault;
        *pNumberOfImages = count;
    }
    {
        LARGE_INTEGER dLibMove;
        dLibMove.QuadPart = 4;
        CHECK_RETURN(fs->Seek(dLibMove, SEEK_CUR, NULL));
    }
    return OK;
}

ReadMPOError T4HP::MPO::ReadMPO_ReadMPF_Part2(
    IStream *fs,
    ULARGE_INTEGER startOfMPHeader,
    BOOL isBigEndian,
    ULONG numberOfImages,
    [out] ULARGE_INTEGER *paLocates)
{
    {
        DEF_GET_buffer(fs,16);
        paLocates[0].QuadPart=UNMARSHAL_4BYTE(buffer+8,isBigEndian);
    }
    for (size_t idxNo = 1; idxNo < numberOfImages; idxNo++)
    {
        DEF_GET_buffer(fs,16);
        paLocates[idxNo].QuadPart=UNMARSHAL_4BYTE(buffer+8,isBigEndian)+startOfMPHeader.QuadPart;
    }
}