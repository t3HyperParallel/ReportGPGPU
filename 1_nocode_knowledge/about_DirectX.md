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

## Direct3D12

### ID3D12Device

### ID3D12CommandQueue

デバイスに転送するコマンドを保持するキュー

### ID3D12DescriptionHeap

#### ルートシグネチャ

シェーダーに渡すデータを指定する

#### 記述子

VRAM上のリソースのメタデータ
ルートシグネチャに必要


## 参考リンク

+ https://ichigopack.net/win32com/com_base_3.html
IUnknownについて
+ https://docs.microsoft.com/windows/win32/direct3darticles/directx-warp
ワープデバイスについて
+ 
