/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TFILLPAD_HPP
#define TFILLPAD_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <pto/npu/a5/utils.hpp>
#include "TLoad.hpp"

namespace pto {

template <typename T, typename U>
PTO_INTERNAL MaskReg PSetTyped(U dist) {
    if constexpr (sizeof(T) == sizeof(float)) {
        return pset_b32(dist);
    } else if constexpr (sizeof(T) == sizeof(half)) {
        return pset_b16(dist);
    } else if constexpr (sizeof(T) == sizeof(uint8_t)) {
        return pset_b8(dist);
    }
}

template <typename T, typename DistType>
PTO_INTERNAL void CopyValidElementsVec(__ubuf__ T *dstPtr, __ubuf__ T *srcPtr, uint64_t srcValidRow,
    uint64_t srcValidCol, unsigned srcStride, unsigned dstStride, DistType distValue) {
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    RegTensor<T> vreg0;
    MaskReg preg;
    uint16_t repeatTimes = CeilDivision(srcValidCol, elementsPerRepeat);
    for (uint16_t i = 0; i < (uint16_t)(srcValidRow); ++i) {
        uint32_t sreg = (uint32_t)(srcValidCol);
        for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
            preg = CreatePredicate<T>(sreg);
            vlds(vreg0, srcPtr + i * srcStride, j * elementsPerRepeat, NORM);
            vsts(vreg0, dstPtr + i * dstStride, j * elementsPerRepeat, distValue, preg);
        }
    }
}

template <typename TileDataDst, typename TileDataSrc, bool inplace>
__tf__ PTO_INTERNAL void TFillPad(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, uint64_t dstValidRow, uint64_t dstValidCol, uint64_t srcValidRow,
    uint64_t srcValidCol) {
    using T = typename TileDataSrc::DType;
    using U = typename TileDataDst::DType;
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
    __ubuf__ U *dstPtr = (__ubuf__ U *)__cce_get_tile_ptr(dst);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataSrc::DType);
    constexpr unsigned srcStride = TileDataSrc::Cols;
    constexpr unsigned dstStride = TileDataDst::Cols;
    unsigned padCols = TileDataDst::Cols - srcValidCol;
    unsigned padRows = dstValidRow - srcValidRow;
    auto uint_pv = GetPadValue<TileDataDst>();
    T padValue;
    *(T *)&padValue = *((T *)&uint_pv);

    static_assert(sizeof(T) == sizeof(U), "Fix: TFillPad src and dst data type is different!");
    __VEC_SCOPE__ {
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        if constexpr (!inplace) {
            CopyValidElementsVec<T>(dstPtr, srcPtr, srcValidRow, srcValidCol, srcStride, dstStride, distValue);
        }
        uint16_t padRepeatTimes = CeilDivision(padCols, elementsPerRepeat);
        RegTensor<T> vreg_pad0;
        UnalignReg ureg;
        MaskReg pg_all = PSetTyped<T>(PAT_ALL);
        vdup(vreg_pad0, padValue, pg_all, MODE_ZEROING);
        for (uint16_t i = 0; i < (uint16_t)(srcValidRow); ++i) {
            uint32_t cols = (uint32_t)(padCols);
            __ubuf__ T *pdst = dstPtr + i * dstStride + srcValidCol;
            for (uint16_t j = 0; j < (uint16_t)padRepeatTimes; ++j) {
                uint32_t sreg = cols > elementsPerRepeat ? elementsPerRepeat : cols;
                vstus(ureg, sreg, vreg_pad0, pdst, POST_UPDATE);
                cols -= elementsPerRepeat;
            }
            vstas(ureg, pdst, 0, POST_UPDATE);
        }
        __ubuf__ T *pdst = dstPtr + srcValidRow * dstStride;
        uint32_t pad1d = (uint32_t)(padRows * dstStride);
        padRepeatTimes = CeilDivision(pad1d, elementsPerRepeat);
        MaskReg preg1d;
        for (uint16_t j = 0; j < (uint16_t)padRepeatTimes; ++j) {
            preg1d = CreatePredicate<T>(pad1d);
            vsts(vreg_pad0, pdst, j * elementsPerRepeat, distValue, preg1d);
        }
    } // end VF

} // end of tf

