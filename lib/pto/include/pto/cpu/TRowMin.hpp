/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWMIN_HPP
#define TROWMIN_HPP

#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"
#include <pto/common/pto_tile.hpp>

namespace pto {

template <typename TileOut, typename TileIn>
PTO_INTERNAL void TRowmin_Impl(typename TileOut::TileDType dst, typename TileIn::TileDType src, unsigned validRow,
                              unsigned validCol)
{
    if (validRow == 0 || validCol == 0) {
        return;
    }
    cpu::parallel_for_1d(0, validRow, static_cast<std::size_t>(validRow) * validCol, [&](std::size_t i) {
        typename TileOut::DType minVal;
        if constexpr (TileIn::SFractal == SLayout::NoneBox && TileIn::isRowMajor) {
            const std::size_t base = i * TileIn::Cols;
            minVal = src[base];
            PTO_CPU_VECTORIZE_LOOP
            for (std::size_t j = 1; j < validCol; ++j) {
                minVal = std::min(minVal, static_cast<typename TileOut::DType>(src[base + j]));
            }
        } else {
            size_t idx = GetTileElementOffset<TileIn>(i, 0);
            minVal = src[idx];
            for (std::size_t j = 1; j < validCol; ++j) {
                idx = GetTileElementOffset<TileIn>(i, j);
                if (src[idx] < minVal) {
                    minVal = src[idx];
                }
            }
        }

        if constexpr (TileOut::SFractal == SLayout::NoneBox && TileOut::isRowMajor) {
            dst[i * TileOut::Cols] = minVal;
        } else {
            dst[GetTileElementOffset<TileOut>(i, 0)] = minVal;
        }
    });
}

template <typename TileDataOut, typename TileDataIn, typename TileDataTmp>
PTO_INTERNAL void TROWMIN_IMPL(TileDataOut &dst, TileDataIn &src, TileDataTmp &tmp)
{
    (void)tmp;
    TRowmin_Impl<TileDataOut, TileDataIn>(dst.data(), src.data(), src.GetValidRow(), src.GetValidCol());
}

} // namespace pto

#endif
