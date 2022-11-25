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
+ インターレース画像の生成の実装をデバイス版とホスト版で比較 !最優先
  + 画像をロード・転送
  WIC
  + 画像処理
  デバイス版はカーネルコード、ホスト版はホストコード
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
WICはMPOに非対応まｍｐで、自前で解析してJpegにしてからDecoderに渡す必要がある。そのためのライブラリを用意した。

```mermaid
graph TD
  IWICImagingFactory[/"IWICImagingFactory"/]---IWICImagingFactory.CreateDecoderFromFileName[["CreateDecodeFromFileName"]]
  IWICImagingFactory.CreateDecoderFromFileName-->IWICBitmapDecoder[/"IWICBitmapDecoder"/]
  IWICBitmapDecoder---IWICBitmapDecoder.GetFrame[["GetFrame"]]
  IWICBitmapDecoder.GetFrame-->IWICBitmapFrameDecode[/"IWICBitmapFrameDecode"/]
  IWICBitmapFrameDecode---IWICBitmapSource.GetSize[["GetSize"]]
  IWICBitmapSource.GetSize--->size[/"画像のサイズ"/]
  IWICBitmapFrameDecode---IWICBitmapSource.CopyPixels[["CopyPixels"]]
  size & IWICBitmapSource.CopyPixels-->raw[/"画像の生データ(BYTE[])"/]
  
  IWICImagingFactory-.-IWICImagingFactory.CreateFormatConverter[["CreateFormatConverter"]]
  IWICImagingFactory.CreateFormatConverter-.->unusedIWICFormatConverter["IWICFormatConverter<br/>（起動前）"]
  unusedIWICFormatConverter-.-IWICFormatConverter.Init[["Init"]]
  IWICBitmapFrameDecode-.->|フォーマットの変換が<br/>必要な場合|convertedIWICFormatConverter
  IWICFormatConverter.Init-.->convertedIWICFormatConverter["IWICFormatConverter<br/>（コンバート完了）"]
  convertedIWICFormatConverter-.-IWICBitmapSource.GetSize & IWICBitmapSource.CopyPixels
```

### 初期化、VRAMへの画像のロード

> Do not mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1 (IDXGIFactory1) in an application.

```mermaid
graph TD
  CreateDXGIFactory[[CreateDXGIFactory]]-->IDXGIFactory[/"IDXGIFactory"/]
  
  IDXGIFactory-->EnumWarpAdapter["cuD3DGetDeviceをチェックしながら<br/>EnumWarpAdapter"]-->CUdevice[/"CUdevice"/] & IDXGIAdapter[/"IDXGIAdapter"/]
  IDXGIAdapter-->ID3D11Device
  
  IDXGIAdapter-->D3D11CreateDeviceAndSwapChain[[D3D11CreateDeviceAndSwapChain]]-->ID3D11Device[/"ID3D11Device"/] & IDXGISwapChain[/"IDXGISwapChain"/]
  
  RegisterClass["RegisterClassした<br/>WNDCLASS"]-->CreateWindow
  imageSize[/"WICで取得した画像のサイズ<br/>(UINT,UINT)"/]-->calcImageSize["処理後の画像サイズを計算"]
  calcImageSize-->CreateWindow[["CreateWindow"]]-->HWND[/"ウィンドウ<br/>(HWND)"/]
  calcImageSize & HWND-->DXGI_SWAP_CHAIN_DESC[/"DXGI_SWAP_CHAIN_DESC"/]-->IDXGISwapChain
  
  ID3D11Device[/"ID3D11Device"/]-->ID3D11Device.CreateTexture2D[["CreateTexture2D"]]
  rawImage[/"WICで取得した画像データとサイズ<br/>(BYTE[],UINT,UINT)"/]
  rawImage & ID3D11Device.CreateTexture2D-->ID3D11Texture2D[/"VRAM上の画像<br/>(ID3D11Texture2D)"/]
  ID3D11Texture2D & cuGraphicsD3D11RegisterResource[["cuGraphicsD3D11RegisterResource"]]-->sourceCUgraphicsResource[/"VRAM上の画像<br/>(CUgraphicsResource)"/]
  
  IDXGISwapChain-->IDXGISwapChain.GetBuffer[["GetBufferで全てのバッファの参照を取得"]]-->buffers[/"バッファ<br/>(ID3D11Texture2D)"/]
  cuGraphicsD3D11RegisterResource & buffers-->targetCUgraphicsResource[/"バッファ<br/>(CUgraphicsResource)"/]
```

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

```mermaid
flowchart TD
  cu_registerResource["リソースを登録してCUgraphicsResourceを取得<br/>(cuGraphicsD3D11RegisterResourceなど)"]
  subgraph kernel
    kernelStart[/"処理開始"\]
    cuGraphicsMapResources[["cuGraphicsMapResources"]]
    cuGraphicsResourceGetMappedPointer[["cuGraphicsResourceGetMappedPointerでCUdeviceptrを取得"]]
    doKernel["カーネル処理を実行"]
    cuGraphicsUnmapResources[["cuGraphicsUnmapResources"]]
    kernelEnd[\"処理終了"/]
  end
  cuGraphicsUnregisterResource[["cuGraphicsUnregisterResource"]]
  
  cu_registerResource-->kernel-->cuGraphicsUnregisterResource
  kernelStart-->cuGraphicsMapResources-->cuGraphicsResourceGetMappedPointer-->doKernel-->cuGraphicsUnmapResources-->kernelEnd
```

### 表示処理

```mermaid
flowchart TD
  drawStart[/"開始"\]-->kernel["画像を処理してバッファに書き込み"]
  kernel-->present[["IDXGISwapChain.Present"]]
  present-->drawEnd[\"終了"/]
```

### メインメモリへのデータの返却、WICでのエンコード

WICは解像度情報も扱える

