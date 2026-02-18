/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPREFETCH_HPP
#define TPREFETCH_HPP

#include "TLoad.hpp"

namespace pto {
template <typename TileData, typename GlobalData>
__tf__ AICORE void TPrefetchDoCopy(typename TileData::TileDType __out__ dstTile,
    typename GlobalData::DType __in__ *srcPtr, uint16_t rowChunk, uint32_t colChunk, int stride3)
{
    const uint16_t nBurst = rowChunk;
    const uint32_t lenBurst = static_cast<uint32_t>(colChunk * sizeof(typename GlobalData::DType));
    const uint32_t gmGap =
        static_cast<uint32_t>((stride3 - static_cast<int>(colChunk)) * sizeof(typename GlobalData::DType));
    const uint32_t ubGap = 0;
    const uint32_t ubPad = 0;
    __ubuf__ typename TileData::DType *dstPtr =
        (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(dstTile);
    TLoadInstrGm2ub<TileData, GlobalData>(dstPtr, srcPtr, nBurst, lenBurst, gmGap, ubGap, ubPad);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TPrefetchCopySlice(TileData &dst, typename GlobalData::DType *basePtr, int s3, int s4, int st3,
    int st4, uint16_t tileRows, uint32_t maxColsPerChunk, bool fits)
{
    if (fits) {
        TPrefetchDoCopy<TileData, GlobalData>(dst.data(), basePtr, static_cast<uint16_t>(s3),
            static_cast<uint32_t>(s4), st3);
        return;
    }

    for (int r = 0; r < s3; r += tileRows) {
        const uint16_t rowChunk = static_cast<uint16_t>((s3 - r) < tileRows ? (s3 - r) : tileRows);
        typename GlobalData::DType *rowPtr = basePtr + r * st3;
        for (int c = 0; c < s4; c += static_cast<int>(maxColsPerChunk)) {
            const uint32_t colChunk = static_cast<uint32_t>((s4 - c) < static_cast<int>(maxColsPerChunk)
                                                              ? (s4 - c)
                                                              : maxColsPerChunk);
            typename GlobalData::DType *srcPtr = rowPtr + c * st4;
            TPrefetchDoCopy<TileData, GlobalData>(dst.data(), srcPtr, rowChunk, colChunk, st3);
        }
    }
}

// Prefetch GlobalTensor into a Vec tile without layout/type checks (dst is temporary)
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TPREFETCH_IMPL(TileData &dst, GlobalData &src)
{
    const uint16_t tileRows = TileData::Rows;
    const uint32_t tileCols = TileData::Cols;
    const std::size_t tileRowBytes = static_cast<std::size_t>(tileCols) * sizeof(typename TileData::DType);
    const std::size_t tileBytes = static_cast<std::size_t>(tileRows) * tileRowBytes;

    const int s0 = src.GetShape(pto::GlobalTensorDim::DIM_0);
    const int s1 = src.GetShape(pto::GlobalTensorDim::DIM_1);
    const int s2 = src.GetShape(pto::GlobalTensorDim::DIM_2);
    const int s3 = src.GetShape(pto::GlobalTensorDim::DIM_3);
    const int s4 = src.GetShape(pto::GlobalTensorDim::DIM_4);

    const int st0 = src.GetStride(pto::GlobalTensorDim::DIM_0);
    const int st1 = src.GetStride(pto::GlobalTensorDim::DIM_1);
    const int st2 = src.GetStride(pto::GlobalTensorDim::DIM_2);
    const int st3 = src.GetStride(pto::GlobalTensorDim::DIM_3);
    const int st4 = src.GetStride(pto::GlobalTensorDim::DIM_4);

    const std::size_t sliceBytes = static_cast<std::size_t>(s3) * s4 * sizeof(typename GlobalData::DType);
    const bool fits = sliceBytes <= tileBytes;
    const uint32_t maxColsPerChunk = static_cast<uint32_t>(tileRowBytes / sizeof(typename GlobalData::DType));

    for (int n0 = 0; n0 < s0; ++n0) {
        for (int n1 = 0; n1 < s1; ++n1) {
            for (int n2 = 0; n2 < s2; ++n2) {
                typename GlobalData::DType *basePtr = src.data() + n0 * st0 + n1 * st1 + n2 * st2;
                TPrefetchCopySlice<TileData, GlobalData>(dst, basePtr, s3, s4, st3, st4, tileRows, maxColsPerChunk,
                    fits);
            }
        }
    }
}
} // namespace pto

#endif
