/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWEXPAND_HPP
#define TROWEXPAND_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {

template <typename TileDataOut, typename TileDataIn>
PTO_INTERNAL void TRowExpandCheck(unsigned srcValidRow, unsigned srcValidCol,unsigned dstValidRow) {
    static_assert((sizeof(typename TileDataIn::DType) == 1) || (sizeof(typename TileDataIn::DType) == 2) ||
                  (sizeof(typename TileDataIn::DType) == 4), "Fix: TROWEXPAND data type must be b8/b16/b32");
    static_assert(TileDataIn::Loc == pto::TileType::Vec, "Fix: TROWEXPAND Src TileType must be Vec Tile!");
    static_assert(TileDataOut::Loc == pto::TileType::Vec, "Fix: TROWEXPAND Dst TileType must be Vec Tile!");
    static_assert(TileDataIn::SFractal == SLayout::NoneBox,
      "Fix: TROWEXPAND only support Nd or Dn fractal Tile");
    static_assert(TileDataOut::isRowMajor && TileDataOut::SFractal == SLayout::NoneBox,
      "Fix: TROWEXPAND only support Nd fractal Tile");
    static_assert(std::is_same_v<typename TileDataOut::DType, typename TileDataIn::DType>,
      "Fix: TROWEXPAND input data type must be consistent with the output data type.");
    PTO_ASSERT(srcValidRow == dstValidRow,
        "Fix: TROWEXPAND input valid row must be consistent with the output valid row.");
    PTO_ASSERT(srcValidRow != 0 && srcValidCol != 0,
        "Fix: TROWEXPAND input shape is invalid, validCol or validRow is 0.");
}

template <typename T, unsigned DstStride, unsigned SrcStride>
PTO_INTERNAL void TRowExpandInstr_NoPostUpdate(__ubuf__ T* dstPtr, __ubuf__ T* srcPtr, unsigned dstValidRow,
                                               unsigned dstValidCol, uint16_t repeatTimes, uint16_t eleCntValue) {
    constexpr auto distValue = 
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    __VEC_SCOPE__
    {
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        MaskReg preg;
        __ubuf__ T* dstOffset;
        uint32_t sreg = eleCntValue;
        MaskReg pg0 = CreatePredicate<T>(sreg);
        for (uint16_t i = 0; i < (uint16_t)dstValidRow; i++) {
            vlds(vreg0, srcPtr, i * SrcStride, NORM);
            vdup(vreg1, vreg0, pg0, POS_LOWEST, MODE_ZEROING);
            sreg = (uint32_t)(dstValidCol);
            dstOffset = dstPtr + i * DstStride;
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; j++) {
                preg = CreatePredicate<T>(sreg);
                vsts(vreg1, dstOffset, (int32_t)(j * eleCntValue), distValue, preg);
            }
        }
    }
}

template <typename T, typename TileDataOut, typename TileDataIn>
PTO_INTERNAL constexpr bool needBrcb() {
    /*
        isBroadcastSupportType:
            Only 16bit and 32bit are supported.
        isStaticShape:
            Broadcast is a special case where the src tile is a single row or column,
            src and dst tile are static shapes to ensure that the tile data is saved continuously.
        isBroadcast:
            [M, 1] -> [M, elemPerBlock], src is column major.
            The value of sizeof(T) x M is a multiple of 32Byte, it also means that M must be a multiple of 8,
            this constraint is implemented by the Tile basic definition.
    */
    constexpr bool isBroadcastSupportType = (sizeof(T) == 2 || sizeof(T) == 4);
    constexpr bool isStaticShape =
        (TileDataIn::Rows == TileDataIn::ValidRow) && (TileDataOut::Rows == TileDataOut::ValidRow) &&
        (TileDataIn::Cols == TileDataIn::ValidCol) && (TileDataOut::Cols == TileDataOut::ValidCol);

    constexpr unsigned elemPerBlock = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr bool isBroadcast = (!TileDataIn::isRowMajor) && (TileDataIn::Cols == 1) &&
                                 (TileDataIn::Rows == TileDataOut::Rows) && (TileDataOut::Cols == elemPerBlock);
    return (isBroadcastSupportType && isStaticShape && isBroadcast);
}

