/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLSUM_HPP
#define TCOLSUM_HPP

#include <pto/common/utils.hpp>
#include <pto/common/type.hpp>

namespace pto {
    template <typename T, int SrcStride, int DstStride>
    PTO_INTERNAL void BinarySum(__ubuf__ T *dst, __ubuf__ T *src, int validRow, int validCol)
    {
        set_mask_count(); 
        set_vector_mask(0, validCol);
        for (uint32_t i = 0; i < validRow / 2; i++) {
            vadd(dst + i * DstStride, src + 2 * i * SrcStride, src + (2 * i + 1) * SrcStride, 0, 1, 1, 1, 8, 8, 8);
        }
        pipe_barrier(PIPE_V);

        if (validRow % 2 == 1) {
            vadd(dst, dst, src + (validRow - 1) * SrcStride, 0, 1, 1, 1, 8, 8, 8);
            pipe_barrier(PIPE_V);
        }
        set_mask_norm();
        set_vector_mask(-1, -1);
    }

    template <typename T, int SrcStride, int DstStride>
    PTO_INTERNAL void SequentialSum(__ubuf__ T *dst, __ubuf__ T *src, int validRow, int validCol)
    {
        set_mask_count(); 
        set_vector_mask(0, validCol);
        for (int i = 1; i < validRow; i++) {
            vadd(dst, dst, src + i * SrcStride, 0, 1, 1, 1, 8, 8, 8);
            pipe_barrier(PIPE_V);
        }
        set_mask_norm();
        set_vector_mask(-1, -1);
    }
    
    template <typename T, typename TileDataDst, typename TileDataSrc, typename TileDataTmp, int srcstride,
              int dststride, int tmpstride, bool IsBinary>
    __tf__ PTO_INTERNAL void TColSum(typename TileDataDst::TileDType __out__ dst,
                                     typename TileDataSrc::TileDType __in__ src,
                                     typename TileDataTmp::TileDType __in__ tmp,
                                     int validRow, int validCol) {
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
        __ubuf__ T *tmpPtr = (__ubuf__ T *)__cce_get_tile_ptr(tmp);

        constexpr int DTypeSize = sizeof(T);
        int lenBurst = (validCol * DTypeSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;

        if (validRow == 1) {
            copy_ubuf_to_ubuf(dstPtr, srcPtr, 0, 1, lenBurst, 0, 0);
            pipe_barrier(PIPE_V);
            return;
        }

        if (IsBinary) {
            BinarySum<T, srcstride, tmpstride>(tmpPtr, srcPtr, validRow, validCol);
            int cnt = validRow / 2;
            while (cnt > 1) {
                BinarySum<T, tmpstride, tmpstride>(tmpPtr, tmpPtr, cnt, validCol);
                pipe_barrier(PIPE_V);
                cnt /= 2;
            }
            copy_ubuf_to_ubuf(dstPtr, tmpPtr, 0, 1, lenBurst, 0, 0);
            pipe_barrier(PIPE_V);
        } else {
            copy_ubuf_to_ubuf(dstPtr, srcPtr, 0, 1, lenBurst, 0, 0);
            pipe_barrier(PIPE_V);
            SequentialSum<T, srcstride, dststride>(dstPtr, srcPtr, validRow, validCol);
        }
    }

    template <typename TileDataDst, typename TileDataSrc, typename TileDataTmp>
    PTO_INTERNAL void TCOLSUM_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataTmp &tmp, bool IsBinary) {
        using T = typename TileDataSrc::DType;
        static_assert(TileDataDst::Loc == pto::TileType::Vec && TileDataSrc::Loc == pto::TileType::Vec &&
                      TileDataTmp::Loc == pto::TileType::Vec, "Fix: TCOLSUM only support Vec Tile");
        static_assert(TileDataSrc::isRowMajor && TileDataSrc::SFractal == SLayout::NoneBox,
                      "Fix: TCOLSUM only support Nd fractal Tile");
        static_assert(TileDataDst::isRowMajor && TileDataDst::SFractal == SLayout::NoneBox,
                      "Fix: TCOLSUM only support Nd fractal Tile");
        static_assert(TileDataTmp::isRowMajor && TileDataTmp::SFractal == SLayout::NoneBox,
                      "Fix: TCOLSUM only support Nd fractal Tile");
        static_assert(std::is_same_v<T, half> || std::is_same_v<T, float> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>,
                      "Fix: TCOLSUM input data type is not supported by this instruction.");
        static_assert(std::is_same_v<typename TileDataDst::DType, T> && std::is_same_v<typename TileDataTmp::DType, T>,
            "Fix: TCOLSUM input data type must be consistent with the output data type and the tmp data type.");
        PTO_ASSERT(src.GetValidCol() == dst.GetValidCol(), 
                   "Fix: TCOLSUM input valid col must be consistent with the output valid row.");

        if (src.GetValidRow() == 0 || src.GetValidCol() == 0) {
            return;
        }
        constexpr int srcstride = TileDataSrc::RowStride;
        constexpr int dststride = TileDataDst::RowStride;
        constexpr int tmpstride = TileDataTmp::RowStride;
        int validRow = src.GetValidRow();
        int validCol = src.GetValidCol();
        if (IsBinary) {
            TColSum<T, TileDataDst, TileDataSrc, TileDataTmp, srcstride, dststride, tmpstride, true>
                (dst.data(), src.data(), tmp.data(), validRow, validCol);
        } else {
            TColSum<T, TileDataDst, TileDataSrc, TileDataTmp, srcstride, dststride, tmpstride, false>
                (dst.data(), src.data(), tmp.data(), validRow, validCol);
        }  
    }

