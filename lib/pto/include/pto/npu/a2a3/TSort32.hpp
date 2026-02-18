/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSORT32_HPP
#define TSORT32_HPP

#include <pto/common/constants.hpp>

namespace pto {

constexpr const uint32_t BLOCK_SIZE = 32;
constexpr const uint32_t FLOAT_DST_STRIDE_COEF = 2;
constexpr const uint32_t HALF_DST_STRIDE_COEF = 4;
constexpr const uint32_t MAX_UB_TMP = 32 * 255;

template <typename T, typename IdxT, unsigned dstStride, unsigned srcStride>
PTO_INTERNAL void LargeTmpBufferImpl(__ubuf__ T *dstPtr, __ubuf__ T *srcPtr, __ubuf__ IdxT *idxPtr, __ubuf__ T *tmpPtr,
    unsigned validRow, unsigned repeatNumPerRow, unsigned idxStride, unsigned srcTailPerRow, unsigned srcTailRepeatNum)
{
    T minVal = -(0.0 / 0.0);
    auto loopNum = ((repeatNumPerRow + REPEAT_MAX) - 1) / REPEAT_MAX;
    constexpr uint32_t typeCoef = (sizeof(T) == sizeof(float)) ? FLOAT_DST_STRIDE_COEF : HALF_DST_STRIDE_COEF;
    for (int32_t i = 0; i < validRow; i++) {
        for (int32_t j = 0; j < loopNum; j++) {
            if (j < loopNum - 1) {
                vbitsort(dstPtr + i * dstStride + j * REPEAT_MAX * BLOCK_SIZE * typeCoef,
                    srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE,
                    idxPtr + i * idxStride + j * REPEAT_MAX * BLOCK_SIZE, REPEAT_MAX);
                pipe_barrier(PIPE_V);
            } else {
                // sort for last block
                vbitsort(dstPtr + i * dstStride + j * REPEAT_MAX * BLOCK_SIZE * typeCoef,
                    srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE,
                    idxPtr + i * idxStride + j * REPEAT_MAX * BLOCK_SIZE, srcTailRepeatNum - 1);
                pipe_barrier(PIPE_V);

                // copy row src cbuf to tmp cbuf
                uint16_t lenBurst = (srcTailPerRow * sizeof(T) + BLOCK_SIZE - 1) / BLOCK_SIZE;
                copy_ubuf_to_ubuf(tmpPtr, srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE +
                    (srcTailRepeatNum - 1) * BLOCK_SIZE, 0, 1, lenBurst, 0, 0);
                pipe_barrier(PIPE_V);

                // dup -inf of tial value in tmp cbuf
                uint64_t mask = ~(((static_cast<uint32_t>(1)) << (srcTailPerRow)) - 1);
                set_mask_norm();
                set_vector_mask(0, mask);
                vector_dup(tmpPtr, minVal, 1, 1, 1, 8, (int64_t)0);
                pipe_barrier(PIPE_V);

                // sort for tmp and out to dst
                vbitsort(dstPtr + i * dstStride + (j * REPEAT_MAX + (srcTailRepeatNum - 1)) * BLOCK_SIZE * typeCoef,
                    tmpPtr, idxPtr + i * idxStride + (j * REPEAT_MAX + (srcTailRepeatNum - 1)) * BLOCK_SIZE, 1);
                pipe_barrier(PIPE_V);
                set_vector_mask(-1, -1);
            }
        }
    }
} 

template <typename DstTileData, typename SrcTileData, typename IdxTileData,
    unsigned dstStride, unsigned srcStride>
__tf__ AICORE void TSort32Impl(typename DstTileData::TileDType __out__ dst,
                            typename SrcTileData::TileDType __in__ src,
                            typename IdxTileData::TileDType __in__ idx,
                            unsigned validRow,
                            unsigned repeatNumPerRow,
                            unsigned idxStride)
{
    using T = typename DstTileData::DType;
    using IdxT = typename IdxTileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
    __ubuf__ IdxT *idxPtr = (__ubuf__ IdxT *)__cce_get_tile_ptr(idx);

    if (repeatNumPerRow <= REPEAT_MAX) {
        for (int32_t i = 0; i < validRow; i++) {
            vbitsort(dstPtr + i * dstStride, srcPtr + i * srcStride, idxPtr + i * idxStride, repeatNumPerRow);
            pipe_barrier(PIPE_V);
        }
    } else {
        uint32_t loopNum = ((repeatNumPerRow + REPEAT_MAX) - 1) / REPEAT_MAX;
        uint32_t tailRepeatNum = repeatNumPerRow % REPEAT_MAX;
        constexpr uint32_t typeCoef = (sizeof(T) == sizeof(float)) ? FLOAT_DST_STRIDE_COEF : HALF_DST_STRIDE_COEF;
        for (int32_t i = 0; i < validRow; i++) {
            for (int32_t j = 0; j < loopNum; j++) {
                uint32_t repeatNum = (j == loopNum -1) ? tailRepeatNum : REPEAT_MAX;
                vbitsort(
                    dstPtr + i * dstStride + j * REPEAT_MAX * BLOCK_SIZE * typeCoef,
                    srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE,
                    idxPtr + i * idxStride + j * REPEAT_MAX * BLOCK_SIZE,
                    repeatNum);
                pipe_barrier(PIPE_V);
            }
        }
    }
}

template <typename DstTileData, typename SrcTileData, typename IdxTileData, typename TmpTileData, unsigned dstStride, unsigned srcStride>
__tf__ AICORE void TSort32Impl(typename DstTileData::TileDType __out__ dst,
                            typename SrcTileData::TileDType __in__ src,
                            typename IdxTileData::TileDType __in__ idx,
                            typename TmpTileData::TileDType __in__ tmp,
                            unsigned validRow,
                            unsigned repeatNumPerRow,
                            unsigned idxStride,
                            unsigned srcShapeBytesPerRow,
                            unsigned srcTailPerRow,
                            unsigned srcTailRepeatNum)
{
    using T = typename DstTileData::DType;
    using IdxT = typename IdxTileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
    __ubuf__ IdxT *idxPtr = (__ubuf__ IdxT *)__cce_get_tile_ptr(idx);
    __ubuf__ T *tmpPtr = (__ubuf__ T *)__cce_get_tile_ptr(tmp);

    T minVal = -(0.0 / 0.0);
    if (srcShapeBytesPerRow * sizeof(float) / sizeof(T) <= MAX_UB_TMP) {
        for (int32_t i = 0; i < validRow; i++) {
            // copy row src cbuf to tmp cbuf
            uint16_t lenBurst = (srcShapeBytesPerRow + BLOCK_SIZE - 1) / BLOCK_SIZE;
            copy_ubuf_to_ubuf(tmpPtr,srcPtr + i * srcStride, 0, 1, lenBurst, 0, 0);
            pipe_barrier(PIPE_V);

            // dup -NAN of tial value in tmp cbuf
            uint64_t mask = ~(((static_cast<uint32_t>(1)) << (srcTailPerRow)) - 1);
            set_mask_norm();
            set_vector_mask(0, mask);
            vector_dup(tmpPtr + repeatNumPerRow * BLOCK_SIZE, minVal, 1, 1, 1, 8, (int64_t)0);
            pipe_barrier(PIPE_V);

            // sort for tmp and out to dst
            vbitsort(dstPtr + i * dstStride, tmpPtr, idxPtr + i * idxStride, repeatNumPerRow + 1);
            pipe_barrier(PIPE_V);
            set_vector_mask(-1, -1);
        }
    } else {
        LargeTmpBufferImpl<T, IdxT, dstStride, srcStride>(dstPtr, srcPtr, idxPtr, tmpPtr, validRow, repeatNumPerRow, idxStride,
            srcTailPerRow, srcTailRepeatNum);            
    }
}

template <typename DstTileData, typename SrcTileData, typename IdxTileData>
PTO_INTERNAL void CheckStatic()
{
    static_assert((std::is_same<typename DstTileData::DType, half>::value) ||
                    (std::is_same<typename DstTileData::DType, float>::value),
                    "Dst and src must be half or float.");
    static_assert((std::is_same<typename IdxTileData::DType, uint32_t>::value),
                    "Idx must be uint32_t.");
    static_assert((std::is_same<typename DstTileData::DType, typename SrcTileData::DType>::value),
                    "Dst and src must be same.");
    static_assert((DstTileData::Loc == TileType::Vec) && (SrcTileData::Loc == TileType::Vec) &&
                    (IdxTileData::Loc == TileType::Vec),
                    "TileType must be Vec.");
    static_assert((DstTileData::isRowMajor && SrcTileData::isRowMajor && IdxTileData::isRowMajor),
                    "Expect row major");
}

// 32 Align Interface, No tmpTile
template <typename DstTileData, typename SrcTileData, typename IdxTileData>
PTO_INTERNAL void TSORT32_IMPL(DstTileData &dst, SrcTileData &src, IdxTileData &idx)
{
    CheckStatic<DstTileData, SrcTileData, IdxTileData>();
    unsigned validRow = dst.GetValidRow();
    unsigned repeatNumPerRow = src.GetValidCol() / BLOCK_SIZE;
    constexpr unsigned dstStride = DstTileData::RowStride;
    constexpr unsigned srcStride = SrcTileData::RowStride;
    unsigned idxStride = idx.GetValidRow() == 1 ? 0 : IdxTileData::RowStride;

    TSort32Impl<DstTileData, SrcTileData, IdxTileData, dstStride, srcStride>
        (dst.data(), src.data(), idx.data(), validRow, repeatNumPerRow, idxStride);
}

// 32 Non-Align Interface, Have tmpTile
template <typename DstTileData, typename SrcTileData, typename IdxTileData, typename TmpTileData>
PTO_INTERNAL void TSORT32_IMPL(DstTileData &dst, SrcTileData &src, IdxTileData &idx, TmpTileData &tmp)
{
    CheckStatic<DstTileData, SrcTileData, IdxTileData>();
    unsigned validRow = dst.GetValidRow();
    unsigned repeatNumPerRow = src.GetValidCol() / BLOCK_SIZE;
    constexpr unsigned byteSize = sizeof(typename DstTileData::DType);
    constexpr unsigned idxByteSize = sizeof(typename IdxTileData::DType);
    constexpr unsigned dstStride =
        ((DstTileData::RowStride * byteSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE * BLOCK_BYTE_SIZE) / byteSize;
    constexpr unsigned srcStride = 
        ((SrcTileData::RowStride * byteSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE * BLOCK_BYTE_SIZE) / byteSize;
    constexpr unsigned tmpIdxStride = 
        ((IdxTileData::RowStride * idxByteSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE * BLOCK_BYTE_SIZE) / idxByteSize;
    unsigned idxStride = idx.GetValidRow() == 1 ? 0 : tmpIdxStride;

    if (src.GetValidCol() % BLOCK_SIZE > 0) {
        unsigned srcShapeBytesPerRow = src.GetValidCol() * byteSize;
        unsigned srcTailPerRow = src.GetValidCol() % BLOCK_SIZE;
        unsigned srcTailRepeatNum = ((src.GetValidCol() + BLOCK_SIZE - 1) / BLOCK_SIZE) % REPEAT_MAX;
        TSort32Impl<DstTileData, SrcTileData, IdxTileData, TmpTileData, dstStride, srcStride>
            (dst.data(), src.data(), idx.data(), tmp.data(), validRow, repeatNumPerRow, idxStride,
            srcShapeBytesPerRow, srcTailPerRow, srcTailRepeatNum);
    } else {
        TSort32Impl<DstTileData, SrcTileData, IdxTileData, dstStride, srcStride>
            (dst.data(), src.data(), idx.data(), validRow, repeatNumPerRow, idxStride);
    }
}
}
#endif