/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TGATHERB_HPP
#define TGATHERB_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset, unsigned elementsPerRepeat,
    unsigned blockSizeElem, unsigned dstRowStride, unsigned offsetRowStride>
__tf__ PTO_INTERNAL void TGatherBRowWise(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, typename TileDataOffset::TileDType __in__ offset, unsigned validRow,
    unsigned validCol, uint16_t repeatTimes, uint32_t remainEleNum) {
    using T = typename TileDataDst::DType;
    __ubuf__ typename TileDataSrc::DType *srcAddr = (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src);
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    uint16_t lastRepeat = repeatTimes - 1;
    uint32_t count = elementsPerRepeat;
    constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    __VEC_SCOPE__ {
        __ubuf__ uint32_t *offsetPtr = (__ubuf__ uint32_t *)__cce_get_tile_ptr(offset);
        MaskReg preg0 = CreatePredicate<T>(count);
        MaskReg preg1 = CreatePredicate<T>(remainEleNum);
        RegTensor<uint32_t> vregOffset;
        RegTensor<T> vregDst;
        for (uint16_t i = 0; i < (uint16_t)validRow; ++i) {
            uint32_t perRowOffset = i * offsetRowStride;
            uint32_t perRowDstOffset = i * dstRowStride;
            for (uint16_t j = 0; j < (uint16_t)lastRepeat; ++j) {
                vlds(vregOffset, offsetPtr, (perRowOffset + j * 8), NORM);
                vgatherb(vregDst, srcAddr, vregOffset, preg0);
                vsts(vregDst, dstPtr, (perRowDstOffset + j * elementsPerRepeat), NORM_B32, preg0);
            }
            vlds(vregOffset, offsetPtr, (perRowOffset + lastRepeat * 8), NORM);
            vgatherb(vregDst, srcAddr, vregOffset, preg1);
            vsts(vregDst, dstPtr, (perRowDstOffset + lastRepeat * elementsPerRepeat), NORM_B32, preg1);
        }
    }
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset, unsigned elementsPerRepeat,
    unsigned blockSizeElem, unsigned dstRowStride, unsigned offsetRowStride>
__tf__ PTO_INTERNAL void TGatherBColWise(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, typename TileDataOffset::TileDType __in__ offset, unsigned validRow,
    unsigned validCol, uint16_t repeatTimes, uint32_t remainEleNum) {
    using T = typename TileDataDst::DType;
    __ubuf__ uint32_t *offsetPtr = (__ubuf__ uint32_t *)__cce_get_tile_ptr(offset);
    __ubuf__ typename TileDataSrc::DType *srcAddr = (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src);
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    uint32_t count = elementsPerRepeat;
    uint16_t lastRepeat = repeatTimes - 1;
    constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    __VEC_SCOPE__ {
        MaskReg preg0 = CreatePredicate<T>(count);
        MaskReg preg1 = CreatePredicate<T>(remainEleNum);
        RegTensor<uint32_t> vregOffset;
        RegTensor<T> vregDst;
        for (uint16_t i = 0; i < (uint16_t)lastRepeat; i++) {
            uint32_t perRowOffset = i * 8;
            uint32_t perRowDstOffset = i * elementsPerRepeat;
            for (uint16_t j = 0; j < (uint16_t)validRow; j++) {
                vlds(vregOffset, offsetPtr, (perRowOffset + j * offsetRowStride), NORM);
                vgatherb(vregDst, srcAddr, vregOffset, preg0);
                vsts(vregDst, dstPtr, (perRowDstOffset + j * dstRowStride), distValue, preg0);
            }
        }
        for (uint16_t j = 0; j < (uint16_t)validRow; j++) {
            vlds(vregOffset, offsetPtr, (lastRepeat * 8 + j * offsetRowStride), NORM);
            vgatherb(vregDst, srcAddr, vregOffset, preg1);
            vsts(vregDst, dstPtr, (lastRepeat * elementsPerRepeat + j * dstRowStride), distValue, preg1);
        }
    }
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset>
PTO_INTERNAL void TGATHERB_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataOffset &offset) {
    static_assert(sizeof(typename TileDataDst::DType) == 4 || sizeof(typename TileDataDst::DType) == 2 ||
                      sizeof(typename TileDataDst::DType) == 1,
        "Fix: TGATHERB has invalid data type.");
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType);
    constexpr unsigned staticRepeatTimes = (TileDataDst::Cols + elementsPerRepeat - 1) / elementsPerRepeat;
    constexpr unsigned dstRowStride = TileDataDst::RowStride;
    constexpr unsigned offsetRowStride = TileDataOffset::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();
    uint16_t repeatTimes = CeilDivision(validCol, elementsPerRepeat);
    uint32_t remainEleNum = validCol % elementsPerRepeat ?: elementsPerRepeat;
    if constexpr (staticRepeatTimes > TileDataDst::Rows) {
        TGatherBRowWise<TileDataDst, TileDataSrc, TileDataOffset, elementsPerRepeat, blockSizeElem, dstRowStride,
            offsetRowStride>(dst.data(), src.data(), offset.data(), validRow, validCol, repeatTimes, remainEleNum);
    } else {
        TGatherBColWise<TileDataDst, TileDataSrc, TileDataOffset, elementsPerRepeat, blockSizeElem, dstRowStride,
            offsetRowStride>(dst.data(), src.data(), offset.data(), validRow, validCol, repeatTimes, remainEleNum);
    }
}
} // namespace pto

#endif