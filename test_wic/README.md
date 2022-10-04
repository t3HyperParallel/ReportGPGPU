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

### WICStreamTest1

WICには簡易的にファイルストリームオープンを行える方法が用意されている。

（もし詳細に制御する必要がある場合、SHCreateStreamOnFile を使用する。）

SeekとReadについて実験を行ったのがこのサンプルである。

### MPOLoadTest1

IWICBitmapDecoderには拡張子から形式を推定するInitializeFromFilenameという初期化方法が用意されているが、Windowsは標準にはMPOのデコーダーを用意していない。しかしMPOはJPEGを直接格納している為、JPEGコンテナの位置までシークしたストリームをJPEGデコーダーに与えることでMPOをデコードすることができる。

sample.MPO は2枚のJPEG画像を格納している。ファイルの冒頭からJPEGコンテナが存在するためシークなしでデコーダーに与えると1枚目の画像を出力する。

ディレクティブを指定せずにコンパイルするとMPOLoadTest1.exeが生成される。このプログラムはMPOLoadTest1_out.pngを生成する。これは左目用画像である。

2枚目の画像の位置までシークしてからデコーダーに与えると
このプログラムもこのソースコードにプリプロセッサ処理を用いて実装してある。こちらのプログラムは右目用画像を生成する。ビルドするには次の通りのコマンドが必要。

```ps1
cl.exe .\MPOLoadTest1.cpp user32.lib gdi32.lib /DSECOND_IMAGE /Fe:MPOLoadTest1_2.ex
```

（VSCodeを使っているならエディタを左右分割して2枚の画像を並べると比較しやすい）
