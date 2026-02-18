/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWEXPANDMUL_HPP
#define TROWEXPANDMUL_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TRowExpandBinOp.hpp"

namespace pto {
    
    template <typename T> struct RowExpandMulOp {
        PTO_INTERNAL static void RowExpandBinaryInstr(RegTensor<T> &reg_dst, RegTensor<T> &reg_src0, RegTensor<T> &reg_src1, MaskReg &preg)
        {
            vmul(reg_dst, reg_src0, reg_src1, preg, MODE_ZEROING);
        }
    };

    template <typename TileDataDst, typename TileDataSrc1, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
    __tf__ AICORE OP_NAME(TROWEXPANDMUL) OP_TYPE(broadcast) void TRowExpandMul(typename TileDataDst::TileDType __out__ dst, 
                                typename TileDataDst::TileDType __in__ src0,
                                typename TileDataSrc1::TileDType __in__ src1,
                                unsigned validRow,
                                unsigned validCol, VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
        using T = typename TileDataDst::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
        __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);

        RowExpandBinaryInstr<RowExpandMulOp<T>, TileDataDst, TileDataSrc1, elementsPerRepeat, blockSizeElem, rowStride>(
                                dstPtr, src0Ptr, src1Ptr, validRow, validCol);
    }

    template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
    PTO_INTERNAL void TROWEXPANDMUL_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1) {
        using T = typename TileDataDst::DType;
        static_assert(std::is_same_v<typename TileDataDst::DType, typename TileDataSrc0::DType> &&
            std::is_same_v<typename TileDataDst::DType, typename TileDataSrc1::DType>,
            "Fix: TROWEXPANDMUL src and dst data type is different!");
        static_assert(
            std::is_same_v<typename TileDataDst::DType, half> || std::is_same_v<typename TileDataDst::DType, float>,
            "Fix: TROWEXPANDMUL Invalid data type.");
        constexpr bool src0eqdst = std::is_same_v<TileDataDst, TileDataSrc0>;
        constexpr bool src1eqdst = std::is_same_v<TileDataDst, TileDataSrc1>;
        static_assert(TileDataDst::isRowMajor && (src0eqdst || src1eqdst), "Fix: TROWEXPANDMUL Invalid tile shape.");
        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType); 
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType); 
        constexpr unsigned rowStride = TileDataDst::RowStride;
        unsigned validRow = dst.GetValidRow();
        unsigned validCol = dst.GetValidCol();

        if constexpr (src0eqdst) {
            unsigned src1ValidCol = src1.GetValidCol();
            PTO_ASSERT(((TileDataSrc1::isRowMajor && src1ValidCol == 32 / sizeof(T)) ||
                        (!TileDataSrc1::isRowMajor && src1ValidCol == 1)) &&
                        src1.GetValidRow() == validRow, "TROWEXPANDMUL: invalid src1 shape.");
            TRowExpandMul<TileDataDst, TileDataSrc1, elementsPerRepeat, blockSizeElem, rowStride>(dst.data(), src0.data(), src1.data(), validRow, validCol);
        } else  {
            unsigned src0ValidCol = src0.GetValidCol();
            PTO_ASSERT(((TileDataSrc0::isRowMajor && src0ValidCol == 32 / sizeof(T)) ||
                        (!TileDataSrc0::isRowMajor && src0ValidCol == 1)) &&
                        src0.GetValidRow() == validRow, "TROWEXPANDMUL: invalid src0 shape.");
            TRowExpandMul<TileDataDst, TileDataSrc0, elementsPerRepeat, blockSizeElem, rowStride>(dst.data(), src1.data(), src0.data(), validRow, validCol);
        }
    }
}
#endif