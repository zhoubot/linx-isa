/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWEXPANDSUB_HPP
#define TROWEXPANDSUB_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <pto/npu/a2a3/TRowExpandBinOp.hpp>

namespace pto {
template <typename T>
struct RowExpandSubOp {
    PTO_INTERNAL static void RowExpandBinInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t repeats) {
        vsub(dst, src0, src1, repeats, 1, 1, 0, 8, 8, 0);
    }
    PTO_INTERNAL static void RowExpandBinInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t repeats,
        uint8_t dstRepeatStride, uint8_t src0RepeatStride) {
        vsub(dst, src0, src1, repeats, 1, 1, 0, dstRepeatStride, src0RepeatStride, 1);
    }
};

template <typename T>
struct RowExpandSubOp2 {
    PTO_INTERNAL static void RowExpandBinInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t repeats) {
        vsub(dst, src1, src0, repeats, 1, 0, 1, 8, 0, 8);
    }
    PTO_INTERNAL static void RowExpandBinInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t repeats,
        uint8_t dstRepeatStride, uint8_t src0RepeatStride) {
        vsub(dst, src1, src0, repeats, 1, 0, 1, dstRepeatStride, 1, src0RepeatStride);
    }
};

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TROWEXPANDSUB_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1) {
    using T = typename TileDataDst::DType;
    static_assert(std::is_same_v<typename TileDataDst::DType, typename TileDataSrc0::DType> &&
        std::is_same_v<typename TileDataDst::DType, typename TileDataSrc1::DType>,
        "Fix: TROWEXPANDSUB src and dst data type is different!");
    static_assert(
        std::is_same_v<typename TileDataDst::DType, half> || std::is_same_v<typename TileDataDst::DType, float>,
        "Fix: TROWEXPANDSUB Invalid data type.");
    constexpr bool src0eqdst = std::is_same_v<TileDataDst, TileDataSrc0>;
    constexpr bool src1eqdst = std::is_same_v<TileDataDst, TileDataSrc1>;
    static_assert(TileDataDst::isRowMajor && (src0eqdst || src1eqdst), "Fix: TROWEXPANDSUB Invalid tile shape.");
    constexpr unsigned rowStride = TileDataDst::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();
    if constexpr (src0eqdst) {
        unsigned src1ValidCol = src1.GetValidCol();
        PTO_ASSERT(((TileDataSrc1::isRowMajor && src1ValidCol == 32 / sizeof(T)) ||
                    (!TileDataSrc1::isRowMajor && src1ValidCol == 1)) &&
                    src1.GetValidRow() == validRow, "TROWEXPANDSUB: invalid src1 shape.");
        TRowExpandBin<RowExpandSubOp<T>, TileDataDst, TileDataSrc1, rowStride>(dst.data(), src0.data(), src1.data(), validRow, validCol);
    } else  {
        unsigned src0ValidCol = src0.GetValidCol();
        PTO_ASSERT(((TileDataSrc0::isRowMajor && src0ValidCol == 32 / sizeof(T)) ||
                    (!TileDataSrc0::isRowMajor && src0ValidCol == 1)) &&
                    src0.GetValidRow() == validRow, "TROWEXPANDSUB: invalid src0 shape.");
        TRowExpandBin<RowExpandSubOp2<T>, TileDataDst, TileDataSrc0, rowStride>(dst.data(), src1.data(), src0.data(), validRow, validCol);
    }
}
} // namespace pto
#endif