/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TLRELU_HPP
#define TLRELU_HPP

#include <pto/common/constants.hpp>
#include "pto/npu/a2a3/TBinSOp.hpp"

namespace pto
{
    template<typename T>
    struct LReluOp {
        PTO_INTERNAL static void BinSInstr(__ubuf__ T* dst, __ubuf__ T* src0, T src1, uint8_t repeats) {
            vlrelu(dst, src0, src1, repeats, 1, 1, 8, 8);
        }
        PTO_INTERNAL static void BinSInstr(__ubuf__ T* dst, __ubuf__ T* src0, T src1, uint8_t repeats, uint8_t dstRepeatStride, uint8_t srcRepeatStride) {
            vlrelu(dst, src0, src1, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
        }
    };

    template <typename T, typename TileDataDst, typename TileDataSrc>
    __tf__ PTO_INTERNAL void TLRelu(typename TileDataDst::TileDType __out__ dstData,
        typename TileDataSrc::TileDType __in__ srcData, T __in__ scalar, unsigned validRow, unsigned validCol) {
        __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
        __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
        constexpr unsigned elementsPerRepeat = pto::REPEAT_BYTE / sizeof(T);
        constexpr unsigned blockSizeElem = pto::BLOCK_BYTE_SIZE / sizeof(T);
        constexpr unsigned dstStride = TileDataDst::RowStride;
        constexpr unsigned srcStride = TileDataSrc::RowStride;
        TBinSInstr<LReluOp<T>, TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>
            (dst, src, scalar, validRow, validCol);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TLRELU_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
    {
        // static assertions
        using T = typename TileDataSrc::DType;        
        static_assert(std::is_same_v<T, typename TileDataDst::DType>,
            "TLRELU: The data type of dst must be consistent with src.");
        static_assert(std::is_same<T, half>::value ||
                      std::is_same<T, float16_t>::value ||
                      std::is_same<T, float>::value ||
                      std::is_same<T, float32_t>::value, "TLRELU: Invalid data type");

        static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc::Loc == TileType::Vec,
            "TLRELU: TileType of dst and src tiles must be TileType::Vec.");

        // dynamic checks
        unsigned dstValidRow = dst.GetValidRow();
        unsigned dstValidCol = dst.GetValidCol();
        PTO_ASSERT(dstValidRow > 0 && dstValidCol > 0, "TLRELU: Number of valid rows and valid columns of dst tile must be greater than 0.");
        PTO_ASSERT(dstValidRow == src.GetValidRow(), "TLRELU: Number of valid rows of dst and src must be the same.");
        PTO_ASSERT(dstValidCol == src.GetValidCol(), "TLRELU: Number of valid columns of dst and src must be the same.");

        TLRelu<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dstValidRow, dstValidCol);
    }
}

#endif