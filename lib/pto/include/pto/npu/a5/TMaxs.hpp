/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TMAXS_HPP
#define TMAXS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TBinSOp.hpp"

namespace pto {

template <typename T> struct MaxSOp {
    PTO_INTERNAL static void BinSInstr(RegTensor<T> &reg_dst, RegTensor<T> &reg_src, T scalar, MaskReg &preg)
    {
        vmaxs(reg_dst, reg_src, scalar, preg, MODE_ZEROING);
    }
};

template <typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride, unsigned srcRowStride>
__tf__ PTO_INTERNAL OP_NAME(TMAXS) OP_TYPE(element_wise)
void TMaxS(typename TileDataDst::TileDType __out__ dst, 
           typename TileDataSrc::TileDType __in__ src, 
           typename TileDataSrc::DType scalar,
           unsigned kValidRows, unsigned kValidCols,
           VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename TileDataDst::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
    BinaryInstr<MaxSOp<T>, TileDataDst, TileDataSrc, T, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>(
                dstPtr, srcPtr, scalar, kValidRows, kValidCols, version);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TMAXS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
{
    using T = typename TileDataDst::DType;
    static_assert((TileDataDst::Loc == TileType::Vec) && (TileDataSrc::Loc == TileType::Vec),
                  "TileType of dst and src tiles must be TileType::Vec.");
    static_assert((TileDataDst::ValidCol <= TileDataDst::Cols) && (TileDataDst::ValidRow <= TileDataDst::Rows),
                  "Number of valid columns and rows must not be greater than number of tile columns and rows.");
    static_assert((TileDataSrc::ValidCol <= TileDataSrc::Cols) && (TileDataSrc::ValidRow <= TileDataSrc::Rows),
                  "Number of valid columns and rows must not be greater than number of tile columns and rows.");

    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    constexpr unsigned dstRowStride = TileDataDst::RowStride;
    constexpr unsigned srcRowStride = TileDataSrc::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    PTO_ASSERT(src.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");

    TMaxS<TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>
        (dst.data(), src.data(), scalar, validRow, validCol);
}
}  // namespace pto
#endif