template <typename TileDataDst, typename TileDataSrc, bool inplace>
PTO_INTERNAL void TFILLPAD_GENERIC_IMPL(TileDataDst &dst, TileDataSrc &src) {
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataSrc::DType);
    constexpr unsigned srcStride = TileDataSrc::RowStride;
    constexpr unsigned dstStride = TileDataDst::RowStride;
    uint64_t validSrcRow = src.GetValidRow();
    uint64_t validSrcCol = src.GetValidCol();
    uint64_t validDstRow = dst.GetValidRow();
    uint64_t validDstCol = dst.GetValidCol();

    using T = typename TileDataSrc::DType;
    using U = typename TileDataDst::DType;
    static_assert(TileDataDst::PadVal != PadValue::Null, "Fix: TFillPad dst vecTile pad value must not be Null!");
    static_assert(sizeof(T) == sizeof(U), "Fix: TFillPad src and dst data type is different!");
    static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "Fix: TFillPad has invalid data type.");

    TFillPad<TileDataDst, TileDataSrc, inplace>(
        dst.data(), src.data(), validDstRow, validDstCol, validSrcRow, validSrcCol);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TFILLPAD_IMPL(TileDataDst &dst, TileDataSrc &src) {
    static_assert(TileDataDst::Cols == TileDataSrc::Cols && TileDataDst::Rows == TileDataSrc::Rows,
        "Fix: TFillPad Dst/Src vecTile Rows/Cols must be the same.");

    TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, false>(dst, src);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TFILLPAD_INPLACE_IMPL(TileDataDst &dst, TileDataSrc &src) {
    static_assert(TileDataDst::Cols == TileDataSrc::Cols && TileDataDst::Rows == TileDataSrc::Rows,
        "Fix: TFillPad Dst vecTile Rows/Cols must be greater or equal to src vecTile.");

    TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, true>(dst, src);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TFILLPAD_EXPAND_IMPL(TileDataDst &dst, TileDataSrc &src) {
    static_assert(TileDataDst::Cols >= TileDataSrc::Cols && TileDataDst::Rows >= TileDataSrc::Rows,
        "Fix: TFillPad Dst/Src vecTile Rows/Cols must be the same.");

    TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, false>(dst, src);
}

template <typename TileData>
__tf__ PTO_INTERNAL void TFillPad(typename TileData::TileDType __out__ dst, uint32_t dstValidRow, uint32_t dstValidCol)
{
    using U = typename TileData::DType;
    __cbuf__ U *dstPtr = (__cbuf__ U *)__cce_get_tile_ptr(dst);
    constexpr uint32_t elementsPerBlock = C0_SIZE_BYTE / sizeof(U);
    uint32_t alignedValidCol = CeilAlignment(dstValidCol, elementsPerBlock);

#if defined(__DAV_CUBE__)
    uint16_t blockLen = TileData::Rows - dstValidRow; // unit is 32B
    uint16_t repeat = alignedValidCol / elementsPerBlock;
    uint16_t repeatGap = dstValidRow;

    int64_t repeatConfig =
        (static_cast<uint64_t>(blockLen) << 16) |  // [30:16] is the block number of each repeat
        (static_cast<uint64_t>(repeatGap) << 32) | // [46:32] is the repeat gap between two consecutive repeats
        static_cast<uint64_t>(repeat);             // [14:0] is the repeat times
    if (blockLen != 0) {
        create_cbuf_matrix((__cbuf__ uint16_t *)(dstPtr + dstValidRow * elementsPerBlock), repeatConfig, 0);
    }
    if (alignedValidCol != TileData::Cols) { // if alignedValidCol is not equal to TileData::Cols, need to pad the left column
        blockLen = TileData::Rows;        // unit is 32B
        repeatConfig = (static_cast<uint64_t>(blockLen) << 16) | // [30:16] is the block number of each repeat
                       (static_cast<uint64_t>(0) << 32) | 1;     // [46:32] is the repeat gap
        create_cbuf_matrix((__cbuf__ uint16_t *)(dstPtr + TileData::Rows * alignedValidCol), repeatConfig, 0);
    }
#endif
}

template <typename TileData, PadValue PadVal = PadValue::Zero>
PTO_INTERNAL void TFILLPAD_IMPL(TileData &dst, TileData &src)
{
    static_assert(!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor),
        "Fix: TFillPad Dst matTile now only support NZ layout.");
    static_assert(TileData::PadVal == PadValue::Zero || TileData::PadVal == PadValue::Null,
        "Fix: TFillPad dst matTile pad value only support Zero or Null!");
    using T = typename TileData::DType;
    static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "Fix: TFillPad type must be b4/b8/b16/b32.");

    uint32_t validDstRow = dst.GetValidRow();
    uint32_t validDstCol = dst.GetValidCol();
    TFillPad<TileData>(dst.data(), validDstRow, validDstCol);
}


} // namespace pto
#endif
