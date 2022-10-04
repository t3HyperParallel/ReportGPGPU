# nvJPEGの実験用ディレクトリ

## 準備

### CUDA-ToolKitをねじ込むためにやったこと

+ CUDA-Toolkitをインストールした
    言うまでもない
+ インクルードパスを通す（環境変数）
    環境変数を弄りたくなかったのでLaunch-VsDevShell.ps1を実行した後に$env:INCLUDEにCUDAToolkitのincludeフォルダを追加した
+ インクルードパスを通す（VSCode）
    C++拡張機能の設定に追加

## 参考にした

+ <https://github.com/NVIDIA/CUDALibrarySamples/tree/master/nvJPEG/nvJPEG-Decoder>
