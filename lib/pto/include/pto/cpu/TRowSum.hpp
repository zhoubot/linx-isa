/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWSUM_HPP
#define TROWSUM_HPP
#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"

namespace pto {
    template <typename TileDst, typename TileSrc>
    void TRowSum(typename TileDst::TileDType dst, typename TileSrc::TileDType src, uint16_t M, uint16_t N)
    {
        cpu::parallel_for_1d(0, M, static_cast<std::size_t>(M) * N, [&](std::size_t i) {
            TypeSum<TileDst> sum = 0;
            if constexpr (TileSrc::SFractal == SLayout::NoneBox && TileSrc::isRowMajor) {
                const std::size_t base = i * TileSrc::Cols;
                PTO_CPU_VECTORIZE_LOOP
                for (std::size_t j = 0; j < N; ++j) {
                    sum += src[base + j];
                }
            } else {
                for (std::size_t j = 0; j < N; ++j) {
                    sum += src[GetTileElementOffset<TileSrc>(i, j)];
                }
            }
            if constexpr (TileDst::SFractal == SLayout::NoneBox && TileDst::isRowMajor) {
                dst[i * TileDst::Cols] = static_cast<typename TileDst::DType>(sum);
            } else {
                dst[GetTileElementOffset<TileDst>(i, 0)] = static_cast<typename TileDst::DType>(sum);
            }
        });
    }

    template <typename TileDst, typename TileSrc>
    PTO_INTERNAL void CheckRSValid()
    {
        using SrcType = typename TileSrc::DType;
        using DstType = typename TileDst::DType;
        static_assert(
            (std::is_same_v<SrcType, half> && std::is_same_v<DstType, half>) ||  // f162f16
                (std::is_same_v<SrcType, half> && std::is_same_v<DstType, float>) ||  // f162f32
                (std::is_same_v<SrcType, float> && std::is_same_v<DstType, float>)  // f322f32
            , "Not supported data type");
        static_assert(
            (TileSrc::Rows == TileDst::Rows),
            "Inconsistent number of m, n");
    }

    template <typename TileDataOut, typename TileDataIn, typename TileDataTmp>
    PTO_INTERNAL void TROWSUM_IMPL(TileDataOut &dstTile, TileDataIn &srcTile, TileDataTmp &tmp)
    {
        CheckRSValid<TileDataOut, TileDataIn>();

        uint16_t m = srcTile.GetValidRow();
        uint16_t n = srcTile.GetValidCol();

        TRowSum<TileDataOut, TileDataIn>(dstTile.data(), srcTile.data(), m, n);
    }
}
#endif
