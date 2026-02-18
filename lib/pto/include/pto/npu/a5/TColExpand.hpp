/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLEXPAND_HPP
#define TCOLEXPAND_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {
    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TColExpandCheck(unsigned srcValidRow, unsigned srcValidCol, unsigned dstValidCol)
    {
        static_assert(std::is_same_v<typename TileDataDst::DType, typename TileDataSrc::DType>, 
            "Fix: TCOLEXPAND input data type must be consistent with the output data type.");
        static_assert((sizeof(typename TileDataSrc::DType) == 1) || (sizeof(typename TileDataSrc::DType) == 2) ||
                      (sizeof(typename TileDataSrc::DType) == 4), "Fix: TCOLEXPAND data type must be b8/b16/b32");
        static_assert(TileDataDst::Loc == pto::TileType::Vec, "Fix: TCOLEXPAND Dst TileType must be Vec Tile!");
        static_assert(TileDataSrc::Loc == pto::TileType::Vec, "Fix: TCOLEXPAND Src TileType must be Vec Tile!");
        static_assert(TileDataDst::isRowMajor && TileDataDst::SFractal == SLayout::NoneBox,
            "Fix: TCOLEXPAND only support Nd fractal Tile.");
        static_assert(TileDataSrc::isRowMajor && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TCOLEXPAND only support Nd fractal Tile.");
        PTO_ASSERT(srcValidCol == dstValidCol,
            "Fix: TCOLEXPAND input valid col must be consistent with output valid col.");
        PTO_ASSERT(srcValidRow != 0 && srcValidCol != 0, 
            "Fix: TCOLEXPAND input shape in invalid, validCol or validRow is 0.");
    }

    template <typename T, unsigned DstStride>
    PTO_INTERNAL void TColExpandInstr_NoPostUpdate(__ubuf__ T* dstPtr, __ubuf__ T* srcPtr, unsigned dstValidRow, 
        unsigned dstValidCol, uint16_t repeatTimes, uint16_t eleCntValue) {
        
        constexpr auto distValue = 
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        __VEC_SCOPE__
        {
        RegTensor<T> vreg0;
        __ubuf__ T* dstOffset;
        MaskReg preg;
        uint32_t sreg = dstValidCol;

        for (uint16_t i = 0; i < (uint16_t)dstValidRow; i++) {
            sreg = (uint32_t)(dstValidCol);
            dstOffset = dstPtr + i * DstStride;
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; j++) {
                vlds(vreg0, srcPtr, j * eleCntValue, NORM);
                preg = CreatePredicate<T>(sreg);
                vsts(vreg0, dstOffset, (uint32_t)(j * eleCntValue), distValue, preg);
            }
        }
        }
    }

    template <typename T, unsigned DstStride>
    PTO_INTERNAL void TColExpandInstr_PostUpdate(__ubuf__ T* dstPtr, __ubuf__ T* srcPtr, unsigned dstValidRow, 
        unsigned dstValidCol, uint16_t repeatTimes, uint16_t eleCntValue) {
        
        constexpr auto distValue = 
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        __VEC_SCOPE__
        {
        RegTensor<T> vreg0;
        __ubuf__ T* dstOffset;
        MaskReg preg;

        for (uint16_t j = 0; j < (uint16_t)repeatTimes; j++) {
            vlds(vreg0, srcPtr, eleCntValue, NORM, POST_UPDATE);
            preg = CreatePredicate<T>(dstValidCol);
            dstOffset = dstPtr + j * eleCntValue;
            for (uint16_t i = 0; i < (uint16_t)dstValidRow; i++) {
                vsts(vreg0, dstOffset, (uint32_t)DstStride, distValue, preg, POST_UPDATE);
            }
        }
    }
    }

    template <typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeEle>
    __tf__ PTO_INTERNAL void TColExpand(typename TileDataDst::TileDType __out__ dst,
                                        typename TileDataSrc::TileDType __in__ src,
                                        unsigned dstValidRow,
                                        unsigned dstValidCol,
                                        unsigned version = VFImplKind::VFIMPL_DEFAULT)
    {
        using T = typename TileDataDst::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
        constexpr uint16_t eleCntValue = elementsPerRepeat;
        uint16_t repeatTimes = CeilDivision(dstValidCol, eleCntValue);

        switch (version)
        {
        case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
        case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
            TColExpandInstr_NoPostUpdate<T, TileDataDst::Cols>(dstPtr, srcPtr, dstValidRow, dstValidCol, repeatTimes, eleCntValue);
        case VFImplKind::VFIMPL_1D_POST_UPDATE:
        case VFImplKind::VFIMPL_2D_POST_UPDATE:
            TColExpandInstr_PostUpdate<T, TileDataDst::Cols>(dstPtr, srcPtr, dstValidRow, dstValidCol, repeatTimes, eleCntValue);
        default:
            TColExpandInstr_PostUpdate<T, TileDataDst::Cols>(dstPtr, srcPtr, dstValidRow, dstValidCol, repeatTimes, eleCntValue);
        }
    }


    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TCOLEXPAND_IMPL(TileDataDst &dst, TileDataSrc &src) 
    {
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType);
        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
        unsigned dstValidRow = dst.GetValidRow();
        unsigned dstValidCol = dst.GetValidCol();
        TColExpandCheck<TileDataDst, TileDataSrc>(src.GetValidRow(), src.GetValidCol(), dstValidCol);
        TColExpand<TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem>(
            dst.data(), src.data(), dstValidRow, dstValidCol);
    }
}
#endif