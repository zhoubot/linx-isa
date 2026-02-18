/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TEXPANDS_HPP
#define TEXPANDS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TBinSOp.hpp"

namespace pto {

template <typename T> struct ExpandSOp {
    PTO_INTERNAL static void BinSInstr(RegTensor<T> &reg_dst, RegTensor<T> &reg_src0, T scalar, MaskReg &preg)
    {
        vdup(reg_dst, scalar, preg, MODE_ZEROING);
    }
};

template <typename TileDataDst, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
__tf__ PTO_INTERNAL void TExpandS(
        typename TileDataDst::TileDType __out__ dst,
        typename TileDataDst::DType scalar,
        unsigned kValidRows,
        unsigned kValidCols,
        VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename TileDataDst::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    BinaryInstr<ExpandSOp<T>, TileDataDst, TileDataDst, T, elementsPerRepeat, blockSizeElem, rowStride, rowStride>(
                dstPtr, nullptr, scalar, kValidRows, kValidCols, version);
}

template <typename TileDataDst>
PTO_INTERNAL void TEXPANDS_IMPL(TileDataDst &dst, typename TileDataDst::DType scalar)
{
    using T = typename TileDataDst::DType;
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value || std::is_same<T, int>::value ||
                      std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value ||
                      std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value ||
                      std::is_same<T, half>::value || std::is_same<T, float16_t>::value ||
                      std::is_same<T, float>::value || std::is_same<T, float32_t>::value,
        "TEXPANDS: Invalid data type");
    static_assert(TileDataDst::Loc == TileType::Vec, "Location of src and dst tiles must be Location::Vec.");
    static_assert(TileDataDst::ValidCol <= TileDataDst::Cols,
        "Number of valid columns must not be greater than number of tile columns.");
    static_assert(TileDataDst::ValidRow <= TileDataDst::Rows,
        "Number of valid rows must not be greater than number of tile rows.");

    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    constexpr unsigned rowStride = TileDataDst::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    TExpandS<TileDataDst, elementsPerRepeat, blockSizeElem, rowStride>(dst.data(), scalar, validRow, validCol);
}
}  // namespace pto
#endif
