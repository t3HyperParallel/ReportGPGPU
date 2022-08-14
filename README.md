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
  CUDA D3D11 Interpolability
  + 表示
  Direct3D? Direct2D?
  + ストレージに出力
  DirectXの何かしら?
  WinUIライブラリ?
  
## フローチャート

### おまじない（初期化）

```mermaid
flowchart TD
  cuInit[/"cuInit"/]
```

### WICでの画像の展開

WICはファイルを生データ(BYTE\[\])に展開するまでを担当する。

```mermaid
flowchart TD
  IWICImagingFactory-->|CreateDecoderFromFileName|IWICBitmapDecoder
  IWICBitmapDecoder-->|GetFrame|IWICBitmapFrameDecode
  IWICBitmapFrameDecode-->checkFormat{"fomrat<br/>OK?"}
  checkFormat-->|yes<br/>IWICBitmapSource.CopyPixels|middle
  IWICImagingFactory-.->|CreateFormatConverter|unusedIWICFormatConverter["IWICFormatConverter<br/>(unused)"]
  unusedIWICFormatConverter-->IWICFormatConverter.Init[["Init"]]
  IWICFormatConverter.Init-->convertedIWICFormatConverter["IWICFormatConverter<br/>(converted)"]
  checkFormat-->|no|convertedIWICFormatConverter
  convertedIWICFormatConverter-->|IWICBitmapSource.CopyPixels|middle["raw data(BYTE[])<br/>and metadata"]
```

### 初期化、画像のロード

> Do not mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1 (IDXGIFactory1) in an application.

### DXGIAdapterの選定

```mermaid
flowchart TD
  CreateDXGIFactory[[CreateDXGIFactory]]-->IDXGIFactory[/"IDXGIFactory"/]
  
  IDXGIFactory-->EnumWarpAdapter[[EnumWarpAdapter<br/>with checking cuD3DGetDevice]]-->CUdevice[/"CUdevice"/] & IDXGIAdapter[/"IDXGIAdapter"/]
  IDXGIAdapter-->ID3D11Device
  
  IDXGIFactory-->D3D11CreateDeviceAndSwapChain[[D3D11CreateDeviceAndSwapChain]]-->ID3D11Device[/"ID3D11Device"/] & IDXGISwapChain[/"IDXGISwapChain"/]
  
  imageSize[/"WICで取得した画像のサイズ<br/>(UINT,UINT)"/]-->calcImageSize["処理後の画像サイズを計算"]
  calcImageSize-->CreateWindow[["CreateWindow"]]-->HWND[/"ウィンドウ<br/>(HWND)"/]
  calcImageSize & HWND-->DXGI_SWAP_CHAIN_DESC[/"DXGI_SWAP_CHAIN_DESC"/]-->IDXGISwapChain
  
  ID3D11Device[/"ID3D11Device"/]-->ID3D11Device.CreateTexture2D[["CreateTexture2D"]]
  rawImage[/"WICで取得した画像データとサイズ<br/>(BYTE[],UINT,UINT)"/]
  rawImage & ID3D11Device.CreateTexture2D-->ID3D11Texture2D[/"VRAM上の画像<br/>(ID3D11Texture2D)"/]
  cuGraphicsD3D11RegisterResource[["cuGraphicsD3D11RegisterResource"]] & ID3D11Texture2D-->CUgraphicsResource[/"CUgraphicsResource"/]
```
