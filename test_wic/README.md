# サンプル集

## コンパイルするには

Visual Studio Tools より適当な Visual C ビルドツールに一時的にパスが通っている状態になるスクリプトが同梱されている。

（PowerShell向けは Launch-VSDevShell.ps1 である。以下はPowerShellで操作を行う事を前提としている。）

コンパイラは cl.exe である。

ライブラリ user32.lib と gdi32.lib に依存しているのでそれもコンパイラに指定すると次のようになる。

```ps1
cl.exe filename user32.lib gdi32.lib
```

cl.exe はBOM付きUTF-8を前提としており、BOM無しUTF-8を与えると警告を出す。

## 各サンプルについて

### sample1.MPO

サンプル画像。撮影にはnew3DSを使用した。

MPOの形式はCIPA DC-007でMPフォーマットとして定められている。要点をかいつまむと以下の通りである。

+ JPEGのEOIの後にはどのようなビット列があってもよいことを活用して複数の画像を単一ファイルに納めている
+ 各JPEGコンテナのAPP2をMPフォーマット付属情報としている
  + フォーマット識別コードは 4D506400 ("MPF")
  + 最初のコンテナのMPフォーマット付属情報には**残りのコンテナの開始位置**が含まれる

### WICStreamTest1

WICには簡易的にファイルストリームオープンを行える方法が用意されている。

（もし詳細に制御する必要がある場合、SHCreateStreamOnFile を使用する。）

SeekとReadについて実験を行ったのがこのサンプルである。

### MPOLoadTest1

IWICBitmapDecoderには拡張子から形式を推定するInitializeFromFilenameという初期化方法が用意されているが、Windowsは標準にはMPOのデコーダーを用意していない。しかしMPOはJPEGを直接格納している為、JPEGコンテナの位置までシークしたストリームをJPEGデコーダーに与えることでMPOをデコードすることができる。

sample.MPO は2枚のJPEG画像を格納している。ファイルの冒頭からJPEGコンテナが存在するためシークなしでデコーダーに与えると1枚目の画像を出力する。

ディレクティブを指定せずにコンパイルするとMPOLoadTest1.exeが生成される。このプログラムはMPOLoadTest1_out.pngを生成する。これは左目用画像である。

2枚目の画像の位置までシークしてからデコーダーに与えると、当然2枚目の画像を生成する。

次の通りのコマンドでコンパイルすると、コマンドで定義されたシンボルにifdefが反応して別の内容のプログラムが生成される。MPOLoadTest1_2.exeはMPOLoadTest1_2_out.pngを生成する右目用画像を生成する。

```ps1
cl.exe .\MPOLoadTest1.cpp user32.lib gdi32.lib /DSECOND_IMAGE /Fe:MPOLoadTest1_2.exe
```

（VSCodeを使っているならエディタを左右分割して2枚の画像を並べると比較しやすい）

### ReuseStreamTest1

MPOもそうであるが、同時にアプリケーションで使用する複数のアイテムを単一のファイルに格納することは珍しいことではない。その場合、アイテム毎にストリームの開閉を行う事は非常に無駄が多い。従って、MPOファイルへのストリームを複数のJPEGデコーダで使い回すことは当然可能であるべきで、以前WinFormsでMPOをロードしたときも同様に行えることを確認している。

基本的なことはLoadTest1と同じであるが、ReuseStreamTest1はReuseStreamTest1_out_1.pngとReuseStreamTest1_out_2.pngを生成する。

### MetadataTest1.cpp

MPOのメタデータから、2枚目の画像の開始位置を読むテストである。同様の方法で3枚以上の画像が含まれる場合も対応可能と思われる。
（今回の実験では3枚目以降の画像がある場合は扱わない為、このコードはそれを保証しないこととする。）
JPEGのSOIを探す方法でも画像の開始位置を探索することが可能であるが、走査が必要となることを考慮すると場合によってはこちらの方が早いのではないかと思われる。
