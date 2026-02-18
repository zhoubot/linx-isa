/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWEXPANDBIN_HPP
#define TROWEXPANDBIN_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>

namespace pto {

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TRowExpandBinOps_2D_NoPostUpdate(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidCols, elementsPerRepeat);
    constexpr unsigned stride = TileDataSrc::Cols;

    __VEC_SCOPE__
    {
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;
        RegTensor<T> vreg_uld;
        MaskReg preg;
        vector_bool preg_b8_all = pset_b8(PAT_ALL);
        vector_align ureg_1;
        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(kValidRows); ++i) {
            vldas(ureg_1, (__ubuf__ T*)(src1Ptr + i*stride));
            vldus(vreg_uld, ureg_1, (__ubuf__ T*)(src1Ptr + i*stride));
            vdup(vreg1, vreg_uld, preg_b8_all, POS_LOWEST, MODE_ZEROING);
            uint32_t sreg = (uint32_t)(kValidCols);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, src0Ptr,  i * rowStride + j * elementsPerRepeat, NORM);
                Op::RowExpandBinaryInstr(vreg2, vreg0, vreg1, preg);
                vsts(vreg2, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TRowExpandBinOps_2D_NoPostUpdate2(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidCols, elementsPerRepeat);
    constexpr unsigned stride = TileDataSrc::Cols;

    __VEC_SCOPE__
    {
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;
        MaskReg preg;
        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(kValidRows); ++i) {
            uint32_t sreg = (uint32_t)(kValidCols);
            vlds(vreg1, src1Ptr,  i * blockSizeElem, BLK);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, src0Ptr,  i * rowStride + j * elementsPerRepeat, NORM);
                Op::RowExpandBinaryInstr(vreg2, vreg0, vreg1, preg);
                vsts(vreg2, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL
void TRowExpandBinOps_2D_PostUpdate(__ubuf__ typename TileData::DType *dstPtr, 
                    __ubuf__ typename TileData::DType *src0Ptr, 
                    __ubuf__ typename TileDataSrc::DType *src1Ptr,
                    unsigned kValidRows,
                    unsigned kValidCols) {
    using T = typename TileData::DType;
    uint16_t repeatTimes = CeilDivision(kValidCols, elementsPerRepeat);
    constexpr unsigned stride = TileDataSrc::Cols;

    __VEC_SCOPE__
    {
        RegTensor<T> vreg0_PU;
        RegTensor<T> vreg1_PU;
        RegTensor<T> vreg2_PU;
        RegTensor<T> vreg_PU_uld;
        MaskReg preg;
        vector_bool preg_b8_all = pset_b8(PAT_ALL);
        vector_align ureg_1;
        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(kValidRows); ++i) {
            vldas(ureg_1, (__ubuf__ T*)(src1Ptr + i*stride));
            vldus(vreg_PU_uld, ureg_1, (__ubuf__ T*)(src1Ptr + i*stride));
            vdup(vreg1_PU, vreg_PU_uld, preg_b8_all, POS_LOWEST, MODE_ZEROING);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                vlds(vreg0_PU, src0Ptr,  i * rowStride + j * elementsPerRepeat, NORM);
                uint32_t count = ((j + 1) * elementsPerRepeat >= kValidCols ? kValidCols - j * elementsPerRepeat : elementsPerRepeat);
                preg = CreatePredicate<T>(count);
                Op::RowExpandBinaryInstr(vreg2_PU, vreg0_PU, vreg1_PU, preg);
                vsts(vreg2_PU, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

template <typename Op, typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL void RowExpandBinaryInstr(__ubuf__ typename TileData::DType *dstPtr, 
                              __ubuf__ typename TileData::DType *src0Ptr, 
                              __ubuf__ typename TileDataSrc::DType *src1Ptr,
                              unsigned kValidRows, unsigned kValidCols) {
    if constexpr (TileDataSrc::isRowMajor) {
        if constexpr (TileData::ValidCol == TileData::Cols) {
            TRowExpandBinOps_2D_NoPostUpdate2<Op, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dstPtr, src0Ptr, src1Ptr, kValidRows, kValidCols);
        } else {
            TRowExpandBinOps_2D_NoPostUpdate2<Op, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dstPtr, src0Ptr, src1Ptr, kValidRows, kValidCols);
        }
    } else {
        if constexpr (TileData::ValidCol == TileData::Cols) {
            TRowExpandBinOps_2D_NoPostUpdate<Op, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dstPtr, src0Ptr, src1Ptr, kValidRows, kValidCols);
        } else {
            TRowExpandBinOps_2D_NoPostUpdate<Op, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dstPtr, src0Ptr, src1Ptr, kValidRows, kValidCols);
        }
    }
}

}  // namespace pto
#endif