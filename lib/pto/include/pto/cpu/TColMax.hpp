
/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TCOLMAX_HPP
#define TCOLMAX_HPP

#include <pto/common/pto_tile.hpp>
#include <cmath>

namespace pto {
    template <typename TileDst, typename TileSrc>
    void TColMax(typename TileDst::TileDType dst, typename TileSrc::TileDType src, uint16_t M, uint16_t N)
    {
        for (uint16_t j = 0; j < N; j++) {
            typename TileDst::DType max = src[GetTileElementOffset<TileSrc>(0,j)];

            for (uint16_t i = 1; i < M; i++) {
                size_t idx = GetTileElementOffset<TileSrc>(i, j);
                if (src[idx] > max) {
                    max = src[idx];
                }
            }
            dst[GetTileElementOffset<TileDst>(0,j)] = max;
        }
    }

    template <typename TileDst, typename TileSrc>
    PTO_INTERNAL void CheckCMValid()
    {
        using SrcType = TileSrc::DType;
        using DstType = TileDst::DType;
        static_assert(
            (std::is_same_v<SrcType, half> && std::is_same_v<DstType, half>) ||  // f162f16
                (std::is_same_v<SrcType, half> && std::is_same_v<DstType, float>) ||  // f162f32
                (std::is_same_v<SrcType, float> && std::is_same_v<DstType, float>)  // f322f32
            , "Not supported data type");
        static_assert(
            (TileSrc::Cols == TileDst::Cols),
            "Inconsistent number of cols");
        static_assert(
            (TileDst::Rows == 1),
            "Inconsistent number of dst tile rows");
    }

    template <typename TileDst, typename TileSrc>
    PTO_INTERNAL void TCOLMAX_IMPL(TileDst &dstTile, TileSrc &srcTile)
    {
        CheckCMValid<TileDst, TileSrc>();

        unsigned m = srcTile.GetValidRow();
        unsigned n = srcTile.GetValidCol();

        TColMax<TileDst, TileSrc>(dstTile.data(), srcTile.data(), m, n);
    }
}
#endif
