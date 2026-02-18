/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSCATTER_HPP
#define TSCATTER_HPP

#include "pto/cpu/tile_offsets.hpp"
#include <pto/common/pto_tile.hpp>
#include <type_traits>

namespace pto {

template <typename TileData, typename TileInd>
PTO_INTERNAL void TSCATTER_IMPL(TileData &dst, TileData &src, TileInd &indexes)
{
    using IndexT = typename TileInd::DType;
    static_assert(std::is_integral_v<IndexT>, "TSCATTER: indexes must be an integral type");

    const unsigned validRow = src.GetValidRow();
    const unsigned validCol = src.GetValidCol();
    if (validRow == 0 || validCol == 0) {
        return;
    }

    for (unsigned i = 0; i < validRow; ++i) {
        for (unsigned j = 0; j < validCol; ++j) {
            const size_t srcOff = GetTileElementOffset<TileData>(i, j);
            const size_t idxOff = GetTileElementOffset<TileInd>(i, j);
            const auto dstRow = static_cast<unsigned>(indexes.data()[idxOff]);
            const size_t dstOff = GetTileElementOffset<TileData>(dstRow, j);
            dst.data()[dstOff] = src.data()[srcOff];
        }
    }
}

} // namespace pto

#endif

