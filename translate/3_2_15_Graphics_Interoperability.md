# 3. Programming Interface

## 3.2. CUDA Runtime

### 3.2.15. Graphics Interoperability

> Some resources from OpenGL and Direct3D may be mapped into the address space of CUDA, either to enable CUDA to read data written by OpenGL or Direct3D, or to enable CUDA to write data for consumption by OpenGL or Direct3D.

> A resource must be registered to CUDA before it can be mapped using the functions mentioned in OpenGL Interoperability and Direct3D Interoperability. These functions return a pointer to a CUDA graphics resource of type struct cudaGraphicsResource. Registering a resource is potentially high-overhead and therefore typically called only once per resource. A CUDA graphics resource is unregistered using cudaGraphicsUnregisterResource(). Each CUDA context which intends to use the resource is required to register it separately.

> Once a resource is registered to CUDA, it can be mapped and unmapped as many times as necessary using cudaGraphicsMapResources() and cudaGraphicsUnmapResources(). cudaGraphicsResourceSetMapFlags() can be called to specify usage hints (write-only, read-only) that the CUDA driver can use to optimize resource management.

> A mapped resource can be read from or written to by kernels using the device memory address returned by cudaGraphicsResourceGetMappedPointer() for buffers and cudaGraphicsSubResourceGetMappedArray() for CUDA arrays.

> Accessing a resource through OpenGL, Direct3D, or another CUDA context while it is mapped produces undefined results. OpenGL Interoperability and Direct3D Interoperability give specifics for each graphics API and some code samples. SLI Interoperability gives specifics for when the system is in SLI mode.

#### 3.2.15.1. OpenGL Interoperability

（略）

#### 3.2.15.2. Direct3D Interoperability

> Direct3D interoperability is supported for Direct3D 9Ex, Direct3D 10, and Direct3D 11.

> A CUDA context may interoperate only with Direct3D devices that fulfill the following criteria: Direct3D 9Ex devices must be created with DeviceType set to D3DDEVTYPE_HAL and BehaviorFlags with the D3DCREATE_HARDWARE_VERTEXPROCESSING flag; Direct3D 10 and Direct3D 11 devices must be created with DriverType set to D3D_DRIVER_TYPE_HARDWARE.

> The Direct3D resources that may be mapped into the address space of CUDA are Direct3D buffers, textures, and surfaces. These resources are registered using cudaGraphicsD3D9RegisterResource(), cudaGraphicsD3D10RegisterResource(), and cudaGraphicsD3D11RegisterResource().

> The following code sample uses a kernel to dynamically modify a 2D width x height grid of vertices stored in a vertex buffer object.

##### 3.2.15.2.1. Direct3D 9 Version

##### 3.2.15.2.2. Direct3D 10 Version

##### [3.2.15.2.3. Direct3D 11 Version](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#direct3d-11-version)

（コード例につき本文参照）
