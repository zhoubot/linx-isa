/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TADD_HPP
#define TADD_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <pto/npu/a5/common.hpp>
#include <pto/npu/a5/utils.hpp>
#include <pto/npu/a5/TBinOp.hpp>
#include <pto/common/debug.h>

namespace pto {

template <typename T>
struct AddOp {
    PTO_INTERNAL static void BinInstr(
        RegTensor<T> &reg_dst, RegTensor<T> &reg_src0, RegTensor<T> &reg_src1, MaskReg &preg) {
        vadd(reg_dst, reg_src0, reg_src1, preg, MODE_ZEROING);
    }
};

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned src0RowStride = dstRowStride, unsigned src1RowStride = dstRowStride>
__tf__ PTO_INTERNAL OP_NAME(TADD) OP_TYPE(element_wise) void TAdd(typename TileData::TileDType __out__ dst,
    typename TileData::TileDType __in__ src0, typename TileData::TileDType __in__ src1, unsigned validRows,
    unsigned validCols, VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
    if constexpr (dstRowStride == src0RowStride && dstRowStride == src1RowStride) {
        BinaryInstr<AddOp<T>, TileData, elementsPerRepeat, blockSizeElem, dstRowStride>(
            dstPtr, src0Ptr, src1Ptr, validRows, validCols, version);
    } else {
        BinaryInstr<AddOp<T>, TileData, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride, src1RowStride>(
            dstPtr, src0Ptr, src1Ptr, validRows, validCols, version);
    }
    return;
}

template <typename T, typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TAddCheck(const TileDataDst &dst, const TileDataSrc0 &src0, const TileDataSrc1 &src1) {
    static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, float> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, half> ||
                      std::is_same_v<T, bfloat16_t> || std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>,
        "Fix: TAdd has invalid data type.");
    static_assert(TileDataDst::isRowMajor && TileDataSrc0::isRowMajor && TileDataSrc1::isRowMajor,
        "Fix: TAdd only support row major layout.");
    unsigned validRows = dst.GetValidRow();
    unsigned validCols = dst.GetValidCol();
    PTO_ASSERT(src0.GetValidRow() == validRows && src0.GetValidCol() == validCols,
        "Fix: TADD input tile src0 valid shape mismatch with output tile dst shape.");
    PTO_ASSERT(src1.GetValidRow() == validRows && src1.GetValidCol() == validCols,
        "Fix: TADD input tile src1 valid shape mismatch with output tile dst shape.");
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TADD_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1) {
    using T = typename TileDataDst::DType;
    TAddCheck<T, TileDataDst, TileDataSrc0, TileDataSrc1>(dst, src0, src1);
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    // when tileshape of src0, src1 and dst are the same, validRows and validCols are also the same
    if constexpr (std::is_same_v<TileDataDst, TileDataSrc0> && std::is_same_v<TileDataDst, TileDataSrc1>) {
        constexpr unsigned dstRowStride = TileDataDst::RowStride;
        TAdd<TileDataDst, elementsPerRepeat, blockSizeElem, dstRowStride>(
            dst.data(), src0.data(), src1.data(), dst.GetValidRow(), dst.GetValidCol());
    } else {
        // when tileshape of src0, src1 and dst are different, validRows and validCols are also the same
        constexpr unsigned dstRowStride = TileDataDst::RowStride;
        constexpr unsigned src0RowStride = TileDataSrc0::RowStride;
        constexpr unsigned src1RowStride = TileDataSrc1::RowStride;
        TAdd<TileDataDst, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride, src1RowStride>(
            dst.data(), src0.data(), src1.data(), dst.GetValidRow(), dst.GetValidCol());
    }
}
} // namespace pto
#endif
