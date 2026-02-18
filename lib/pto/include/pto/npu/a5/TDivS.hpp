/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TDIVS_HPP
#define TDIVS_HPP

#include <pto/common/constants.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TBinSOp.hpp"


namespace pto {

    template <typename T> struct DivSOp {
        PTO_INTERNAL static void BinSInstr(RegTensor<T> &vregdst, RegTensor<T> &vregsrc, T src1, MaskReg &preg)
        {
            float divider = static_cast<float>(src1);
            if (divider != 0.0f)
            {
                divider = 1.0f / divider;
            }
            else
            {
                divider = 1.0 / 0.0;
            }
            if constexpr (std::is_same<T, float>::value)
            {
                vmuls(vregdst, vregsrc, divider, preg);
            }
            else if constexpr (std::is_same<T, half>::value)
            {
                vdup(vregdst, src1, preg, MODE_ZEROING);
                vdiv(vregdst, vregsrc, vregdst, preg);
            }
            else if constexpr (std::is_same<T, int32_t>::value)
            {
                RegTensor<float> tempDst;
                vcvt(tempDst, vregsrc, preg, ROUND_R);
                vmuls(tempDst, tempDst, divider, preg);
                vcvt(vregdst, tempDst, preg, ROUND_Z, RS_ENABLE);
            }
            else if constexpr (std::is_same<T, int16_t>::value)
            {
                RegTensor<half> tempDst;
                vcvt(tempDst, vregsrc, preg, ROUND_R);
                vmuls(tempDst, tempDst, divider, preg);
                vcvt(vregdst, tempDst, preg, ROUND_Z, RS_ENABLE);
            }
        }
    };

        template <typename T> struct DivSOpS {
        PTO_INTERNAL static void BinSInstr(RegTensor<T> &vregdst, RegTensor<T> &vregsrc, T src0, MaskReg &preg)
        {
            if constexpr (std::is_same<T, float>::value || std::is_same<T, half>::value)
            {
                vdup(vregdst, src0, preg, MODE_ZEROING);
                vdiv(vregdst, vregdst, vregsrc, preg);
            }
            else if constexpr (std::is_same<T, int32_t>::value)
            {
                RegTensor<float> tempDst;
                RegTensor<float> tempSrc;
                vdup(vregdst, src0, preg, MODE_ZEROING);
                vcvt(tempDst, vregdst, preg, ROUND_R);
                vcvt(tempSrc, vregsrc, preg, ROUND_R);
                vdiv(tempDst, tempDst, tempSrc, preg);
                vcvt(vregdst, tempDst, preg, ROUND_Z, RS_ENABLE);
            }
            else if constexpr (std::is_same<T, int16_t>::value)
            {
                RegTensor<half> tempDst;
                RegTensor<half> tempSrc;
                vdup(vregdst, src0, preg, MODE_ZEROING);
                vcvt(tempDst, vregdst, preg, ROUND_R);
                vcvt(tempSrc, vregsrc, preg, ROUND_R);
                vdiv(tempDst, tempDst, tempSrc, preg);
                vcvt(vregdst, tempDst, preg, ROUND_Z, RS_ENABLE);
            }
        }
    };
    template <typename T, unsigned DstCols, unsigned SrcCols>
    PTO_INTERNAL void TDivs_naive(__ubuf__ T *dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        PtoSetWaitFlag<PIPE_V, PIPE_S>();
        for (int i = 0; i < validRow; i++) {
            for (int j = 0; j < validCol; j++) {
                int dstOffset = i * DstCols + j;
                int srcOffset = i * SrcCols + j;
                dst[dstOffset] = src0[srcOffset] / src1;
            }
        }
    }

    template <typename T, unsigned DstCols, unsigned SrcCols>
    PTO_INTERNAL void TSDiv_naive(__ubuf__ T *dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        PtoSetWaitFlag<PIPE_V, PIPE_S>();
        for (int i = 0; i < validRow; i++) {
            for (int j = 0; j < validCol; j++) {
                int dstOffset = i * DstCols + j;
                int srcOffset = i * SrcCols + j;
                dst[dstOffset] = src1 / src0[srcOffset];
            }
        }
    }
    template <typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride, unsigned srcRowStride>
    __tf__ PTO_INTERNAL OP_NAME(TDIVS) OP_TYPE(element_wise)
    void TDivS(typename TileDataDst::TileDType __out__ dst,
                                typename TileDataSrc::TileDType __in__ src0, 
                                typename TileDataSrc::DType __in__ src1, 
                                unsigned validRow,
                                unsigned validCol,
                                VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
        using T = typename TileDataDst::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
        if constexpr(std::is_same<T, int16_t>::value) {
            TDivs_naive<T, TileDataDst::Cols, TileDataSrc::Cols>(dst, src0, src1, validRow, validCol);
        } else {
            BinaryInstr<DivSOp<T>, TileDataDst, TileDataSrc, T, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>(
                    dstPtr, src0Ptr, src1, validRow, validCol, version);
        }  
    }

