/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_PREFETCH_HPP
#define PTO_PREFETCH_HPP

#include <acl/acl.h>
#include <pto/pto-inst.hpp>
#include <pto/npu/a2a3/TLoad.hpp>

namespace pto {

constexpr uint32_t kPtoPrefetchTileBytes = 64U * 1024U;
constexpr uint32_t kPtoPrefetchDefaultBlocks = 20U;

#define PTO_AIV_ATTR __attribute__((aiv))
#define PTO_PREFETCH_DEVICE_ENABLED 1

#if PTO_PREFETCH_DEVICE_ENABLED
namespace detail {
PTO_INTERNAL void PtoPrefetchKernelBody(__gm__ uint8_t *tensor, uint64_t total_elems)
{
    constexpr uint32_t tile_bytes = kPtoPrefetchTileBytes;
    constexpr uint32_t tile_elems = tile_bytes / sizeof(uint8_t);
    static_assert(tile_elems > 0, "tile_elems must be positive");

    using DType = uint8_t;
    using PrefetchTile = Tile<TileType::Vec, DType, 1, tile_elems, BLayout::RowMajor, 1, DYNAMIC>;
    using PrefetchShape = Shape<1, 1, 1, 1, DYNAMIC>;
    using PrefetchStride = Stride<1, 1, 1, DYNAMIC, 1>;

    const uint32_t block_dim = static_cast<uint32_t>(get_block_num());
    if (block_dim == 0)
        return;

    const uint32_t blk = get_block_idx();

    const uint64_t total_tiles = (total_elems + tile_elems - 1ULL) / static_cast<uint64_t>(tile_elems);
    const uint64_t tiles_per_block = (total_tiles + block_dim - 1ULL) / static_cast<uint64_t>(block_dim);

    const uint64_t tile_start = static_cast<uint64_t>(blk) * tiles_per_block;
    if (tile_start >= total_tiles)
        return;
    const uint64_t tile_end = tile_start + tiles_per_block;

    const uint64_t start = tile_start * static_cast<uint64_t>(tile_elems);
    uint64_t end = (tile_end >= total_tiles) ? total_elems : (tile_end * static_cast<uint64_t>(tile_elems));

    for (uint64_t offset = start; offset < end; offset += tile_elems) {
        const uint64_t remaining = end - offset;
        const uint32_t cur_elems = (remaining < tile_elems) ? static_cast<uint32_t>(remaining) : tile_elems;

        PrefetchTile tile(cur_elems);
        TASSIGN(tile, 0u);

        PrefetchShape dyn_shape(1, 1, 1, 1, static_cast<int>(cur_elems));
        PrefetchStride dyn_stride(1, 1, 1, static_cast<int>(cur_elems), 1);
        GlobalTensor<DType, PrefetchShape, PrefetchStride> g(tensor + offset, dyn_shape, dyn_stride);
        TPREFETCH(tile, g);
    }
}
} // namespace detail

// Generic prefetch kernel: split a 1D tensor across blocks (get_blockdim()) and issue TPREFETCH
__global__ AICORE PTO_AIV_ATTR void PTO_PREFETCH_AIV(__gm__ uint8_t *tensor, uint64_t total_elems)
{
    detail::PtoPrefetchKernelBody(tensor, total_elems);
}
#endif // PTO_PREFETCH_DEVICE_ENABLED

// Host wrapper to launch PTO_PREFETCH with bytes input and optional SDMA/AIV core selection.
// Use the template parameters to pick SDMA or AIV and to set aiv_cores for finer control.
template <bool UseSdma = true, int AivCores = -1>
void PTO_PREFETCH(__gm__ void *tensor, uint64_t tensor_bytes, aclrtStream stream)
{
    if (tensor_bytes == 0)
        return;

    if constexpr (UseSdma) {
        aclrtCmoAsync((void *)(uint64_t)tensor, static_cast<size_t>(tensor_bytes), ACL_RT_CMO_TYPE_PREFETCH, stream);
    } else {
        static_assert(AivCores > 0, "AivCores must be > 0 when UseSdma is false");
        PTO_PREFETCH_AIV<<<AivCores, nullptr, stream>>>((__gm__ uint8_t *)tensor, tensor_bytes);
    }
}

} // namespace pto

#endif // PTO_PREFETCH_HPP
