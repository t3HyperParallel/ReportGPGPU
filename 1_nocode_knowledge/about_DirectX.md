## COM (Component Object Model)

APIをインスタンスとし、アプリケーション-インスタンス間でクエリを通しやりとりする仕組み<br/>
古来よりWindowsの随所で使用されていたが、現在ではDirectXで使用されるものとされることが多い

勘違いされやすいが、各言語・ランタイムのもつオブジェクト指向の型システムとは独立した概念である

### COMのインターフェース

COMはオブジェクト指向におけるインターフェースの概念をもち、COMオブジェクトはCOMインターフェースを実装している。
インターフェースであるが、COMで実装されたAPIを利用する側がCOMオブジェクトのインスタンスを直接生成することはないので、事実上インスタンス自体の直接の型である

### IUnknown

COMの基底となるインターフェース
（全てのCOMインターフェースはIUnknownを継承している）

IUnknown自体が参照カウンタを実装することを要求しているため、全てのCOMオブジェクトは共有スマートポインタで管理されることになる。<br/>
スマートポインタ機能のC++向けのラッパーとしてComPtrが用意されている。

## DXGI (DirectX Graphic Infrastructure)

> Microsoft DirectX Graphics Infrastructure (DXGI) handles enumerating graphics adapters, enumerating display modes, selecting buffer formats, sharing resources between processes (such as, between applications and the Desktop Window Manager (DWM)), and presenting rendered frames to a window or monitor for display.

> ough most graphics programming is done using Direct3D, you can use DXGI to present frames to a window, monitor, or other graphics component for eventual composition and display. You can also use DXGI to read the contents on a monitor.

(引用元： https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dx-graphics-dxgi)

+ カーネルモードのグラフィックドライバーとの通信を行う
+ 単一のグラフィックスリソースを複数のプロセスから利用できるようにする
+ ウィンドウやモニターにフレームを表示する
+ グラフィックスハードウェア（グラフィックスカードやモニター）を抽象化したCOMオブジェクトを提供する
+ 各機能レベルのD3D/D2Dのリソースを抽象化し相互利用機能を提供する

### IDXGIFactory (COM Interface)

ファクトリ：他のオブジェクトを生成する機能をもつ機能のこと<br/>
DXGIの全ての機能の開始地点

### IDXGIAdapter (COM Interface)

ディスプレイアダプタ：グラフィックスカードあるいはそれを仮想化したワープデバイス

#### ワープデバイス

グラフィックスカードが使用したいDirect3Dの機能レベルを満たしていない場合でもワープデバイスを通せば使用することができる
仮想化されている分ハードウェアの互換性によるエラーを起こしづらいため、対応コストの軽減に有用
またはデバッグのための比較対象として用いることもある

### IDXGISwapChain

スワップチェーン：バックバッファとフロントバッファを差し替える仕組み
（バックバッファリングについては別資料に詳細）

## Direct3D11

### ID3D11Device

Direct3D11が稼働するデバイス。
ひとつのディスプレイアダプタに複数のデバイスがあるらしい
（IDXGIDeviceの説明に"that produce image data"とあるので、グラフィックス処理を行う実体のこと？）

### ID3D11Texture2D

2Dのテクスチャ、つまりビットマップ画像
（2Dでないテクスチャもあるということ 参考： https://docs.microsoft.com/ja-jp/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-intro ）
CUDAのグラフィックスAPI連携機能はこれにアクセスすることができる

#### ピクセルとテクセル

殊3DCG分野ではレンダリング先の画像を「ピクチャ」と呼び、素材画像を「テクスチャ」と呼ぶことがあるようだ
用語「ピクセル」と「テクセル」もこれに準じて使い分けされる

#### ミップマップ

3DCGに於いて、一般的なパースではカメラから遠い程オブジェクトは小さくなり、オブジェクトが描画されるピクセルの数も少なくなる
テクスチャの解像度がどれ程高くても射影先の解像度が低ければ無意味となる
そのため、あるテクスチャに対して解像度を下げたものを事前に用意しておき、カメラからの距離に応じて解像度を下げることによってレンダリング処理を軽量化することができる。


## 参考リンク

+ https://ichigopack.net/win32com/com_base_3.html
IUnknownについて
+ https://docs.microsoft.com/windows/win32/direct3darticles/directx-warp
ワープデバイスについて
+ 