    template <typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride, unsigned srcRowStride>
    __tf__ PTO_INTERNAL OP_NAME(TDIVS) OP_TYPE(element_wise)
    void TDivS(typename TileDataDst::TileDType __out__ dst,
                                typename TileDataSrc::DType __in__ src1, 
                                typename TileDataSrc::TileDType __in__ src0, 
                                unsigned validRow,
                                unsigned validCol,
                                VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
        using T = typename TileDataDst::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
        if constexpr(std::is_same<T, int16_t>::value) {
            TSDiv_naive<T, TileDataDst::Cols, TileDataSrc::Cols>(dst, src0, src1, validRow, validCol);
        } else {
            BinaryInstr<DivSOpS<T>, TileDataDst, TileDataSrc, T, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>(
                    dstPtr, src0Ptr, src1, validRow, validCol, version);
        }
    }
    
    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TDIVS_IMPL(TileDataDst &dst, TileDataSrc &src0, typename TileDataSrc::DType scalar)
    {
        static_assert(
                    std::is_same<typename TileDataDst::DType, uint32_t>::value ||
                    std::is_same<typename TileDataDst::DType, int32_t>::value ||
                    std::is_same<typename TileDataDst::DType, int>::value ||
                    std::is_same<typename TileDataDst::DType, uint16_t>::value ||
                    std::is_same<typename TileDataDst::DType, int16_t>::value ||
                    std::is_same<typename TileDataDst::DType, uint8_t>::value ||
                    std::is_same<typename TileDataDst::DType, int8_t>::value ||
                    std::is_same<typename TileDataDst::DType, half>::value ||
                    std::is_same<typename TileDataDst::DType, float16_t>::value ||
                    std::is_same<typename TileDataDst::DType, float>::value ||
                    std::is_same<typename TileDataDst::DType, float32_t>::value,
                      "TDIVS: Invalid data type");

        static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataDst::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataSrc::ValidCol <= TileDataSrc::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataSrc::ValidRow <= TileDataSrc::Rows, "Number of valid rows must not be greater than number of tile rows.");
        static_assert(TileDataDst::ValidCol <= TileDataDst::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataDst::ValidRow <= TileDataDst::Rows, "Number of valid rows must not be greater than number of tile rows.");

        PTO_ASSERT(src0.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
        PTO_ASSERT(src0.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");

        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType);
        constexpr unsigned dstRowStride = TileDataDst::RowStride;
        constexpr unsigned srcRowStride = TileDataSrc::RowStride;
        unsigned validRow = dst.GetValidRow();
        unsigned validCol = dst.GetValidCol();
        TDivS<TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>
            (dst.data(), src0.data(), scalar, validRow, validCol);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TDIVS_IMPL(TileDataDst &dst, typename TileDataSrc::DType scalar, TileDataSrc &src0)
    {
        static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataDst::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataSrc::ValidCol <= TileDataSrc::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataSrc::ValidRow <= TileDataSrc::Rows, "Number of valid rows must not be greater than number of tile rows.");
        static_assert(TileDataDst::ValidCol <= TileDataDst::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataDst::ValidRow <= TileDataDst::Rows, "Number of valid rows must not be greater than number of tile rows.");

        PTO_ASSERT(src0.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
        PTO_ASSERT(src0.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");

        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType);
        constexpr unsigned dstRowStride = TileDataDst::RowStride;
        constexpr unsigned srcRowStride = TileDataSrc::RowStride;
        unsigned validRow = dst.GetValidRow();
        unsigned validCol = dst.GetValidCol();
        TDivS<TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride>
            (dst.data(), scalar, src0.data(), validRow, validCol);
    }
}
#endif
