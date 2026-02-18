/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TMINS_HPP
#define TMINS_HPP

#include <pto/common/constants.hpp>
#include "TBinSOp.hpp"

namespace pto
{
    template<typename T>
    struct MinSOp {
        PTO_INTERNAL static void BinSInstr(__ubuf__ T* dst, __ubuf__ T* src0, T src1, uint8_t repeats) {
            vmins(dst, src0, src1, repeats, 1, 1, 8, 8);
        }
        PTO_INTERNAL static void BinSInstr(__ubuf__ T* dst, __ubuf__ T* src0, T src1, uint8_t repeats, uint8_t dstRepeatStride, uint8_t srcRepeatStride) {
            vmins(dst, src0, src1, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
        }
    };

    template <typename T, typename TileDataDst, typename TileDataSrc>
    __tf__ PTO_INTERNAL void TMinS(typename TileDataDst::TileDType __out__ dstData,
                                   typename TileDataSrc::TileDType __in__ srcData,
                                   T __in__ scalar,
                                   unsigned validRow,
                                   unsigned validCol) {
        __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
        __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
        constexpr unsigned elementsPerRepeat = pto::REPEAT_BYTE / sizeof(T);
        constexpr unsigned blockSizeElem = pto::BLOCK_BYTE_SIZE / sizeof(T);
        constexpr unsigned dstStride = TileDataDst::RowStride;
        constexpr unsigned srcStride = TileDataSrc::RowStride;
        TBinSInstr<MinSOp<T>, TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>
            (dst, src, scalar, validRow, validCol);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TMINS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
    {
        using T = typename TileDataSrc::DType;
        static_assert(std::is_same_v<T, typename TileDataDst::DType>,
            "TMINS: The data type of dst must be consistent with src.");
        static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value ||
                      std::is_same<T, int16_t>::value || std::is_same<T, half>::value ||
                      std::is_same<T, float16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, float32_t>::value, "TMINS: Invalid data type");

        static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");

        PTO_ASSERT(src.GetValidCol() == dst.GetValidCol(), "Number of cols of src and dst must be the same.");
        PTO_ASSERT(src.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");

        unsigned dstValidRow = dst.GetValidRow();
        unsigned dstValidCol = dst.GetValidCol();
        if ((dstValidRow != 0 && dstValidCol != 0) &&
            (dstValidRow == src.GetValidRow() && dstValidCol == src.GetValidCol())) {
            TMinS<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dstValidRow, dstValidCol);
        } else {
            PTO_ASSERT(false, "TMINS: dstTile validRow/validCol must be consistent with of src.");
        }
    }
}

#endif
