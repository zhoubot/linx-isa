/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLEXPANDSUB_HPP
#define TCOLEXPANDSUB_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"
#include "TColExpandBinOp.hpp"

namespace pto {
    
    template <typename T> struct ColExpandSubOp {
        PTO_INTERNAL static void ColExpandBinaryInstr(RegTensor<T> &reg_dst, RegTensor<T> &reg_src0, RegTensor<T> &reg_src1, MaskReg &preg)
        {
            vsub(reg_dst, reg_src0, reg_src1, preg, MODE_ZEROING);
        }
    };

    template <typename TileData, typename TileDataSrc, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned rowStride>
    __tf__ AICORE void TColExpandSub(typename TileData::TileDType __out__ dst, 
                                typename TileData::TileDType __in__ src0,
                                typename TileDataSrc::TileDType __in__ src1,
                                unsigned validRow,
                                unsigned validCol) {
        using T = typename TileData::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
        __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);

        ColExpandBinaryInstr<ColExpandSubOp<T>, TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(
                                dstPtr, src0Ptr, src1Ptr, validRow, validCol);
    }

    template <typename TileData, typename TileDataSrc>
    PTO_INTERNAL void TCOLEXPANDSUB_IMPL(TileData &dst, TileData &src0, TileDataSrc &src1) {
        static_assert(std::is_same<typename TileData::DType, float>::value ||
                      std::is_same<typename TileData::DType, half>::value,
                      "Fix: TCOLEXPANDSUB Invalid data type.");
        static_assert(TileData::isRowMajor, "Fix: TCOLEXPANDSUB not supported Layout type");
        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileData::DType); 
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileData::DType); 
        constexpr unsigned rowStride = TileData::RowStride;
        unsigned validRow = dst.GetValidRow();
        unsigned validCol = dst.GetValidCol();

        TColExpandSub<TileData, TileDataSrc, elementsPerRepeat, blockSizeElem, rowStride>(dst.data(), src0.data(), src1.data(), validRow, validCol);
    }
}
#endif