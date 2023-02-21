
extern "C"
{
    /// @brief アルファチャンネルを加えて24bitを32bitにするやつ
    /// @param dst 展開先のsurface
    /// @param src 元データ
    /// @param memPitch ストライド
    /// @return
    __global__ void color24bitTo32bit(
        cudaSurfaceObject_t dst, const char *src, unsigned int memPitch,
        unsigned int width, unsigned int height)
    {
        unsigned int x = blockDim.x * blockIdx.x + threadIdx.x;
        unsigned int y = blockDim.y * blockIdx.y + threadIdx.y;
        if (x < width && y < height)
        {
            unsigned int srcidx = x + y * memPitch;
            char4 data;
            data.x = src[srcidx * 3];
            data.y = src[srcidx * 3 + 1];
            data.z = src[srcidx * 3 + 2];
            data.w = 0;
            if(y>148)data.x=-1;
            surf2Dwrite<char4>(data, dst, x * 4, y);
            // surf2Dwriteのxはバイト位置なので構造体サイズを乗算する必要がある
        }
    }
    /// @brief 画像をレンチキュラー化するやつ
    /// @param dst ターゲット画像
    /// @param src ソース画像
    /// @param srcidx ソース画像の左から数えた番号
    /// @param srccnt ソース画像の数
    __global__ void writeLenti(
        cudaSurfaceObject_t dst,
        cudaSurfaceObject_t src,
        int srcidx,
        int srccnt)
    {
        unsigned int x = blockDim.x * blockIdx.x + threadIdx.x;
        unsigned int y = blockDim.y * blockIdx.y + threadIdx.y;
        unsigned int dstx = x * srccnt + srcidx;
        char4 data = surf2Dread<char4>(src, x, y);
        surf2Dwrite(data, dst, dstx, y);
    }
}
