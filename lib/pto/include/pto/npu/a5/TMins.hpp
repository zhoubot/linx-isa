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
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TBinSOp.hpp"

namespace pto {

template <typename T> struct MinSOp {
    PTO_INTERNAL static void BinSInstr(RegTensor<T> &reg_dst, RegTensor<T> &reg_src0, T src1, MaskReg &preg)
    {
        vmins(reg_dst, reg_src0, src1, preg, MODE_ZEROING);
    }
};

template <typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride, unsigned srcRowStride>
__tf__ PTO_INTERNAL OP_NAME(TMINS) OP_TYPE(element_wise)
void TMinS(typename TileDataDst::TileDType __out__ dst, 
           typename TileDataSrc::TileDType __in__ src0, 
           typename TileDataSrc::DType src1,
           unsigned kValidRows,
           unsigned kValidCols,
           VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename TileDataDst::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    BinaryInstr<MinSOp<T>, TileDataDst, TileDataSrc, T, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>(
                dstPtr, src0Ptr, src1, kValidRows, kValidCols, version);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TMINS_IMPL(TileDataDst &dst, TileDataSrc &src0, typename TileDataSrc::DType src1)
{
    using T = typename TileDataDst::DType;
    static_assert(TileDataDst::Loc == TileType::Vec, "TileType of dst tiles must be TileType::Vec.");
    static_assert(TileDataDst::ValidCol <= TileDataDst::Cols,
                  "Number of valid columns must not be greater than number of tile columns.");
    static_assert(TileDataDst::ValidRow <= TileDataDst::Rows,
                  "Number of valid rows must not be greater than number of tile rows.");
    static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src tiles must be TileType::Vec.");
    static_assert(TileDataSrc::ValidCol <= TileDataSrc::Cols,
                  "Number of valid columns must not be greater than number of tile columns.");
    static_assert(TileDataSrc::ValidRow <= TileDataSrc::Rows,
                  "Number of valid rows must not be greater than number of tile rows.");

    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    constexpr unsigned dstRowStride = TileDataDst::RowStride;
    constexpr unsigned srcRowStride = TileDataSrc::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    PTO_ASSERT(src0.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");

    TMinS<TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>(dst.data(), src0.data(), src1, validRow, validCol);
}
}  // namespace pto
#endif
