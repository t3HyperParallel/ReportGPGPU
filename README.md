# ReportGPGPU

このページは書きかけです
半分は間違いが入ってます

## やること

このリポジトリ自体は教科書の体ににするつもり

+ GPGPUに関する説明（超簡単に）
+ 開発システムの導入の説明
+ GPUデバイスコードプログラミングについて
+ インターレース画像の生成の実装 !最優先
  + 画像をロード・転送
  WICで画像を転送
  + デバイスコードで処理
  CUDA D3D11 Interoperability
  + 表示
  Direct3D? Direct2D?
  + ストレージに出力
  WIC
  
## フローチャート

DirectX各インターフェイスのバージョンは適宜読み替え

### おまじない（初期化）

```mermaid
flowchart TD
  cuInit[["cuInit"]]
```

### WICでの画像の展開

WICはファイルを生データ(BYTE\[\])に展開するまでを担当する。

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
  
  IDXGIFactory-->D3D11CreateDeviceAndSwapChain[[D3D11CreateDeviceAndSwapChain]]-->ID3D11Device[/"ID3D11Device"/] & IDXGISwapChain[/"IDXGISwapChain"/]
  
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

