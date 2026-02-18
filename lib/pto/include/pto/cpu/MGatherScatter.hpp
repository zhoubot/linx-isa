/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef MGATHER_SCATTER_HPP
#define MGATHER_SCATTER_HPP

#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"
#include <pto/common/pto_tile.hpp>
#include <type_traits>

namespace pto {

template <typename TileDst, typename GlobalData, typename TileInd>
PTO_INTERNAL void MGATHER_IMPL(TileDst &dst, GlobalData &src, TileInd &indexes)
{
    using IndexT = typename TileInd::DType;
    static_assert(std::is_integral_v<IndexT>, "MGATHER: indexes must be an integral type");
    static_assert(sizeof(typename TileDst::DType) == sizeof(typename GlobalData::DType),
                  "MGATHER: element sizes must match");

    const unsigned validRow = dst.GetValidRow();
    const unsigned validCol = dst.GetValidCol();
    if (validRow == 0 || validCol == 0) {
        return;
    }

    auto *base = src.data();
    cpu::parallel_for_rows(validRow, validCol, [&](std::size_t i) {
        for (std::size_t j = 0; j < validCol; ++j) {
            const size_t dstOff = GetTileElementOffset<TileDst>(i, j);
            const size_t idxOff = GetTileElementOffset<TileInd>(i, j);
            const auto idx = static_cast<size_t>(indexes.data()[idxOff]);
            dst.data()[dstOff] = base[idx];
        }
    });
}

template <typename GlobalData, typename TileSrc, typename TileInd>
PTO_INTERNAL void MSCATTER_IMPL(GlobalData &dst, TileSrc &src, TileInd &indexes)
{
    using IndexT = typename TileInd::DType;
    static_assert(std::is_integral_v<IndexT>, "MSCATTER: indexes must be an integral type");
    static_assert(sizeof(typename TileSrc::DType) == sizeof(typename GlobalData::DType),
                  "MSCATTER: element sizes must match");

    const unsigned validRow = src.GetValidRow();
    const unsigned validCol = src.GetValidCol();
    if (validRow == 0 || validCol == 0) {
        return;
    }

    auto *base = dst.data();
    for (unsigned i = 0; i < validRow; ++i) {
        for (unsigned j = 0; j < validCol; ++j) {
            const size_t srcOff = GetTileElementOffset<TileSrc>(i, j);
            const size_t idxOff = GetTileElementOffset<TileInd>(i, j);
            const auto idx = static_cast<size_t>(indexes.data()[idxOff]);
            base[idx] = src.data()[srcOff];
        }
    }
}

} // namespace pto

#endif
