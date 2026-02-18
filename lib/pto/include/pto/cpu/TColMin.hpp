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

#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"
#include <pto/common/pto_tile.hpp>

namespace pto {

template <typename TileOut, typename TileIn>
PTO_INTERNAL void TColmin_Impl(typename TileOut::TileDType dst, typename TileIn::TileDType src, unsigned validRow,
                              unsigned validCol)
{
    if (validRow == 0 || validCol == 0) {
        return;
    }
    cpu::parallel_for_1d(0, validCol, static_cast<std::size_t>(validRow) * validCol, [&](std::size_t j) {
        size_t idx = GetTileElementOffset<TileIn>(0, j);
        typename TileOut::DType minVal = src[idx];
        for (std::size_t i = 1; i < validRow; ++i) {
            idx = GetTileElementOffset<TileIn>(i, j);
            if (src[idx] < minVal) {
                minVal = src[idx];
            }
        }
        dst[GetTileElementOffset<TileOut>(0, j)] = minVal;
    });
}

template <typename TileDataOut, typename TileDataIn>
PTO_INTERNAL void TCOLMIN_IMPL(TileDataOut &dst, TileDataIn &src)
{
    TColmin_Impl<TileDataOut, TileDataIn>(dst.data(), src.data(), src.GetValidRow(), src.GetValidCol());
}

} // namespace pto

#endif
