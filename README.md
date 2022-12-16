# ReportGPGPU

このページは書きかけです
半分は間違いが入ってます

nvJPEGはTesla用のライブラリらしいので多分使えない
TeslaはHPC専用で映像出せないからあっても無意味

## やること

このリポジトリ自体は教科書の体ににするつもり

+ GPGPUに関する説明（超簡単に）
+ 開発システムの導入の説明
+ GPUデバイスコードプログラミングについて
+ 資料の翻訳
  + 特にNVIDIAの回りくどい説明に手を入れる
+ **動画をGPU内部で再生して、それに手を加えられることを示す**
+ インターレース画像の生成の実装をデバイス版とホスト版で比較
  + 画像をロード・転送
  静止画はWIC、動画はDirectShow
  + ピクセル構造体のフォーマット変換はGPU上の方が早い？
  + 画像処理
  デバイス版はカーネルコード、ホスト版はホストコードorHLSL
  + 表示
  CUDA D3D11 Interoperability
  + ストレージに出力
  WIC
+ 速度の比較
  + その他の形式で、カーネルとホストで速度比較
  + CPUからGPU負荷を分散させるプログラム
+ どんな画像処理をGPUでやったら早いか

## 速度の比較

### ~~nvJPEG vs WICを含めた比較~~

1. デバイスでJPEGの展開を行う
  1.1. nvJPEGでデバイスメモリへ展開
  1.2. CUDAカーネルで画像を処理
  1.3. CUDA Direct3D11 Interoperabilityで画像をDirect3D11預かりに
  1.4. Direct3D11で画面に表示
2. ホストでJPEGの展開を行う
  1.1. WICで展開を行う
  1.2. デバイスで画面を処理
  1.3. Direct3D11でデバイスへ転送
  1.4. Direct3D11で画面に表示

~~NVIDIAが公表しているnvJPEGの性能は多数のJPEGファイルを連続で展開するベンチマークであるため、画面に表示しきれる程度の量の画像では高速化に貢献しづらいのではないか。~~
nvJPEGはTesla用ライブラリなので画面表示について考慮するのはほぼ無意味

## フローチャート

DirectX各インターフェイスのバージョンは適宜読み替え

### おまじない（初期化）

```mermaid
flowchart TD
  CoInitialize[[CoInitialize]]
  cuInit[["cuInit"]]
```

### WICでの画像の展開

WICはファイルを生データ(BYTE\[\])に展開するまでを担当する。<br/>
WICはMPOに非対応である為、自前で解析してJpegにしてからDecoderに渡す必要がある。そのためのライブラリもこのプロジェクトに用意した。

ピクセルフォーマットの変換はGPUで行った方が早い可能性あり。
特に24bitカラーを32bitカラーに変換する場合、変換をGPUで行えば転送すべき画像データが3/4のサイズとなる。

```mermaid
graph TD
  CoCreateInstance[["CoCreateInstance or<br/>CoCreateInstanceEx"]]
  -->IWICImagingFactory[/"IWICImagingFactory"/]
  ---IWICImagingFactory.CreateDecoderFromFileName[["CreateDecodeFromFileName"]]
  -->IWICBitmapDecoder[/"IWICBitmapDecoder"/]
  ---IWICBitmapDecoder.GetFrame[["GetFrame"]]
  -->IWICBitmapFrameDecode[/"IWICBitmapFrameDecode"/]
  --"暗黙キャスト"--->IWICBitmapSource[/"IWICBitmapSource"/]

  IWICBitmapSource
  ---IWICBitmapSource.GetSize[["GetSize"]]
  -->size[/"画像のサイズ"/]
  IWICBitmapSource
  ---IWICBitmapSource.CopyPixels[["CopyPixels"]]
  size & IWICBitmapSource.CopyPixels-->raw[/"画像の生データ(BYTE[])"/]

  IWICImagingFactory
  ----IWICImagingFactory.CreateFormatConverter[["CreateFormatConverter"]]
  -->unusedIWICFormatConverter["IWICFormatConverter<br/>（起動前）"]
  ---IWICFormatConverter.Init[["Init"]]
  -->convertedIWICFormatConverter["IWICFormatConverter<br/>（コンバート完了）"]
  --"暗黙キャスト"-->IWICBitmapSource
  IWICBitmapFrameDecode
  -."ピクセルフォーマット<br/>変換する場合".->IWICFormatConverter.Init

  subgraph WICを用いて<br/>フォーマット変換を行う
    IWICImagingFactory.CreateFormatConverter
    unusedIWICFormatConverter
    IWICFormatConverter.Init
    convertedIWICFormatConverter
  end
```

### 初期化、VRAMへの画像のロード

> Do not mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1 (IDXGIFactory1) in an application.

