/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TBIN_HPP
#define TBIN_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <pto/npu/a5/common.hpp>
#include <pto/npu/a5/utils.hpp>

namespace pto {
template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL void TBinOps_1D_NoPostUpdate(
    __ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRows, unsigned validCols) {
    uint16_t repeatTimes = CeilDivision(validRows * validCols, elementsPerRepeat);
    __VEC_SCOPE__ {
        RegTensor<T> vreg0, vreg1, vreg2;
        MaskReg preg;

        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        unsigned sreg = validRows * validCols;
#pragma clang loop unroll(disable)
        for (uint16_t i = 0; i < (uint16_t)repeatTimes; ++i) {
            preg = CreatePredicate<T>(sreg);
            vlds(vreg0, src0Ptr, i * elementsPerRepeat, NORM);
            vlds(vreg1, src1Ptr, i * elementsPerRepeat, NORM);
            Op::BinInstr(vreg2, vreg0, vreg1, preg);
            vsts(vreg2, dstPtr, i * elementsPerRepeat, distValue, preg);
        }
    }
}

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL void TBinOps_1D_PostUpdate(
    __ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRows, unsigned validCols) {
    uint16_t repeatTimes = CeilDivision(validRows * validCols, elementsPerRepeat);
    __VEC_SCOPE__ {
        RegTensor<T> vreg0_PU, vreg1_PU, vreg2_PU;
        MaskReg preg;

        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        unsigned sreg = validRows * validCols;
#pragma clang loop unroll(disable)
        for (uint16_t i = 0; i < (uint16_t)repeatTimes; ++i) {
            preg = CreatePredicate<T>(sreg);
            vlds(vreg0_PU, src0Ptr, elementsPerRepeat, NORM, POST_UPDATE);
            vlds(vreg1_PU, src1Ptr, elementsPerRepeat, NORM, POST_UPDATE);
            Op::BinInstr(vreg2_PU, vreg0_PU, vreg1_PU, preg);
            vsts(vreg2_PU, dstPtr, elementsPerRepeat, distValue, preg, POST_UPDATE);
        }
    }
}

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned src0RowStride = dstRowStride, unsigned src1RowStride = dstRowStride>
PTO_INTERNAL void TBinOps_2D_NoPostUpdate(
    __ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRows, unsigned validCols) {
    uint16_t repeatTimes = CeilDivision(validCols, elementsPerRepeat);

    __VEC_SCOPE__ {
        RegTensor<T> vreg0, vreg1, vreg2;
        MaskReg preg;
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(validRows); ++i) {
            uint32_t sreg = (uint32_t)(validCols);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, src0Ptr, i * src0RowStride + j * elementsPerRepeat, NORM);
                vlds(vreg1, src1Ptr, i * src1RowStride + j * elementsPerRepeat, NORM);
                Op::BinInstr(vreg2, vreg0, vreg1, preg);
                vsts(vreg2, dstPtr, i * dstRowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned src0RowStride = dstRowStride, unsigned src1RowStride = dstRowStride>
PTO_INTERNAL void TBinOps_2D_PostUpdate(
    __ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRows, unsigned validCols) {
    uint16_t repeatTimes = CeilDivision(validCols, elementsPerRepeat);

    __VEC_SCOPE__ {
        RegTensor<T> vreg0_PU, vreg1_PU, vreg2_PU;
        MaskReg preg;
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        for (uint16_t i = 0; i < (uint16_t)(validRows); ++i) {
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j) {
                vlds(vreg0_PU, src0Ptr, i * src0RowStride + j * elementsPerRepeat, NORM);
                vlds(vreg1_PU, src1Ptr, i * src1RowStride + j * elementsPerRepeat, NORM);
                uint32_t count =
                    ((j + 1) * elementsPerRepeat >= validCols ? validCols - j * elementsPerRepeat : elementsPerRepeat);
                preg = CreatePredicate<T>(count);
                Op::BinInstr(vreg2_PU, vreg0_PU, vreg1_PU, preg);
                vsts(vreg2_PU, dstPtr, i * dstRowStride + j * elementsPerRepeat, distValue, preg);
            }
        }
    }
}

// implement the template for tileshape of src0, src1 and dst are same
template <typename Op, typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
PTO_INTERNAL void BinaryInstr(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src0,
    typename TileData::TileDType __in__ src1, unsigned validRows, unsigned validCols, VFImplKind version) {
    using T = typename TileData::DType;
    if constexpr ((TileData::ValidCol == TileData::Cols) || (TileData::Rows == 1)) {
        switch (version) {
            case VFImplKind::VFIMPL_DEFAULT:
                TBinOps_1D_PostUpdate<Op, T, elementsPerRepeat, blockSizeElem, rowStride>(
                    dst, src0, src1, validRows, validCols);
                break;
            case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
            case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
                TBinOps_1D_NoPostUpdate<Op, T, elementsPerRepeat, blockSizeElem, rowStride>(
                    dst, src0, src1, validRows, validCols);
                break;
            case VFImplKind::VFIMPL_1D_POST_UPDATE:
            case VFImplKind::VFIMPL_2D_POST_UPDATE:
            default:
                TBinOps_1D_PostUpdate<Op, T, elementsPerRepeat, blockSizeElem, rowStride>(
                    dst, src0, src1, validRows, validCols);
                break;
        }
    } else {
        switch (version) {
            case VFImplKind::VFIMPL_DEFAULT:
                TBinOps_2D_PostUpdate<Op, T, elementsPerRepeat, blockSizeElem, rowStride>(
                    dst, src0, src1, validRows, validCols);
                break;
            case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
            case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
                TBinOps_2D_NoPostUpdate<Op, T, elementsPerRepeat, blockSizeElem, rowStride>(
                    dst, src0, src1, validRows, validCols);
                break;
            default:
                TBinOps_2D_PostUpdate<Op, T, elementsPerRepeat, blockSizeElem, rowStride>(
                    dst, src0, src1, validRows, validCols);
                break;
        }
    }
}

// implement the template for tileshape of src0, src1 and dst are different
template <typename Op, typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned src0RowStride, unsigned src1RowStride>
PTO_INTERNAL void BinaryInstr(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src0,
    typename TileData::TileDType __in__ src1, unsigned validRows, unsigned validCols, VFImplKind version) {
    using T = typename TileData::DType;
    switch (version) {
        case VFImplKind::VFIMPL_DEFAULT:
            TBinOps_2D_PostUpdate<Op, T, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride, src1RowStride>(
                dst, src0, src1, validRows, validCols);
            break;
        case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
        case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
            TBinOps_2D_NoPostUpdate<Op, T, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride,
                src1RowStride>(dst, src0, src1, validRows, validCols);
            break;
        case VFImplKind::VFIMPL_1D_POST_UPDATE:
        case VFImplKind::VFIMPL_2D_POST_UPDATE:
        default:
            TBinOps_2D_PostUpdate<Op, T, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride, src1RowStride>(
                dst, src0, src1, validRows, validCols);
            break;
    }
}

} // namespace pto
#endif
