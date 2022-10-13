# library

## メモ

### ビルドに関して

参考:<https://learn.microsoft.com/en-us/cpp/build/walkthrough-creating-and-using-a-static-library-cpp?redirectedfrom=MSDN&view=msvc-170#to-add-a-class-to-the-static-library>
7.のNoteを参考

```ps1
cl.exe /c /EHsc $filename /std:c++17
lib.exe $filename
```

+ /std:c++17
  名前空間宣言で名前空間を入れ子にするのに必要。

## mpo_wic.h

### T4HP::MPO::GetSecondImageLocate

ストリーム上のJPEG画像からMPFインデックス情報の画像位置をpLocatesに出力します。

#### (Arg1) IStream *sourceJpeg

当該JpegのSOIシークを合わせたストリーム

#### (Arg2) LARGE_INTEGER *pLocates

出力先

#### (Arg3,optional) GetMoreImageLocates_Info *pInfo

GetMoreImageLocatesに渡す構造体</br>
GetMoreImageLocatesしない場合はnull

#### (Return) ReadMPOError

どの辺りでエラーを出したかを示す。
