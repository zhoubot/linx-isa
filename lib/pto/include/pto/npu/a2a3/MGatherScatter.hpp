/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef MGATHER_SCATTER_NPU_HPP
#define MGATHER_SCATTER_NPU_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <pto/common/constants.hpp>

namespace pto {

template <typename ElemT, typename GmElemT>
PTO_INTERNAL void CopyGmToUb(__ubuf__ ElemT *dst, GmElemT *src, uint32_t bytes)
{
    const uint16_t nBurst = 1;
    const uint32_t lenBurst = bytes;
    const uint32_t gmGap = 0;
    const uint32_t ubGap = 0;
    const uint32_t ubPad = 0;
    if constexpr (sizeof(ElemT) == 1) {
        copy_gm_to_ubuf_align_b8(dst, src, 0, nBurst, lenBurst, 0, ubPad, gmGap, ubGap);
    } else if constexpr (sizeof(ElemT) == 2) {
        copy_gm_to_ubuf_align_b16(dst, src, 0, nBurst, lenBurst, 0, ubPad, gmGap, ubGap);
    } else if constexpr (sizeof(ElemT) == 4) {
        copy_gm_to_ubuf_align_b32(dst, src, 0, nBurst, lenBurst, 0, ubPad, gmGap, ubGap);
    } else if constexpr (sizeof(ElemT) == 8) {
        copy_gm_to_ubuf_align_b32(dst, src, 0, nBurst, lenBurst, 0, ubPad * 2, gmGap, ubGap);
    }
}

template <typename GmElemT, typename ElemT>
PTO_INTERNAL void CopyUbToGm(GmElemT *dst, __ubuf__ ElemT *src, uint32_t bytes)
{
    const uint16_t nBurst = 1;
    const uint32_t lenBurst = bytes;
    const uint32_t gmGap = 0;
    const uint32_t ubGap = 0;
    if constexpr (sizeof(ElemT) == 1) {
        copy_ubuf_to_gm_align_b8(dst, src, 0, nBurst, lenBurst, 0, 0, ubGap, gmGap);
    } else if constexpr (sizeof(ElemT) == 2) {
        copy_ubuf_to_gm_align_b16(dst, src, 0, nBurst, lenBurst, 0, 0, ubGap, gmGap);
    } else if constexpr (sizeof(ElemT) == 4 || sizeof(ElemT) == 8) {
        copy_ubuf_to_gm_align_b32(dst, src, 0, nBurst, lenBurst, 0, 0, ubGap, gmGap);
    }
}

template <typename TileDst, typename TileInd>
__tf__ PTO_INTERNAL void MGatherFromUb(typename TileDst::TileDType __out__ dst, typename TileInd::TileDType __in__ indexes,
    __ubuf__ typename TileDst::DType *tmp, size_t totalElems, unsigned validRow, unsigned validCol)
{
    using IndexT = typename TileInd::DType;
    __ubuf__ typename TileDst::DType *dstPtr = (__ubuf__ typename TileDst::DType *)__cce_get_tile_ptr(dst);
    __ubuf__ IndexT *idxPtr = (__ubuf__ IndexT *)__cce_get_tile_ptr(indexes);
    constexpr unsigned dstStride = TileDst::RowStride;
    constexpr unsigned idxStride = TileInd::RowStride;

    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            const size_t idx = static_cast<size_t>(idxPtr[r * idxStride + c]);
            PTO_ASSERT(idx < totalElems, "MGATHER: index out of bounds.");
            dstPtr[r * dstStride + c] = tmp[idx];
        }
    }
    // Ensure the dst tile is fully materialized before the caller switches pipes.
    pipe_barrier(PIPE_ALL);
}

template <typename TileSrc, typename TileInd>
__tf__ PTO_INTERNAL void MScatterToUb(__ubuf__ typename TileSrc::DType *tmp, typename TileSrc::TileDType __in__ src,
    typename TileInd::TileDType __in__ indexes, size_t totalElems, unsigned validRow, unsigned validCol)
{
    using IndexT = typename TileInd::DType;
    __ubuf__ typename TileSrc::DType *srcPtr = (__ubuf__ typename TileSrc::DType *)__cce_get_tile_ptr(src);
    __ubuf__ IndexT *idxPtr = (__ubuf__ IndexT *)__cce_get_tile_ptr(indexes);
    constexpr unsigned srcStride = TileSrc::RowStride;
    constexpr unsigned idxStride = TileInd::RowStride;

    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            const size_t idx = static_cast<size_t>(idxPtr[r * idxStride + c]);
            PTO_ASSERT(idx < totalElems, "MSCATTER: index out of bounds.");
            tmp[idx] = srcPtr[r * srcStride + c];
        }
    }
    // Ensure tmp updates complete before MTE reads it for GM store.
    pipe_barrier(PIPE_ALL);
}

