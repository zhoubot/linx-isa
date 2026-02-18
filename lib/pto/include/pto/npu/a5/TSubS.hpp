/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSUBS_HPP
#define TSUBS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TBinSOp.hpp"

namespace pto {

template <typename T> struct SubSOp {
    PTO_INTERNAL static void BinSInstr(RegTensor<T> &reg_dst, RegTensor<T> &reg_src, T scalar, MaskReg &preg)
    {
        vadds(reg_dst, reg_src, -scalar, preg, MODE_ZEROING);
    }
};

template <typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride, unsigned src0RowStride>
__tf__ PTO_INTERNAL OP_NAME(TSUBS) OP_TYPE(element_wise)
void TSubS(typename TileDataDst::TileDType __out__ dst, 
           typename TileDataSrc::TileDType __in__ src0, 
           typename TileDataSrc::DType src1,
           unsigned kValidRows,
           unsigned kValidCols,
           VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename TileDataDst::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    BinaryInstr<SubSOp<T>, TileDataDst, TileDataSrc, T, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride>(
                dstPtr, src0Ptr, src1, kValidRows, kValidCols, version);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TSUBS_IMPL(TileDataDst &dst, TileDataSrc &src0, typename TileDataSrc::DType src1)
{
    using T = typename TileDataDst::DType;
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value || std::is_same<T, int16_t>::value ||
                  std::is_same<T, half>::value || std::is_same<T, float16_t>::value ||
                  std::is_same<T, float>::value || std::is_same<T, float32_t>::value,
                  "TSUBS: Invalid data type");
    static_assert((TileDataDst::Loc == TileType::Vec) && (TileDataSrc::Loc == TileType::Vec),
                  "TileType of dst and src tiles must be TileType::Vec.");
    static_assert((TileDataDst::ValidCol <= TileDataDst::Cols) && (TileDataDst::ValidRow <= TileDataDst::Rows) &&
                  (TileDataSrc::ValidCol <= TileDataSrc::Cols) && (TileDataSrc::ValidRow <= TileDataSrc::Rows),
                  "Number of valid columns and rows must not be greater than number of tile columns and rows.");

    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    constexpr unsigned dstRowStride = TileDataDst::RowStride;
    constexpr unsigned src0RowStride = TileDataSrc::RowStride;

    PTO_ASSERT(src0.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");
    PTO_ASSERT(src0.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    TSubS<TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride>
        (dst.data(), src0.data(), src1, validRow, validCol);
}
}  // namespace pto
#endif
