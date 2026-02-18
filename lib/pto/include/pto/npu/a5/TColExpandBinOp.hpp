/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLEXPANDBIN_HPP
#define TCOLEXPANDBIN_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>

namespace pto {
template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TColExpandBinOps_1D_NoPostUpdate(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidRows * kValidCols, elementsPerRepeat);
    __VEC_SCOPE__
    {
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;
        MaskReg  preg;

        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        unsigned sreg = kValidRows * kValidCols;
        #pragma clang loop unroll(disable)
        for (uint16_t i = 0; i < (uint16_t)repeatTimes; ++i) {
            vlds(vreg1, src1Ptr, i*elementsPerRepeat, NORM); 
            preg = CreatePredicate<T>(sreg);
            vlds(vreg0, src0Ptr, i*elementsPerRepeat, NORM);
            Op::ColExpandBinaryInstr(vreg2, vreg0, vreg1, preg);
            vsts(vreg2, dstPtr, i*elementsPerRepeat, distValue, preg);
        }
    }
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TColExpandBinOps_1D_PostUpdate(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidRows * kValidCols, elementsPerRepeat);
    __VEC_SCOPE__
    {
        RegTensor<T> vreg0_PU;
        RegTensor<T> vreg1_PU;
        RegTensor<T> vreg2_PU;
        MaskReg  preg;

        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        unsigned sreg = kValidRows * kValidCols;
        #pragma clang loop unroll(disable)
        for (uint16_t i = 0; i < (uint16_t)repeatTimes; ++i) {
            vlds(vreg1_PU, src1Ptr, i*elementsPerRepeat, NORM); 
            preg = CreatePredicate<T>(sreg);
            vlds(vreg0_PU, src0Ptr, elementsPerRepeat, NORM, POST_UPDATE);
            Op::ColExpandBinaryInstr(vreg2_PU, vreg0_PU, vreg1_PU, preg);
            vsts(vreg2_PU, dstPtr, elementsPerRepeat, distValue, preg, POST_UPDATE);
        }
    } 
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TColExpandBinOps_2D_NoPostUpdate(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidCols, elementsPerRepeat);

    __VEC_SCOPE__
    {
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;
        MaskReg preg;
        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(kValidRows); ++i) {
            uint32_t sreg = (uint32_t)(kValidCols);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                vlds(vreg1, src1Ptr, j*elementsPerRepeat, NORM);
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, src0Ptr,  i * rowStride + j * elementsPerRepeat, NORM);
                Op::ColExpandBinaryInstr(vreg2, vreg0, vreg1, preg);
                vsts(vreg2, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TColExpandBinOps_2D_PostUpdate(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidCols, elementsPerRepeat);

    __VEC_SCOPE__
    {
        RegTensor<T> vreg0_PU;
        RegTensor<T> vreg1_PU;
        RegTensor<T> vreg2_PU;
        MaskReg preg;
        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(kValidRows); ++i) {
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                vlds(vreg0_PU, src0Ptr,  i * rowStride + j * elementsPerRepeat, NORM);
                vlds(vreg1_PU, src1Ptr, j*elementsPerRepeat, NORM);
                uint32_t count = ((j + 1) * elementsPerRepeat >= kValidCols ? kValidCols - j * elementsPerRepeat : elementsPerRepeat);
                preg = CreatePredicate<T>(count);
                Op::ColExpandBinaryInstr(vreg2_PU, vreg0_PU, vreg1_PU, preg);
                vsts(vreg2_PU, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL void ColExpandBinaryInstr(__ubuf__ typename TileData::DType *dstPtr, 
                              __ubuf__ typename TileData::DType *src0Ptr, 
                              __ubuf__ typename TileDataSrc::DType *src1Ptr,
                              unsigned kValidRows, unsigned kValidCols) {
    if constexpr (TileData::ValidCol== TileData::Cols) {
        TColExpandBinOps_1D_PostUpdate<Op, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dstPtr, src0Ptr, src1Ptr, kValidRows, kValidCols);
    } else {
        TColExpandBinOps_2D_NoPostUpdate<Op, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dstPtr, src0Ptr, src1Ptr, kValidRows, kValidCols);
    }
}

}  // namespace pto
#endif