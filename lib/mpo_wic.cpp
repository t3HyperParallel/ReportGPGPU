
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


ReadMPOError T4HP::MPO::GetSecondImageLocate(
    IStream *stream,
    LARGE_INTEGER *pLocate,
    GetMoreImageLocates_Info *pInfo=nullptr)
{
    {
        DEF_GET_buffer(stream, 2);
        if (!MATCH_2BYTE(buffer, 0xFF, 0xD8))
            return JpegSOIError;
    }
    while (TRUE)
    {
        DEF_GET_buffer(stream, 8);
        if (buffer[0] != 0xFF)
            return JpegInvalidMarkerError;
        if (buffer[1] != 0xE2)
        {
            LARGE_INTEGER dLibMove;
            dLibMove.QuadPart = UNMARSHAL_2BYTE(buffer + 2, MARSHAL_BE) - 6;
            CHECK_RETURN(stream->Seek(dLibMove, SEEK_CUR, NULL), StreamError);
            continue;
        }
        if (MATCH_4BYTE(buffer + 4, 'M', 'P', 'F', 0))
            break;
        else
            return MPInvalidJpegApp2;
    }
    ULARGE_INTEGER startOfMPHeader;
    CHECK_RETURN(stream->Seek({0}, SEEK_CUR, &startOfMPHeader),StreamError);
    BOOLEAN isBigEndian;
    ULONG numberOfImages = 0;
    {
        DEF_GET_buffer(stream, 8);
        if (MATCH_4BYTE(buffer, 0x49, 0x49, 0x2A, 0))
            isBigEndian = FALSE;
        else if (MATCH_4BYTE(buffer, 0x4D, 0x4D, 0, 0x2A))
            isBigEndian = TRUE;
        else
            return MPInvalidEndianToken;
        LARGE_INTEGER dLibMove;
        dLibMove.QuadPart = UNMARSHAL_4BYTE(buffer + 4, isBigEndian) - 8;
        CHECK_RETURN(stream->Seek(dLibMove, SEEK_CUR, NULL), StreamError);
    }
    ULONG countMPIndex;
    {
        DEF_GET_buffer(stream, 2);
        countMPIndex = UNMARSHAL_2BYTE(buffer, isBigEndian);
    }
    for (size_t idxNo = 0; idxNo < countMPIndex; idxNo++)
    {
        DEF_GET_buffer(stream, 12);
        USHORT tag = UNMARSHAL_2BYTE(buffer, isBigEndian);
        USHORT type = UNMARSHAL_2BYTE(buffer + 2, isBigEndian);
        ULONG count = UNMARSHAL_4BYTE(buffer + 4, isBigEndian);
        ULONG value = UNMARSHAL_4BYTE(buffer + 8, isBigEndian);
        if ((tag < 0xB000) || (0xB004 < tag))
            return MPIndexInformationInvalidTag;
        if (tag != 0xB001)
            continue;
        if (type != 4)
            return MPIndexInformationInvalidTag;
        if (count != 1)
            return MPIndexInformationMismatchCount;
        numberOfImages = value;
    }
    if (numberOfImages < 2)
        return MPIndexInformationNotFoundTag;
    {
        LARGE_INTEGER dLibMove;
        dLibMove.QuadPart = 20;
        CHECK_RETURN(stream->Seek(dLibMove, SEEK_CUR, NULL), StreamError);
    }
    {
        DEF_GET_buffer(stream, 16);
        pLocate->QuadPart = UNMARSHAL_4BYTE(buffer + 8, isBigEndian) + startOfMPHeader.QuadPart;
    }
    if (pInfo != nullptr)
    {
        pInfo->SourceStream = stream;
        pInfo->StartOfMPHeader = startOfMPHeader;
        pInfo->IsBigEndian = isBigEndian;
        pInfo->NumberOfImages = numberOfImages;
    }
    return OK;
}