# ReportGPGPU

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

### DXGI/Direct3D11/Direct2Dの初期化、画像のロード

> Do not mix the use of DXGI 1.0 (IDXGIFactory) and DXGI 1.1 (IDXGIFactory1) in an application. 

```mermaid
flowchart TD
  rawdata[/"rawdata(BYTE[])<br/>and metadata"/]
  CreateDXGIFactory[[CreateDXGIFactory]]-->IDXGIFactory[/"IDXGIFactory"/]
  IDXGIFactory-->EnumWarpAdapter[[EnumWarpAdapter<br/>with checking DriverType]]-->IDXGIAdapter[/"IDXGIAdapter"/]
  IDXGIFactory-->D3D11CreateDeviceAndSwapChain[[D3D11CreateDeviceAndSwapChain]]
  D3D11CreateDeviceAndSwapChain & IDXGIAdapter-->ID3D11Device[/"ID3D11Device"/] & ID3D11SwapChain[/"ID3D11SwapChain"/]
  ID3D11Device-->ID3D11Device.CreateTexture2D[[CreateTexture2D]]
  rawdata & ID3D11Device.CreateTexture2D-->ID3D11Texture2D
  
```

```mermaid
graph TD
  ID2D1Device-->|CreateDeviceContext|ID2D1DeviceContext
  ID2D1DeviceContext-->ID2D1DeviceContext.CreateBitmapFromDxgiSurface[[CreateBitmapFromDxgiSurface]]
  ID2D1BitMap
  IDXGISwapChain-->|GetBuffer|ID3D11Texture2D
  ID3D11Texture2D-->|As|IDXGISurface
  
  ID2D1DeviceContext.CreateBitmapFromDxgiSurface & IDXGISurface-->ID2D1BitMap
```
