/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TSEL_HPP
#define TSEL_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "utils.hpp"

namespace pto {

#define TILE_PTRS(dst, selmask, src0, src1)                                                                        \
    using T = typename TileData::DType;                                                                            \
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);                                                    \
    __ubuf__ typename MaskTile::DType *maskPtr = (__ubuf__ typename MaskTile::DType *)__cce_get_tile_ptr(selmask); \
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);                                                  \
    __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1)

template <typename T, unsigned elementsPerRepeat, uint16_t unRollConstant, unsigned rowStride, unsigned maskStride>
PTO_INTERNAL void TSelHead(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, __ubuf__ uint8_t *maskPtr,
    uint16_t pairedRepeatTimes, unsigned validRow, unsigned validCol) {
    MaskReg preg, selMask0, selMask1, tmpMask0;
    MaskReg tmpMask1 = pset_b16(PAT_ALL);
    RegTensor<T> vreg0, vreg1, vreg2, vreg3, dreg0, dreg1, dreg2;

    constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();

    for (uint16_t i = 0; i < (uint16_t)(validRow); ++i) {
        for (uint16_t j = 0; j < (uint16_t)(pairedRepeatTimes); ++j) {
            uint16_t repeatIdx = j * unRollConstant;
            uint32_t colOffset0 = repeatIdx * elementsPerRepeat;
            uint32_t colOffset1 = colOffset0 + elementsPerRepeat;

            vlds(vreg0, src0Ptr, (int32_t)(i * rowStride + colOffset0), NORM);
            vlds(vreg1, src1Ptr, (int32_t)(i * rowStride + colOffset0), NORM);
            plds(tmpMask0, (__ubuf__ uint32_t *)maskPtr, i * maskStride + repeatIdx * 8, US);
            pintlv_b16(selMask0, selMask1, tmpMask0, tmpMask1);

            vsel(dreg0, vreg0, vreg1, selMask0);

            uint32_t count0 =
                ((colOffset0 + elementsPerRepeat) >= validCol ? validCol - colOffset0 : elementsPerRepeat);
            preg = CreatePredicate<T>(count0);

            vsts(dreg0, dstPtr, (int32_t)(i * rowStride + colOffset0), distValue, preg);

            vlds(vreg2, src0Ptr, (int32_t)(i * rowStride + colOffset1), NORM);
            vlds(vreg3, src1Ptr, (int32_t)(i * rowStride + colOffset1), NORM);
            vsel(dreg1, vreg2, vreg3, selMask1);
            uint32_t count1 =
                ((colOffset1 + elementsPerRepeat) >= validCol ? validCol - colOffset1 : elementsPerRepeat);
            preg = CreatePredicate<T>(count1);
            vsts(dreg1, dstPtr, (int32_t)(i * rowStride + colOffset1), distValue, preg);
        }
    }
}

template <typename T, unsigned elementsPerRepeat, unsigned rowStride, unsigned maskStride>
PTO_INTERNAL void TSelTail(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, __ubuf__ uint8_t *maskPtr,
    uint16_t repeatIdx, uint16_t remainRepeat, unsigned validRow, unsigned validCol) {
    MaskReg preg, selMask2;
    MaskReg tmpMask1 = pset_b16(PAT_ALL);
    RegTensor<T> vreg4, vreg5, dreg2;
    constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    for (uint16_t i = 0; i < (uint16_t)(validRow); ++i) {
        for (uint16_t j = 0; j < (uint16_t)(remainRepeat); ++j) {
            uint32_t colOffset = (repeatIdx + j) * elementsPerRepeat;
            uint32_t count = (validCol > colOffset) ? (validCol - colOffset) : 0;
            preg = CreatePredicate<T>(count);

            plds(selMask2, (__ubuf__ uint32_t *)maskPtr, i * maskStride + (repeatIdx + j) * 8, US);
            punpack(selMask2, selMask2, LOWER);

            vlds(vreg4, src0Ptr, (int32_t)(i * rowStride + colOffset), NORM);
            vlds(vreg5, src1Ptr, (int32_t)(i * rowStride + colOffset), NORM);
            vsel(dreg2, vreg4, vreg5, selMask2);
            vsts(dreg2, dstPtr, (int32_t)(i * rowStride + colOffset), distValue, preg);
        }
    }
}

