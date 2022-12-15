# サンプル集2

## コンパイルするには

こちらのコード郡では色々使うものが増えている。

### ATL / MFC

この章ではComPtrの代わりにCComPtrを使用している。
CComPtrの使用にこれらのライブラリが必要。

ATL/MFCは標準設定ではインストールされないと思われる。されていない場合、VisualStudio Installerから個別でインストールする。
「インストールの詳細」内のチェックボックスで選択可能。

（CComPtrはATLの一部であるが、使用にMFCのインストールが必要であるかは確認していない）

コンパイラには自動で追加される。

### DirectX SDK / Windows SDK

（コンピュータへのインストールについては割愛）

DirectX SDKはWindows SDKに含まれている（旧来は個別だったらしい）

DirectXに関連するヘッダファイルはVisual Studio Build Toolsに含まれるが、参照する静的ライブラリはDirectX SDKに含まれる

この章ではx86_64版Windows SDKの 10.0.22621.0 を使用している
（デフォルトのインストール先は Program files(x86) ）

コンパイラには自動で追加される。

### CUDA Driver API / NVIDIA GPU Computing Toolkit

（コンピュータへのインストールについては割愛）

この章では v11.8 を使用している。この場合、
ヘッダファイルは NVIDIA GPU Computing Toolkit/CUDA/v11.8/include 、
静的ライブラリは NVIDIA GPU Computing Toolkit/CUDA/v11.8/lib/
に存在する。

コンパイラにこれらを追加する方法は複数あるが、筆者はLaunch-VsDevShell.ps1に倣い、コマンドラインに対し一時的に環境変数を追加する方法をとっている。
Toolkitインストール時に環境変数 CUDA_PATH に適当なバージョンのディレクトリが設定されるので、これを利用する。
筆者環境では次の通り。

```ps1
$env:INCLUDE+=";$env:CUDA_PATH\include"
$env:LIB+=";$env:CUDA_PATH\lib\x64"
```

## サンプル

### DisplayTest1.cpp

ウィンドウを出すテスト

### InterOpTest1.cpp

IDXGIFactory::EnumAdaptersとcuD3D11GetDeviceを使用するテスト
DXGIアダプタとCUDAデバイスの数を表示する。

IDXGIFactory::EnumAdaptersはアダプタ番号に対応するディスプレイアダプタを示すIDXGIAdapterを返す。アダプタ番号を0から順に列挙して使用することができる。

cuD3D11GetDeviceはIDXGIAdapterに対応するDXGIアダプタがCUDAデバイスであるかを調べ、そうであるなら対応するCUdeviceを与える。

Windows8以降では通常アダプタ0はMicrosoft Basic Render Driverであり、1以降にビデオカードが配置されている。
したがって、Windows11でiGPUとGeForce GTX 1650 Tiが搭載されている筆者環境ではDXGIデバイスは3つ、CUDAデバイスは1つとなる。

### InterOpTest2.cpp

CUDA Direct3D11 Interoperabilityを使用するテスト

CUDAの標準ライブラリにはGraphics Interoperabilityと称しグラフィクスAPIとの連携機能としてDirect3D9/10/11のTexture1D/2D/3DとBuffer、あるいはOpenGLのバッファへの参照をマップしCUDAカーネルで利用できる機能がある。

（詳細な使用の流れはリポジトリのトップディレクトリにある README.md に記載したグラフを参照のこと）
要点を挙げると、

+ CUDA APIで正常に扱えるテクスチャには制約がある。
+ グラフィクスAPIのリソースは事前にCUDA API側に登録することで参照を取得できる。
+ ID3D11Texture1D/2D/3DからはCUdevptrを取得できず、CUarrayとして参照を取得する。

マップしたリソースはカーネルでの処理が終わる度（即ち毎フレーム）cuGraphicsUnmapResourcesをかける必要がある。
しかし、リソースの登録自体は毎フレーム呼び出してはいけない。

このコードではD3D11Texture2Dに対応するCUgraphicsResourceを取得、テクスチャに対応するCUarrayを取得、cuMemcpy2Dを使ってスワップチェーンのバッファに書き込みを行う。
（取り込んだサンプル画像のフォーマットが間違っていることに起因する画像の乱れがあるが、CUDA APIからスワップチェーンのバッファに書き込みが行えることを十分示せているので放置してある。）
