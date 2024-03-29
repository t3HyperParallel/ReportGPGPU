# Media Foundation と Media Session

Media Foundationのメディアセッションの使用方法についてダイアグラムでまとめてある（途中）

## 概要

参考: <https://learn.microsoft.com/ja-jp/windows/win32/medfound/overview-of-the-media-foundation-architecture>

Media Foundationは動画・音声を取り扱う仕組みであり、そのアーキテクチャは2種類ある。
いずれもコーデックや変換などの処理をメディアソースからメディアシンクまでのパイプラインとしてモデル化ていて、アプリケーションの外部にメディアセッションを構築してアプリケーションから制御するものとアプリケーションをデータフローの一部にするものがあるが、ここでは前者を取り扱う。

メディアソース、メディアシンク、その間にあるMF Transform(MFT)からなる
トポロジを生成してセッションに登録する。

MF変換にはDirect3D9/11/12連携機能が用意されている。
今回はDirect3D11を用いる。

参考: <https://learn.microsoft.com/ja-jp/windows/win32/medfound/supporting-direct3d-11-video-decoding-in-media-foundation>

mermaid sequenceDiagramには生成/消失が存在しないのでご容赦
例外処理などは省いている

```mermaid
flowchart TD
    start_MF["Media Foundationの起動"]
    -->create_src["動画ファイルからメディアソースを作成"]
    -->create_topo["メディアソースからトポロジを作成"]
    -->create_session["メディアセッションを作成、トポロジを設定"]
    -->get_event["セッションからイベントを取得"]
    -->use["セッションの使用"]
    -->dispose["開放"]
```

## Media Foundationの初期化・終了

```mermaid
sequenceDiagram
    participant app
    participant mf as Media Foundation
    app->>mf:MFStartUp()
    app->>mf:MFShutDown()
```

## ソースリゾルバーの使用

この段はメディアソースを取得することが目的となる。

等、と書いているのは取得方法が4種類あるからである。

```mermaid
sequenceDiagram
    participant app as アプリケーション
    participant mf as Media Foundation
    participant sr as Source Resolver<br/>:IMFSourceResolver
    participant srcU as メディアソース<br/>:IUnknown
    participant src as メディアソース<br/>:IMFMediaSource
    app->>+mf:MFCreateSourceResolver()
    mf->>sr:生成
    deactivate mf
    app->>+sr:CreateObjectFromURL() 等
    sr->>srcU:生成
    deactivate sr
    app->>+srcU:QueryInterface()
    srcU->>src:取得
    deactivate srcU

    app-)sr:Release()
    app-)srcU:Release()
```

## 再生トポロジの作成

この段はトポロジの作成が目的となる。

```mermaid
sequenceDiagram
    participant app as アプリケーション
    participant mf as Media Foundation
    participant topo as 再生トポロジ<br/>:IMFTopology
    participant node as ソースノード<br/>:IMFTopologyNode<br/>（一時）
    participant out as 出力ノード<br/>:IMFTopologyNode<br/>（一時）
    participant src as メディアソース<br/>:IMFMediaSource<br/>（継続）
    participant pd as プレゼンテーション記述子<br/>:IMFPresentationDescriptor
    participant sd as ストリーム記述子<br/>:IMFStreamDescriptor<br/>（一時）
    participant mth as メディアタイプハンドラ<br/>:IMFMediaTypeHandler<br/>（一時）
    participant act as オーディオ/ビデオ<br/>レンダラーアクティベート<br/>:IMFActivate<br/>（一時）
    app->>+mf:MFCreateTopology()
    mf->>topo:生成
    deactivate mf
    app->>topo:GetStreamDescriptorCount()
    app->>+src:CreateIMFPresentationDescriptor()
    src->>pd:生成
    deactivate src
    app->>+pd:GetStreamDescriptorCount()
    pd-->>-app:記述子の数を代入
    loop 全ての記述子インデックス（0から記述子の数-1まで）について
        app->>+pd:GetStreamDescriptorByIndex()
        pd->>sd:生成
        deactivate pd
        opt *pfSelected==true
            app->>+sd:GetMediaTypeHandler
            sd->>mth:生成
            deactivate sd
            app->>+mth:GetMajorType()
            mth-->>-app:ストリームメジャー型を代入
            app->>mth:Release()
            alt ストリームメジャー型 == MFMediaType_Audio
                app->>+mf:MFCreateAudioRendererActivate()
                mf->>act:生成
                deactivate mf
            else ストリームメジャー型 == MFMediaType_Video
                app->>+mf:MFCreateVideoRendererActivate()
                mf->>act:生成
                deactivate mf
            end
            app->>+mf:MFCreateTopologyNode()
            mf->>node:生成
            deactivate mf
            app->>+node:SetUnknown()
            node-->src:参照
            deactivate node
            app->>+node:SetUnknown()
            node-->pd:参照
            deactivate node
            app->>+node:SetUnknown()
            node-->sd:参照
            deactivate node
            app->>+topo:AddNode()
            topo-->node:参照
            deactivate topo

            app->>+mf:MFCreateTopologyNode()
            mf->>out:生成
            deactivate mf
            app->>+out:SetObject()
            out-->act:参照
            deactivate out
            app->>+topo:AddNode()
            topo-->out:参照
            deactivate topo
            opt 推奨
                app->>out:SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, TRUE)
            end
            app->>+node:ConnectOutput()
            node-->out:参照
            deactivate node
            app->>act:Release()
            app->>node:Release()
            app->>out:Release()
        end
        app->>sd:Release()
    end
```
