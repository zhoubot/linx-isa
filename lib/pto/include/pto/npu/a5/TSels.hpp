/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSELS_HPP
#define TSELS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "utils.hpp"

namespace pto {
template <typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, int dstCols>
PTO_INTERNAL void TSelsNoPadImpl(
    __ubuf__ T *dstPtr,
    __ubuf__ T *src0Ptr,
    __ubuf__ T *src1Ptr,
    uint8_t selectMode,
    unsigned validRow
) {
    __VEC_SCOPE__
    {
        MaskReg maskReg;
        MaskReg preg;
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;
        if (selectMode == 1) {
            maskReg = pset_b8(PAT_ALL);
        } else {
            maskReg = pset_b8(PAT_ALLF);
        }
        uint32_t sreg = (uint32_t)(validRow * dstCols);
        uint16_t repeatTimes = CeilDivision(validRow * dstCols, elementsPerRepeat);
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < repeatTimes; ++i) {
            preg = CreatePredicate<T>(sreg);
            vlds(vreg0, src0Ptr, elementsPerRepeat, NORM, POST_UPDATE);
            vlds(vreg1, src1Ptr, elementsPerRepeat, NORM, POST_UPDATE);
            vsel(vreg2, vreg0, vreg1, maskReg);
            vsts(vreg2, dstPtr, elementsPerRepeat, distValue, preg, POST_UPDATE);
        }
    } // end of VF
}

template <typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, int dstCols>
PTO_INTERNAL void TSelsPadImpl(
    __ubuf__ T *dstPtr,
    __ubuf__ T *src0Ptr,
    __ubuf__ T *src1Ptr,
    uint8_t selectMode,
    unsigned validRow,
    unsigned validCol
) {
    __VEC_SCOPE__
    {
        MaskReg maskReg;
        MaskReg preg;
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;
        if (selectMode == 1) {
            maskReg = pset_b8(PAT_ALL);
        } else {
            maskReg = pset_b8(PAT_ALLF);
        }
        uint16_t repeatTimes = CeilDivision(validCol, elementsPerRepeat);
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(validRow); ++i) {
            uint32_t sreg = (uint32_t)(validCol);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, src0Ptr + i * dstCols, j * elementsPerRepeat, NORM);
                vlds(vreg1, src1Ptr + i * dstCols, j * elementsPerRepeat, NORM);
                vsel(vreg2, vreg0, vreg1, maskReg);
                vsts(vreg2, dstPtr + i * dstCols, j * elementsPerRepeat, distValue, preg);
            }
        }
    } // end VF
}

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem>
__tf__ PTO_INTERNAL OP_NAME(TSELS) OP_TYPE(element_wise)
void TSelsImpl(
    typename TileData::TileDType __out__ dst,
    typename TileData::TileDType __in__ src0,
    typename TileData::TileDType __in__ src1,
    uint8_t selectMode,
    unsigned validRow,
    unsigned validCol,
    unsigned version = VFImplKind::VFIMPL_DEFAULT
) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
    if constexpr (TileData::PadVal == PadValue::Null || TileData::PadVal == PadValue::Zero) {
        TSelsNoPadImpl<T, elementsPerRepeat, blockSizeElem, TileData::Cols>(dstPtr, src0Ptr, src1Ptr, selectMode, validRow);
    } else { // -INF(MIN) or INF(MAX)
        TSelsPadImpl<T, elementsPerRepeat, blockSizeElem, TileData::Cols>(dstPtr, src0Ptr, src1Ptr, selectMode, validRow, validCol);
    }
}

template <typename TileData>
PTO_INTERNAL void TSELS_IMPL(TileData &dst, TileData &src0, TileData &src1, uint8_t selectMode)
{
    using T = typename TileData::DType;
    static_assert(std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value ||
                  std::is_same<T, int32_t>::value || std::is_same<T, half>::value ||
                  std::is_same<T, float32_t>::value || std::is_same<T, uint8_t>::value ||
                  std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value,
                  "TSELS: Invalid data type");
    static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "TSELS: Invalid data type.");
    static_assert(TileData::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
    static_assert(TileData::isRowMajor, "TSELS: not supported Layout type");
    static_assert(TileData::ValidCol <= TileData::Cols, "Number of valid columns must not be greater than number of tile columns.");
    static_assert(TileData::ValidRow <= TileData::Rows, "Number of valid rows must not be greater than number of tile rows.");

    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    PTO_ASSERT(src0.GetValidCol() == src1.GetValidCol(), "Number of columns of src0, src1 must be the same.");
    PTO_ASSERT(src1.GetValidCol() == dst.GetValidCol(), "Number of columns of src1 and dst must be the same.");
    PTO_ASSERT(src0.GetValidRow() == src1.GetValidRow(), "Number of rows of src0, src1 must be the same.");
    PTO_ASSERT(src1.GetValidRow() == dst.GetValidRow(), "Number of rows of src1 and dst must be the same.");

    TSelsImpl<TileData, elementsPerRepeat, blockSizeElem>(dst.data(), src0.data(), src1.data(), selectMode, validRow, validCol);
}
}  // namespace pto
#endif