```mermaid
flowchart TD
  CreateDXGIFactory[[CreateDXGIFactory]]
  -->IDXGIFactory[/"IDXGIFactory"/]
  -->EnumWarpAdapter_cuD3D11GetDevice["cuD3D11GetDeviceをチェックしながら<br/>EnumWarpAdapter"]
  -->IDXGIAdapter[/"IDXGIAdapter"/]
  ---D3D11CreateDeviceAndSwapChain[[D3D11CreateDeviceAndSwapChain]]

  EnumWarpAdapter_cuD3D11GetDevice
  -->CUdevice[/"CUdevice"/]
  -->cuCtxCreate[["cuCtxCreate"]]
  -->CUcontext[/"CUcontext"/]

  RegisterClass[/"RegisterClassした<br/>WNDCLASS"/]
  -->CreateWindow[["CreateWindow"]]
  -->HWND[/"ウィンドウ<br/>(HWND)"/]
  -->DXGI_SWAP_CHAIN_DESC[/"DXGI_SWAP_CHAIN_DESC"/]
  imageSize[/"画像のサイズ"/]
  -->windowSize[/"ウィンドウサイズ"/]
  -->rtSize[/"バッファサイズ<br/>（後述）"/]
  windowSize-->CreateWindow
  rtSize-->DXGI_SWAP_CHAIN_DESC

  DXGI_SWAP_CHAIN_DESC
  -->D3D11CreateDeviceAndSwapChain
  -->ID3D11Device[/"ID3D11Device"/] & IDXGISwapChain[/"IDXGISwapChain"/]

  ID3D11Device
  ---ID3D11Device.CreateTexture2D[["CreateTexture2D"]]
  rawImage[/"画像データ(BYTE[])<br/>及びサイズ"/] & ID3D11Device.CreateTexture2D
  -->ID3D11Texture2D[/"VRAM上の画像<br/>(ID3D11Texture2D)"/]
  -->cuGraphicsD3D11RegisterResource_image[["cuGraphicsD3D11RegisterResource"]]
  -->sourceCUgraphicsResource[/"VRAM上の画像<br/>(CUgraphicsResource)"/]

  IDXGISwapChain
  ---IDXGISwapChain.GetBuffer[["GetBufferで適当なバッファの参照を取得"]]
  -->buffers[/"バッファ<br/>(ID3D11Texture2D)"/]
  -->cuGraphicsD3D11RegisterResource_target[["cuGraphicsD3D11RegisterResource"]]
  -->targetCUgraphicsResource[/"バッファ<br/>(CUgraphicsResource)"/]

  subgraph "コンテキストの処理"
    cuCtxCreate
  end

  subgraph "デバイス下の処理なのでコンテキストの処理を先にやってから" 
    cuGraphicsD3D11RegisterResource_image
    cuGraphicsD3D11RegisterResource_target
  end
```

#### ウィンドウサイズとバッファサイズについて

バッファサイズがレンダーターゲットウィンドウと異なる場合、表示の際に自動でフィットするように変形される。

### CUDA Driver APIでのptxの実行方法

cuModuleLoadExはJITに関する設定を付与できる<br/>
cuLaunchKernelExは矩形情報を構造体で受け取るので同じ設定を使い回す際に有利

```mermaid
flowchart TD
  ptx[/"ptxファイル"/]-->cuModuleLoad[["ファイル名を指定して<br/>cuModuleLoad"]]
  cuModuleLoad-->CUmodule[/"CUmodule"/]
  ptx-."あるいは".->getBinaryImage[["ファイルをメモリに展開"]]
  getBinaryImage-.->cuModuleLoadData[["cuModuleLoadDataもしくは<br/>cuModuleLoadDataEx"]]
  cuModuleLoadData-.->CUmodule

  CUmodule-->cuModuleGetFunction[["関数名を指定して<br/>cuModuleGetFunction"]]
  cuModuleGetFunction-->CUfunction[/"CUfunction"/]
  args[/"カーネルの引数とする<br/>デバイスポインタの配列"/]
  g_tb[/"gridとthread blockのサイズ"/]
  CUfunction & args & g_tb-->cuLaunchKernel[["cuLaunchKernelもしくは<br/>cuLaunchKernelEx"]]
```

### リソースのCUDAでの使用方法

Texture2Dに相当するcuArray間のコピーにはcuMemcpy2Dを使用しなければならない

```mermaid
flowchart TD
  CUgraphicsResource[/"CUgraphicsResource"/]
  -->cuGraphicsMapResources[["cuGraphicsMapResources"]]
  <-->cuGraphicsUnmapResources[[cuGraphicsUnmapResources]]

  CUgraphicsResource
  -."ID3D11Bufferにリンク".->
    cuGraphicsResourceGetMappedPointer[[cuGraphicsGetMappedPointer]]
  -->CUdeviceptr[/"CUdeviceptr"/]

  CUgraphicsResource
  -."ID3D11Texture1D/2D/3Dにリンク".->
    cuGraphicsResourceGetMappedMipmappedArray[["cuGraphicsResourceGetMappedMipmappedArray"]]
  -->CUmipmappedArray[/"CUmipmappedArray"/]

  subgraph "マップ中、グラフィックスAPIはこの間リソースにアクセスできない"
    cuGraphicsResourceGetMappedPointer
    CUdeviceptr
    cuGraphicsResourceGetMappedMipmappedArray
    CUmipmappedArray
  end
```

### 処理内容の反映（画面への表示）

IDXGISwapChain::Presentを呼び出せばよい。
