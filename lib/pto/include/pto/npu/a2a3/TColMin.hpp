/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLMIN_HPP
#define TCOLMIN_HPP

#include "TColReduceOps.hpp"

namespace pto {
    template <typename T>
    struct COLMINOp
    {
        PTO_INTERNAL static void ReduceInstr(__ubuf__ T *dst, __ubuf__ T *src0,
                                              __ubuf__ T *src1, uint8_t repeats,
                                              uint8_t dstRepeatStride,
                                              uint8_t src0RepeatStride,
                                              uint8_t src1RepeatStride)
        {
            vmin(dst, src0, src1, repeats, 1, 1, 1, dstRepeatStride,
                 src0RepeatStride, src1RepeatStride);
        }
    };

    template <typename T, typename TileDataDst, typename TileDataSrc, int srcstride>
    __tf__ PTO_INTERNAL void TColMin(typename TileDataDst::TileDType __out__ dst,
                                              typename TileDataSrc::TileDType __in__ src,
                                              int validRow, int validCol)
    {
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

        ColReduceInstr<COLMINOp<T>, T, TileDataDst, TileDataSrc, srcstride>(
            dstPtr, srcPtr, validRow, validCol);
    }

    template <typename TileDataOut, typename TileDataIn>
    PTO_INTERNAL void TCOLMIN_IMPL(TileDataOut &dst, TileDataIn &src) {
        using T = typename TileDataIn::DType;
        int ValidRow = src.GetValidRow();
        int ValidCol = src.GetValidCol();
        TColReduceCheck<T, TileDataOut, TileDataIn>(ValidRow, ValidCol, dst.GetValidCol());
        if (ValidRow == 0 || ValidCol == 0) {
            return;
        }
        constexpr int srcstride = TileDataIn::RowStride;
        TColMin<T, TileDataOut, TileDataIn, srcstride>(dst.data(), src.data(), ValidRow, ValidCol);
    }
}
#endif
