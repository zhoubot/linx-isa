/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPARTIALMAX_HPP
#define TPARTIALMAX_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <pto/npu/a2a3/TPartOp.hpp>

namespace pto {
template <typename T>
struct PartMaxOp {
    PTO_INTERNAL static void PartInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t repeats)
    {
        vmax(dst, src0, src1, repeats, 1, 1, 1, 8, 8, 8);
    }
    PTO_INTERNAL static void PartInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t repeats,
        uint8_t dstRepeatStride, uint8_t src0RepeatStride, uint8_t src1RepeatStride)
    {
        vmax(dst, src0, src1, repeats, 1, 1, 1, dstRepeatStride, src0RepeatStride, src1RepeatStride);
    }
};

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1, unsigned elementsPerRepeat,
          unsigned blockSizeElem, unsigned dstRowStride, unsigned src0RowStride, unsigned src1RowStride>
__tf__
PTO_INTERNAL
void TPartMax(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc0::TileDType __in__ src0, typename TileDataSrc1::TileDType __in__ src1, unsigned src0ValidRow,
    unsigned src0ValidCol, unsigned src1ValidRow, unsigned src1ValidCol, unsigned dstValidRow, unsigned dstValidCol)
{
    using T = typename TileDataDst::DType;
    bool condSrc0EqDst = (src0ValidRow == dstValidRow && src0ValidCol == dstValidCol);
    bool condSrc1EqDst = (src1ValidRow == dstValidRow && src1ValidCol == dstValidCol);
    PTO_ASSERT(condSrc0EqDst || condSrc1EqDst,
        "Fix: TPARTMAX At most one entry in the valid-rows and valid-cols of src0 and src1 is smaller than dst.");
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
    constexpr int dstCol = TileDataDst::Cols;
    constexpr int src0Col = TileDataSrc0::Cols;
    constexpr int src1Col = TileDataSrc1::Cols;
    constexpr int dstRow = TileDataDst::Rows;
    if (condSrc0EqDst) {
        TPartInstr<PartMaxOp<T>, T, dstCol, src0Col, dstRow, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride,
             src1RowStride>(dstPtr, src0Ptr, src1Ptr, src0ValidRow, src0ValidCol, src1ValidRow, src1ValidCol,
             dstValidRow, dstValidCol);
    } else if (condSrc1EqDst) {
        TPartInstr<PartMaxOp<T>, T, dstCol, src1Col, dstRow, elementsPerRepeat, blockSizeElem, dstRowStride, src1RowStride,
             src0RowStride>(dstPtr, src1Ptr, src0Ptr, src1ValidRow, src1ValidCol, src0ValidRow, src0ValidCol,
             dstValidRow, dstValidCol);
    }
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TPARTMAX_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1)
{
    static_assert(std::is_same_v<typename TileDataDst::DType, typename TileDataSrc0::DType> &&
                  std::is_same_v<typename TileDataDst::DType, typename TileDataSrc1::DType>,
                  "Fix: TPARTMAX src and dst data type is different!");
    static_assert(std::is_same_v<typename TileDataDst::DType, int32_t> ||
                  std::is_same_v<typename TileDataDst::DType, int> ||
                  std::is_same_v<typename TileDataDst::DType, int16_t> ||
                  std::is_same_v<typename TileDataDst::DType, half> ||
                  std::is_same_v<typename TileDataDst::DType, float16_t> ||
                  std::is_same_v<typename TileDataDst::DType, float> ||
                  std::is_same_v<typename TileDataDst::DType, float32_t>,
                  "Fix: TPARTMAX Invalid data type.");
    static_assert(TileDataDst::isRowMajor && TileDataSrc0::isRowMajor && TileDataSrc1::isRowMajor,
                  "Fix: TPARTMAX not supported BLayout type.");
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType);
    unsigned src0ValidRow = src0.GetValidRow();
    unsigned src0ValidCol = src0.GetValidCol();
    unsigned src1ValidRow = src1.GetValidRow();
    unsigned src1ValidCol = src1.GetValidCol();
    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();
    if (dstValidRow == 0 || dstValidCol == 0) {
        return;
    }
    constexpr unsigned dstRowStride = TileDataDst::RowStride;
    constexpr unsigned src0RowStride = TileDataSrc0::RowStride;
    constexpr unsigned src1RowStride = TileDataSrc1::RowStride;

    TPartMax<TileDataDst, TileDataSrc0, TileDataSrc1, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride,
        src1RowStride>(dst.data(), src0.data(), src1.data(), src0ValidRow, src0ValidCol, src1ValidRow, src1ValidCol,
        dstValidRow, dstValidCol);
}
}  // namespace pto
#endif