    template <typename T, typename TileDataDst, typename TileDataSrc, int srcstride, int dststride>
    __tf__ PTO_INTERNAL void TColSumNoTmp(typename TileDataDst::TileDType __out__ dst,
                                         typename TileDataSrc::TileDType __in__ src,
                                         int validRow, int validCol) {
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

        constexpr int DTypeSize = sizeof(T);
        int lenBurst = (validCol * DTypeSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;

        if (validRow == 1) {
            copy_ubuf_to_ubuf(dstPtr, srcPtr, 0, 1, lenBurst, 0, 0);
            pipe_barrier(PIPE_V);
            return;
        }

        copy_ubuf_to_ubuf(dstPtr, srcPtr, 0, 1, lenBurst, 0, 0);
        pipe_barrier(PIPE_V);

        SequentialSum<T, srcstride, dststride>(dstPtr, srcPtr, validRow, validCol);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TCOLSUM_IMPL(TileDataDst &dst, TileDataSrc &src) {
        using T = typename TileDataSrc::DType;
        static_assert(TileDataDst::Loc == pto::TileType::Vec && TileDataSrc::Loc == pto::TileType::Vec,
            "Fix: TCOLSUM only support Vec Tile");
        static_assert(TileDataSrc::isRowMajor && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TCOLSUM only support Nd fractal Tile");
        static_assert(TileDataDst::isRowMajor && TileDataDst::SFractal == SLayout::NoneBox,
            "Fix: TCOLSUM only support Nd fractal Tile");
        static_assert(std::is_same_v<T, half> || std::is_same_v<T, float> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>,
            "Fix: TCOLSUM input data type is not supported by this instruction.");
        static_assert(std::is_same_v<typename TileDataDst::DType, T>,
            "Fix: TCOLSUM input data type must be consistent with the output data type.");
        PTO_ASSERT(src.GetValidCol() == dst.GetValidCol(),
            "Fix: TCOLSUM input/output valid col must be consistent.");

        if (src.GetValidRow() == 0 || src.GetValidCol() == 0) {
            return;
        }
        int validRow = src.GetValidRow();
        int validCol = src.GetValidCol();
        constexpr int srcstride = TileDataSrc::RowStride;
        constexpr int dststride = TileDataDst::RowStride;
        TColSumNoTmp<T, TileDataDst, TileDataSrc, srcstride, dststride>(dst.data(), src.data(), validRow, validCol);
    }
}
#endif
