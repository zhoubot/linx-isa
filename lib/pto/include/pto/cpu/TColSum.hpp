
/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TCOLSUM_HPP
#define TCOLSUM_HPP

#include <pto/common/pto_tile.hpp>
#include <cmath>

namespace pto {
    template <typename TileDst, typename TileSrc>
    void TColSum(typename TileDst::TileDType dst, typename TileSrc::TileDType src, uint16_t M, uint16_t N)
    {
        for (uint16_t j = 0; j < N; j++) {
            typename TileDst::DType sum = 0;

            for (uint16_t i = 0; i < M; i++) {
               sum += src[GetTileElementOffset<TileSrc>(i,j)];
            }
            dst[GetTileElementOffset<TileDst>(0,j)] = sum;
        }
    }

    template <typename TileDst, typename TileSrc>
    PTO_INTERNAL void CheckCSValid()
    {
        using SrcNonDuplicateType = TileSrc::DType;
        using DstNonDuplicateType = TileDst::DType;
        static_assert(
            (std::is_same_v<SrcNonDuplicateType, half> && std::is_same_v<DstNonDuplicateType, half>) ||  // f162f16
                (std::is_same_v<SrcNonDuplicateType, half> && std::is_same_v<DstNonDuplicateType, float>) ||  // f162f32
                (std::is_same_v<SrcNonDuplicateType, float> && std::is_same_v<DstNonDuplicateType, float>)  // f322f32
            , "Not supported data type");
        static_assert(
            (TileSrc::Cols == TileDst::Cols),
            "Assert: Inconsistent number of cols");
        static_assert(
            (TileDst::Rows == 1),
            "Assert: Inconsistent number of dst tile rows");
    }

    template <typename TileDst, typename TileSrc>
    PTO_INTERNAL void TCOLSUM_IMPL(TileDst &dstTile, TileSrc &srcTile)
    {
        CheckCSValid<TileDst, TileSrc>();

        uint16_t m = srcTile.GetValidRow();
        uint16_t n = srcTile.GetValidCol();

        TColSum<TileDst, TileSrc>(dstTile.data(), srcTile.data(), m, n);
    }
}
#endif