template <typename TileData, typename MaskTile, unsigned elementsPerRepeat, unsigned rowStride, unsigned maskStride>
__tf__ PTO_INTERNAL void TSel_b32(typename TileData::TileDType __out__ dst, typename MaskTile::TileDType __in__ selmask,
    typename TileData::TileDType __in__ src0, typename TileData::TileDType __in__ src1, unsigned validRow,
    unsigned validCol) {
    TILE_PTRS(dst, selmask, src0, src1);
    uint16_t repeatTimes = CeilDivision(validCol, elementsPerRepeat);
    constexpr uint32_t unRollConstant = 2;
    uint16_t pairedRepeatTimes = repeatTimes / unRollConstant;
    uint16_t remainRepeat = repeatTimes % unRollConstant;
    uint16_t repeatIdx = pairedRepeatTimes * unRollConstant;

    __VEC_SCOPE__ {
        TSelHead<T, elementsPerRepeat, unRollConstant, rowStride, maskStride>(
            dstPtr, src0Ptr, src1Ptr, maskPtr, pairedRepeatTimes, validRow, validCol);
        TSelTail<T, elementsPerRepeat, rowStride, maskStride>(
            dstPtr, src0Ptr, src1Ptr, maskPtr, repeatIdx, remainRepeat, validRow, validCol);
    } // end of vf
}

template <typename TileData, typename MaskTile, unsigned elementsPerRepeat, unsigned rowStride, unsigned maskStride>
__tf__ PTO_INTERNAL void TSel_b16_8(typename TileData::TileDType __out__ dst,
    typename MaskTile::TileDType __in__ selmask, typename TileData::TileDType __in__ src0,
    typename TileData::TileDType __in__ src1, unsigned validRow, unsigned validCol) {
    TILE_PTRS(dst, selmask, src0, src1);
    uint16_t repeatTimes = CeilDivision(validCol, elementsPerRepeat);
    __VEC_SCOPE__ {
        MaskReg preg, maskreg;
        RegTensor<T> vreg0, vreg1, vreg2;
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)validRow; ++i) {
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                vlds(vreg0, src0Ptr, i * rowStride + j * elementsPerRepeat, NORM);
                vlds(vreg1, src1Ptr, i * rowStride + j * elementsPerRepeat, NORM);
                if (sizeof(T) == 2) {
                    plds(maskreg, (__ubuf__ uint32_t *)maskPtr, i * maskStride + j * 16, US);
                } else {
                    plds(maskreg, (__ubuf__ uint32_t *)maskPtr, i * maskStride + j * 16, NORM);
                }
                uint32_t count =
                    ((j + 1) * elementsPerRepeat >= validCol ? validCol - j * elementsPerRepeat : elementsPerRepeat);
                preg = CreatePredicate<T>(count);
                vsel(vreg2, vreg0, vreg1, maskreg);
                vsts(vreg2, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    } // end of vf
}

template <typename TileData, typename MaskTile>
PTO_INTERNAL void TSEL_IMPL(TileData &dst, MaskTile &selMask, TileData &src0, TileData &src1) {
    static_assert(TileData::isRowMajor, "Fix: TSEL has not supported layout type.");
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileData::DType);
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();
    constexpr unsigned rowStride = TileData::RowStride;
    constexpr unsigned maskStride = MaskTile::RowStride;
    if (sizeof(typename TileData::DType) == 4) {
        TSel_b32<TileData, MaskTile, elementsPerRepeat, rowStride, maskStride>(
            dst.data(), selMask.data(), src0.data(), src1.data(), validRow, validCol);
    } else {
        TSel_b16_8<TileData, MaskTile, elementsPerRepeat, rowStride, maskStride>(
            dst.data(), selMask.data(), src0.data(), src1.data(), validRow, validCol);
    }
}
} // namespace pto
#endif