template <typename TileDst, typename GlobalData, typename TileInd>
PTO_INTERNAL void MGATHER_IMPL(TileDst &dst, GlobalData &src, TileInd &indexes)
{
    using IndexT = typename TileInd::DType;
    static_assert(std::is_integral_v<IndexT>, "MGATHER: indexes must be an integral type");
    static_assert(sizeof(typename TileDst::DType) == sizeof(typename GlobalData::DType),
                  "MGATHER: element sizes must match");
    static_assert(TileDst::Loc == TileType::Vec && TileInd::Loc == TileType::Vec, "MGATHER: only supports Vec tiles.");
    static_assert(TileDst::SFractal == SLayout::NoneBox && TileInd::SFractal == SLayout::NoneBox,
        "MGATHER: only supports ND tiles.");
    static_assert(TileDst::isRowMajor && TileInd::isRowMajor, "MGATHER: only supports row-major ND tiles.");

    const unsigned validRow = dst.GetValidRow();
    const unsigned validCol = dst.GetValidCol();
    if (validRow == 0 || validCol == 0) {
        return;
    }

    const size_t g0 = static_cast<size_t>(src.GetShape(GlobalTensorDim::DIM_0));
    const size_t g1 = static_cast<size_t>(src.GetShape(GlobalTensorDim::DIM_1));
    const size_t g2 = static_cast<size_t>(src.GetShape(GlobalTensorDim::DIM_2));
    const size_t g3 = static_cast<size_t>(src.GetShape(GlobalTensorDim::DIM_3));
    const size_t g4 = static_cast<size_t>(src.GetShape(GlobalTensorDim::DIM_4));
    const size_t totalElems = g0 * g1 * g2 * g3 * g4;
    const size_t totalBytes = totalElems * sizeof(typename TileDst::DType);

    PTO_ASSERT(totalBytes <= TMP_UB_SIZE, "MGATHER: src tensor too large for tmp UB buffer.");
    PTO_ASSERT((totalBytes % BLOCK_BYTE_SIZE) == 0, "MGATHER: src tensor size must be 32B-aligned.");

    __ubuf__ typename TileDst::DType *tmp = (__ubuf__ typename TileDst::DType *)(TMP_UB_OFFSET);
    CopyGmToUb<typename TileDst::DType>(tmp, src.data(), static_cast<uint32_t>(totalBytes));
    pipe_barrier(PIPE_MTE2);
    MGatherFromUb<TileDst, TileInd>(dst.data(), indexes.data(), tmp, totalElems, validRow, validCol);
}

template <typename GlobalData, typename TileSrc, typename TileInd>
PTO_INTERNAL void MSCATTER_IMPL(GlobalData &dst, TileSrc &src, TileInd &indexes)
{
    using IndexT = typename TileInd::DType;
    static_assert(std::is_integral_v<IndexT>, "MSCATTER: indexes must be an integral type");
    static_assert(sizeof(typename TileSrc::DType) == sizeof(typename GlobalData::DType),
                  "MSCATTER: element sizes must match");
    static_assert(TileSrc::Loc == TileType::Vec && TileInd::Loc == TileType::Vec, "MSCATTER: only supports Vec tiles.");
    static_assert(TileSrc::SFractal == SLayout::NoneBox && TileInd::SFractal == SLayout::NoneBox,
        "MSCATTER: only supports ND tiles.");
    static_assert(TileSrc::isRowMajor && TileInd::isRowMajor, "MSCATTER: only supports row-major ND tiles.");

    const unsigned validRow = src.GetValidRow();
    const unsigned validCol = src.GetValidCol();
    if (validRow == 0 || validCol == 0) {
        return;
    }

    const size_t g0 = static_cast<size_t>(dst.GetShape(GlobalTensorDim::DIM_0));
    const size_t g1 = static_cast<size_t>(dst.GetShape(GlobalTensorDim::DIM_1));
    const size_t g2 = static_cast<size_t>(dst.GetShape(GlobalTensorDim::DIM_2));
    const size_t g3 = static_cast<size_t>(dst.GetShape(GlobalTensorDim::DIM_3));
    const size_t g4 = static_cast<size_t>(dst.GetShape(GlobalTensorDim::DIM_4));
    const size_t totalElems = g0 * g1 * g2 * g3 * g4;
    const size_t totalBytes = totalElems * sizeof(typename TileSrc::DType);

    PTO_ASSERT(totalBytes <= TMP_UB_SIZE, "MSCATTER: dst tensor too large for tmp UB buffer.");
    PTO_ASSERT((totalBytes % BLOCK_BYTE_SIZE) == 0, "MSCATTER: dst tensor size must be 32B-aligned.");

    __ubuf__ typename TileSrc::DType *tmp = (__ubuf__ typename TileSrc::DType *)(TMP_UB_OFFSET);
    CopyGmToUb<typename TileSrc::DType>(tmp, dst.data(), static_cast<uint32_t>(totalBytes));
    pipe_barrier(PIPE_MTE2);
    MScatterToUb<TileSrc, TileInd>(tmp, src.data(), indexes.data(), totalElems, validRow, validCol);

    CopyUbToGm(dst.data(), tmp, static_cast<uint32_t>(totalBytes));
    pipe_barrier(PIPE_MTE3);
}

} // namespace pto

#endif
