# 17. Driver API

（略）

## 17.1. Context

> A CUDA context is analogous to a CPU process. All resources and actions performed within the driver API are encapsulated inside a CUDA context, and the system automatically cleans up these resources when the context is destroyed. Besides objects such as modules and texture or surface references, each context has its own distinct address space. As a result, CUdeviceptr values from different contexts reference different memory locations.

CUDA contextはCPUプロセスに類似している。driver APIにおける全てのリソースと操作はCUDA context内にカプセル化されて処理され、コンテキストが破棄されるとき自動でリソースをクリーンアップする。モジュール、テクスチャ・サーフェス参照などのオブジェクトについても、各コンテキストは区別されたアドレス空間をもつ。結果、別のコンテキストに由来するCUdeviceptr値は別のメモリ空間を参照する。

> A host thread may have only one device context current at a time. When a context is created with cuCtxCreate(), it is made current to the calling host thread. CUDA functions that operate in a context (most functions that do not involve device enumeration or context management) will return CUDA_ERROR_INVALID_CONTEXT if a valid context is not current to the thread.

一つのホストスレッドは一度に唯一のdevice context currentをもつ。コンテキストがcuCtxCreate()によって生成されたとき、current（訳注：多分current contextの事）を呼び出し元スレッドに作る。それらしいコンテキストがスレッドにcurrentでない場合、コンテキスト内で処理されるCUDA関数（デバイス列挙かコンテキスト管理を含まないほとんどの関数）はCUDA_ERROR_INVALID_CONTEXTを返却する。

> Each host thread has a stack of current contexts. cuCtxCreate() pushes the new context onto the top of the stack. cuCtxPopCurrent() may be called to detach the context from the host thread. The context is then “floating” and may be pushed as the current context for any host thread. cuCtxPopCurrent() also restores the previous current context, if any.

各ホストスレッドはcurrent contextsのスタックを一つもつ。cuCtxCreateは新しいコンテキストをスタックの一番上にpushする。cuCtxPopCurrentはホストスレッドからコンテキストをdetachする場合に呼び出される。コンテキストは"floating"なら任意のホストスレッドにpushされcurrent contextとなる。
（訳注：つまりスタックの一番上のコンテキストがcurrent contextであるということ？）

> A usage count is also maintained for each context. cuCtxCreate() creates a context with a usage count of 1. cuCtxAttach() increments the usage count and cuCtxDetach() decrements it. A context is destroyed when the usage count goes to 0 when calling cuCtxDetach() or cuCtxDestroy().

usage countも同様にコンテキストごとに一つ管理される。cuCtxCreateはusage countが1の状態でコンテキストを生成する。cuCtxAttachはusage countをインクリメントし、cuCtxDetachはデクリメントする。cuCtxDetachかcuCtxDestroyの呼び出しによってusage countが0になったコンテキストは破棄される。

> The driver API is interoperable with the runtime and it is possible to access the primary context (see Initialization) managed by the runtime from the driver API via cuDevicePrimaryCtxRetain().

driver APIはruntime APIとinteroperableであり、runtime APIが管理するprimary contextはcuDevicePrimaryCtxRetainを経由してdriver APIからアクセス可能である。

> Usage count facilitates interoperability between third party authored code operating in the same context. For example, if three libraries are loaded to use the same context, each library would call cuCtxAttach() to increment the usage count and cuCtxDetach() to decrement the usage count when the library is done using the context. For most libraries, it is expected that the application will have created a context before loading or initializing the library; that way, the application can create the context using its own heuristics, and the library simply operates on the context handed to it. Libraries that wish to create their own contexts - unbeknownst to their API clients who may or may not have created contexts of their own - would use cuCtxPushCurrent() and cuCtxPopCurrent() as illustrated in the following figure.

（訳注：大事じゃなさそうなので略）

## 17.2. Module

## 17.3. Kernels

（わかってるので略）

## 17.4. Interoperability between Runtime and Driver APIs

（大事そうじゃないので略）

## 17.5. Driver Entry Point Access

> The Driver Entry Point Access APIs provide a way to retrieve the address of a CUDA driver function. Starting from CUDA 11.3, users can call into available CUDA driver APIs using function pointers obtained from these APIs.

（略）はCUDA driver functionのアドレスを取得する手段を提供する。CUDA 11.3から提供が開始され、ユーザーはこれらのAPIから得られる関数ポインタを使用して使用可能なCUDA driver APIsを呼び出すことができる

> These APIs provide functionality similar to their counterparts, dlsym on POSIX platforms and GetProcAddress on Windows. The provided APIs will let users:
>
> + Retrieve the address of a driver function using the CUDA Driver API.
> + Retrieve the address of a driver function using the CUDA Runtime API.
> + Request per-thread default stream version of a CUDA driver function. For more details, > see Retrieve per-thread default stream versions
> + Access new CUDA features on older toolkits but with a newer driver.

### 17.5.2. Driver Function Typedefs

> To help retrieve the CUDA Driver API entry points, the CUDA Toolkit provides access to headers containing the function pointer definitions for all CUDA driver APIs. These headers are installed with the CUDA Toolkit and are made available in the toolkit’s include/ directory. The table below summarizes the header files containing the typedefs for each CUDA API header file.
