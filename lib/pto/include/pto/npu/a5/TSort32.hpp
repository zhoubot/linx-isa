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
#include <limits>
#include "common.hpp"

#define PTO_CEIL(x,y)        ((((x)+(y)-1)/(y)) * (y))
#define PTO_DIV_ROUNDUP(x,y) ((((x)+(y)-1)/(y)))

namespace pto {
    
constexpr const uint32_t BLOCK_SIZE = 32;
constexpr const uint32_t FLOAT_DST_STRIDE_COEF = 2;
constexpr const uint32_t HALF_DST_STRIDE_COEF = 4;
constexpr const uint32_t MAX_UB_TMP = 32 * 255;

template <typename DstTileData, typename SrcTileData, typename IdxTileData, unsigned dstStride, unsigned srcStride>
__tf__ AICORE inline void TSort32Impl(
    typename DstTileData::TileDType __out__ dst,
    typename SrcTileData::TileDType __in__ src,
    typename IdxTileData::TileDType __in__ idx,
    unsigned validRow,
    unsigned repeatNumPerRow,
    unsigned idxStride)
{
    using T = typename DstTileData::DType;
    using IdxT = typename IdxTileData::DType;

    __ubuf__ T *dstPtr  = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr  = (__ubuf__ T *)__cce_get_tile_ptr(src);
    __ubuf__ IdxT *idxPtr  = (__ubuf__ IdxT *)__cce_get_tile_ptr(idx);

    if (repeatNumPerRow <= REPEAT_MAX) {
        for (uint32_t i = 0; i < validRow; i++) {
            vbitsort(dstPtr + i * dstStride,
                    srcPtr + i * srcStride,
                    idxPtr + i * idxStride,
                    repeatNumPerRow
            );
        }
    } else {
        uint32_t loopNum = PTO_DIV_ROUNDUP(repeatNumPerRow, REPEAT_MAX);
        uint32_t tailRepeatNum = repeatNumPerRow % REPEAT_MAX;
        constexpr uint32_t typeCoef = (sizeof(T) == sizeof(float)) ? FLOAT_DST_STRIDE_COEF : HALF_DST_STRIDE_COEF;
        for (uint32_t i = 0; i < validRow; i++) {
            for (uint32_t j = 0; j < loopNum; j++) {
                uint32_t repeatNum = (j == loopNum -1) ? tailRepeatNum : REPEAT_MAX;
                vbitsort(
                    dstPtr + i * dstStride + j * REPEAT_MAX * BLOCK_SIZE * typeCoef,
                    srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE,
                    idxPtr + i * idxStride + j * REPEAT_MAX * BLOCK_SIZE,
                    repeatNum
                );    
            }
        }
    }
}

template <typename T, typename IdxT, unsigned dstStride, unsigned srcStride>
PTO_INTERNAL void LargeTmpBufferImpl(__ubuf__ T *dstPtr, __ubuf__ T *srcPtr, __ubuf__ IdxT *idxPtr, __ubuf__ T *tmpPtr,
    unsigned validRow, unsigned repeatNumPerRow, unsigned idxStride, unsigned srcTailPerRow, unsigned srcTailRepeatNum)
{
    T minVal = -(0.0 / 0.0);
    auto loopNum = PTO_DIV_ROUNDUP(repeatNumPerRow, REPEAT_MAX);
    constexpr uint32_t typeCoef = (sizeof(T) == sizeof(float)) ? FLOAT_DST_STRIDE_COEF : HALF_DST_STRIDE_COEF;
    for (int32_t i = 0; i < validRow; i++) {
        for (int32_t j = 0; j < loopNum; j++) {
            if (j < loopNum - 1) {
                vbitsort(dstPtr + i * dstStride + j * REPEAT_MAX * BLOCK_SIZE * typeCoef,
                    srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE, idxPtr + i * idxStride + j * REPEAT_MAX * BLOCK_SIZE,
                    REPEAT_MAX);
            } else {
                //sort for last block
                vbitsort(dstPtr + i * dstStride + j * REPEAT_MAX * BLOCK_SIZE * typeCoef,
                    srcPtr + i * srcStride + j * REPEAT_MAX * BLOCK_SIZE, idxPtr + i * idxStride + j * REPEAT_MAX * BLOCK_SIZE,
                    srcTailRepeatNum - 1);

                // copy row src cbuf to tmp cbuf
                uint16_t lenBurst = PTO_DIV_ROUNDUP(srcTailPerRow * sizeof(T), BLOCK_SIZE);
                copy_ubuf_to_ubuf(tmpPtr, srcPtr + i * srcStride + (j * REPEAT_MAX + (srcTailRepeatNum - 1)) * BLOCK_SIZE,
                    0, 1, lenBurst, 0, 0);

                __VEC_SCOPE__{
                    RegTensor<T> vreg_padded; 
                    uint32_t count_preg = sizeof(T)*srcTailPerRow/2;
                    uint32_t st_count   = sizeof(T)*PTO_CEIL(srcTailPerRow, BLOCK_SIZE)/2;
                    vector_bool st_preg = plt_b16(st_count, POST_UPDATE);
                    vector_bool preg_tail_inv = plt_b16(count_preg, POST_UPDATE);
                    vector_bool preg_all = pset_b16(PAT_ALL);
                    vector_bool preg_tail;
                    pnot(preg_tail, preg_tail_inv, preg_all);
                    vector_align ld_align_reg, st_align_reg;
                    // pad the last 32 elements
                    __ubuf__ T * tmpPtr_lastRepeatPerRow = tmpPtr + PTO_CEIL(srcTailPerRow, BLOCK_SIZE) - BLOCK_SIZE;
                    __ubuf__ T * tmpDstPtr =  tmpPtr_lastRepeatPerRow;
                    // only load and pad the last unaligned 32 elements per row, No need for post-update 
                    vlds(vreg_padded, tmpPtr_lastRepeatPerRow, 0, NORM);   
                    vdup(vreg_padded, minVal, preg_tail, MODE_MERGING);
                    vsts((vector_f16 &)vreg_padded, (__ubuf__ half*&)tmpDstPtr, 0, NORM_B16, st_preg);
                }

                // sort for tmp and out to dst
                vbitsort(dstPtr + i * dstStride + (j * REPEAT_MAX + (srcTailRepeatNum - 1)) * BLOCK_SIZE * typeCoef, tmpPtr,
                    idxPtr + i * idxStride + (j * REPEAT_MAX + (srcTailRepeatNum - 1)) * BLOCK_SIZE, 1);
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
        uint16_t lenBurst = PTO_DIV_ROUNDUP(srcShapeBytesPerRow, BLOCK_SIZE);
        for (int32_t i = 0; i < validRow; i++) {
            copy_ubuf_to_ubuf(tmpPtr, srcPtr + i * srcStride, 0, 1, lenBurst, 0, 0);

            __VEC_SCOPE__{
                RegTensor<T> vreg_padded; 
                uint32_t count_preg = (sizeof(T)*srcTailPerRow)/2;
                uint32_t st_count   = sizeof(T)*PTO_CEIL(srcTailPerRow, BLOCK_SIZE)/2;
                vector_bool st_preg = plt_b16(st_count, POST_UPDATE);
                vector_bool preg_tail_inv = plt_b16(count_preg, POST_UPDATE);
                vector_bool preg_all = pset_b16(PAT_ALL);
                vector_bool preg_tail;
                pnot(preg_tail, preg_tail_inv, preg_all);
                vector_align ld_align_reg, st_align_reg;
                __ubuf__ T * tmpPtr_lastRepeatPerRow = tmpPtr + PTO_CEIL(srcStride, BLOCK_SIZE) - BLOCK_SIZE; // pad the last 32 elements
                __ubuf__ T * tmpDstPtr =  tmpPtr_lastRepeatPerRow;
                // only load and pad the last unaligned 32 elements per row, No need for post-update 
                vlds(vreg_padded, tmpPtr_lastRepeatPerRow, 0, NORM);   
                vdup(vreg_padded, minVal, preg_tail, MODE_MERGING);
                vsts((vector_f16 &)vreg_padded, (__ubuf__ half*&)tmpDstPtr, 0, NORM_B16, st_preg);
            }

            // sort for tmp and out to dst
            vbitsort(dstPtr + i * dstStride, tmpPtr, idxPtr + i * idxStride, repeatNumPerRow + 1);
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
                    "Dst and src must be float or half.");
    static_assert((std::is_same<typename IdxTileData::DType, uint32_t>::value),
                    "Idx must be uint32_t.");
    static_assert((std::is_same<typename DstTileData::DType, typename SrcTileData::DType>::value),
                    "Dst and src mube be same.");
    static_assert((DstTileData::Loc == TileType::Vec) && (SrcTileData::Loc == TileType::Vec) &&
                    (IdxTileData::Loc == TileType::Vec),
                    "TileType must be Vec!");
    static_assert((DstTileData::isRowMajor && SrcTileData::isRowMajor && IdxTileData::isRowMajor),
                    "Expect row major");
}

template <typename DstTileData, typename SrcTileData, typename IdxTileData>
AICORE inline void TSORT32_IMPL(
    DstTileData &dst,
    SrcTileData &src,
    IdxTileData &idx)
{
    CheckStatic<DstTileData, SrcTileData, IdxTileData>();
    unsigned validRow = dst.GetValidRow();
    unsigned repeatNumPerRow = src.GetValidCol() / 32;

    constexpr unsigned dstStride = DstTileData::RowStride;
    constexpr unsigned srcStride = SrcTileData::RowStride;
    unsigned idxStride = idx.GetValidRow() == 1 ? 0 : IdxTileData::RowStride;
    
    TSort32Impl<DstTileData, SrcTileData, IdxTileData, dstStride, srcStride>(
        dst.data(), src.data(), idx.data(), validRow, repeatNumPerRow, idxStride);
}

template <typename DstTileData, typename SrcTileData, typename IdxTileData, typename TmpTileData>
AICORE inline void TSORT32_IMPL(
    DstTileData &dst, SrcTileData &src, IdxTileData &idx, TmpTileData &tmp)
{
    CheckStatic<DstTileData, SrcTileData, IdxTileData>();
    unsigned validRow = dst.GetValidRow();
    unsigned repeatNumPerRow = src.GetValidCol() / 32;

    constexpr unsigned byteSize     = sizeof(typename DstTileData::DType);
    constexpr unsigned dstStride    = PTO_CEIL(DstTileData::RowStride * byteSize, BLOCK_SIZE) / byteSize;
    constexpr unsigned srcStride    = PTO_CEIL(SrcTileData::RowStride * byteSize, BLOCK_SIZE) / byteSize;
    constexpr unsigned tmpIdxStride = PTO_CEIL(IdxTileData::RowStride * 4,        BLOCK_SIZE) / 4;
    
    unsigned idxStride = idx.GetValidRow() == 1 ? 0 : tmpIdxStride;

    if (src.GetValidCol() % 32 == 0) {
        TSort32Impl<DstTileData, SrcTileData, IdxTileData, dstStride, srcStride>(
            dst.data(), src.data(), idx.data(), validRow, repeatNumPerRow, idxStride);
    } else {
        unsigned srcShapeBytesPerRow = src.GetValidCol() * byteSize;
        unsigned srcTailPerRow = src.GetValidCol() % 32;
        unsigned srcTailRepeatNum = PTO_DIV_ROUNDUP(src.GetValidCol(),BLOCK_SIZE)% REPEAT_MAX;
        TSort32Impl<DstTileData, SrcTileData, IdxTileData, TmpTileData, dstStride, srcStride>(
            dst.data(), src.data(), idx.data(), tmp.data(), validRow, repeatNumPerRow, idxStride,
            srcShapeBytesPerRow, srcTailPerRow, srcTailRepeatNum);
    }
}
}
#endif