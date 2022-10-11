#pragma once
#ifndef __T4HP_MPO_WIC_H__
#define __T4HP_MPO_WIC_H__

#include <wrl.h>
#include <combaseapi.h>
#include <wincodec.h>


#define MARSHAL_BE TRUE
#define MARSHAL_LE FALSE

#define UNMARSHAL_4BYTE(ptr, isBE)                                        \
    (                                                                     \
        isBE ? (((ptr)[0] << 24) | ((ptr)[1] << 16) | ((ptr)[2] << 8) | (ptr)[3]) \
             : (((ptr)[3] << 24) | ((ptr)[2] << 16) | ((ptr)[1] << 8) | (ptr)[0]))

#define UNMARSHAL_2BYTE(ptr, isBE) \
    (isBE ? (((ptr)[0] << 8) | (ptr)[1]) : (((ptr)[1] << 8) | (ptr)[0]))

#define MATCH_4BYTE(ptr, pt0, pt1, pt2, pt3) \
    ((ptr)[0] == (pt0) && (ptr)[1] == (pt1) && (ptr)[2] == (pt2) && (ptr)[3] == (pt3))

#define MATCH_2BYTE(ptr, pt0, pt1) \
    ((ptr)[0] == (pt0) && (ptr)[1] == (pt1))

namespace T4HP::MPO
{
    enum ReadMPOError : int
    {
        OK=0,
        StreamError = -101,
        JpegSOIError=-211,
        JpegInvalidMarkerError=-201,
        MPInvalidJpegApp2=-311,
        MPInvalidEndianToken=-410,
        MPIndexInformationInvalidTag=-510,
        MPIndexInformationMismatchType=-511,
        MPIndexInformationMismatchDefault=-512
    };
    // SOIを与えられるとMPヘッダまでシークする
    ReadMPOError ReadMPO_SearchMPF(IStream *fs);

    // 詳しくはPart2参照
    ReadMPOError ReadMPO_ReadMPF_Part1(
        IStream *fs,
        [out] ULARGE_INTEGER *pStartOfMPHeader,
        [out] BOOL *pIsBigEndian,
        [out] ULONG *pNumberOfImages);
    // paLocatesに投入する配列は各自で確保してください
    // この関数は開放の責任を負いません
    ReadMPOError ReadMPO_ReadMPF_Part2(
        IStream *fs,
        ULARGE_INTEGER startOfMPHeader,
        BOOL isBigEndian,
        ULONG numberOfImages,
        [out] ULARGE_INTEGER *paLocates);
}






#endif