template <typename T, typename TileDataOut, typename TileDataIn>
PTO_INTERNAL void TRowExpandBrcb(__ubuf__ T *dst, __ubuf__ T *src) {
    constexpr auto nElemPerVlds = CCE_VL / BLOCK_BYTE_SIZE;
    constexpr auto nElemPerVsts = CCE_VL / sizeof(T);
    constexpr auto distValue = 
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    uint16_t repeatTimes = TileDataIn::Numel / nElemPerVlds;
    if constexpr (sizeof(T) == 4) {
        __VEC_SCOPE__
        {
            RegTensor<T> vreg;
            MaskReg preg;
            uint32_t sreg = TileDataOut::Numel;
            for (uint16_t i = 0; i < (uint16_t)repeatTimes; i++) {
                preg = CreatePredicate<T>(sreg);
                vlds(vreg, src, (int32_t)(i * nElemPerVlds), E2B_B32);
                vsts(vreg, dst, (int32_t)(i * nElemPerVsts), distValue, preg);
            }
        }
    } else {
        __VEC_SCOPE__
        {
            RegTensor<T> vreg;
            MaskReg preg;
            uint32_t sreg = TileDataOut::Numel;
            for (uint16_t i = 0; i < (uint16_t)repeatTimes; i++) {
                preg = CreatePredicate<T>(sreg);
                vlds(vreg, src, (int32_t)(i * nElemPerVlds), E2B_B16);
                vsts(vreg, dst, (int32_t)(i * nElemPerVsts), distValue, preg);
            }
        }
    }
}

template <typename T, typename TileDataOut, typename TileDataIn>
PTO_INTERNAL void TRowExpandInstr_PostUpdate(__ubuf__ T* dstPtr, __ubuf__ T* srcPtr, unsigned dstValidRow,
                                             unsigned dstValidCol, uint16_t repeatTimes, uint16_t eleCntValue) {
    if constexpr (needBrcb<T, TileDataOut, TileDataIn>()) {
        TRowExpandBrcb<T, TileDataOut, TileDataIn>(dstPtr, srcPtr);
    } else {
        constexpr auto distValue = 
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        __VEC_SCOPE__
        {
            RegTensor<T> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            uint32_t sreg = eleCntValue;
            MaskReg pg0 = CreatePredicate<T>(sreg);
            for (uint16_t i = 0; i < (uint16_t)dstValidRow; i++) {
                vlds(vreg0, srcPtr, TileDataIn::RowStride, NORM, POST_UPDATE);
                vdup(vreg1, vreg0, pg0, POS_LOWEST, MODE_ZEROING);
                sreg = (uint32_t)(dstValidCol);
                for (uint16_t j = 0; j < (uint16_t)repeatTimes; j++) {
                    preg = CreatePredicate<T>(sreg);
                    vsts(vreg1, dstPtr, (int32_t)(eleCntValue), distValue, preg, POST_UPDATE);
                }
                dstPtr += (TileDataOut::RowStride - repeatTimes * eleCntValue);
            }
        }
    }
}

template <typename TileDataOut, typename TileDataIn>
__tf__ PTO_INTERNAL OP_NAME(TROWEXPAND) OP_TYPE(broadcast)
void TRowExpand(typename TileDataOut::TileDType __out__ dst,
                                  typename TileDataIn::TileDType __in__ src,
                                  unsigned dstValidRow,
                                  unsigned dstValidCol,
                                  unsigned version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename TileDataOut::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
    constexpr unsigned nRepeatElem = REPEAT_BYTE / sizeof(typename TileDataIn::DType);
    uint16_t repeatTimes = CeilDivision(dstValidCol, nRepeatElem);
    switch (version) {
        case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
        case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
            TRowExpandInstr_NoPostUpdate<T, TileDataOut::RowStride, TileDataIn::RowStride>(dstPtr, srcPtr, dstValidRow,
                dstValidCol, repeatTimes, nRepeatElem);
            break;
        default:
            TRowExpandInstr_PostUpdate<T, TileDataOut, TileDataIn>(dstPtr, srcPtr, dstValidRow, dstValidCol,
                repeatTimes, nRepeatElem);
            break;
    }
}

template <typename TileDataOut, typename TileDataIn>
PTO_INTERNAL void TROWEXPAND_IMPL(TileDataOut &dst, TileDataIn &src)
{
    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();
    TRowExpandCheck<TileDataOut, TileDataIn>(src.GetValidRow(), src.GetValidCol(), dstValidRow);
    TRowExpand<TileDataOut, TileDataIn>(dst.data(), src.data(), dstValidRow, dstValidCol);
}
}  // namespace pto
#endif
