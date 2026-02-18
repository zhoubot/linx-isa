/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWMAX_HPP
#define TROWMAX_HPP

#include <pto/common/pto_tile.hpp>
#include <cmath>
#include "pto/cpu/parallel.hpp"

namespace pto{

    template <typename tile_shape_out, typename tile_shape_in>
    void TRowmax_Impl(typename tile_shape_out::TileDType dst,
                            typename tile_shape_in::TileDType src,
                            unsigned validRow, unsigned validCol
                        ) {
        cpu::parallel_for_1d(0, validRow, static_cast<std::size_t>(validRow) * validCol, [&](std::size_t i) {
            typename tile_shape_out::DType max_val;
            if constexpr (tile_shape_in::SFractal == SLayout::NoneBox && tile_shape_in::isRowMajor) {
                const std::size_t base = i * tile_shape_in::Cols;
                max_val = src[base];
                PTO_CPU_VECTORIZE_LOOP
                for (std::size_t j = 1; j < validCol; ++j) {
                    max_val = std::max(max_val, static_cast<typename tile_shape_out::DType>(src[base + j]));
                }
            } else {
                size_t idx = GetTileElementOffset<tile_shape_in>(i, 0);
                max_val = src[idx];
                for (std::size_t j = 1; j < validCol; ++j) {
                    idx = GetTileElementOffset<tile_shape_in>(i, j);
                    if (src[idx] > max_val) {
                        max_val = src[idx];
                    }
                }
            }

            if constexpr (tile_shape_out::SFractal == SLayout::NoneBox && tile_shape_out::isRowMajor) {
                dst[i * tile_shape_out::Cols] = max_val;
            } else {
                dst[GetTileElementOffset<tile_shape_out>(i, 0)] = max_val;
            }
        });
    }

  template <typename TileDataOut, typename TileDataIn, typename TileDataTmp>
  PTO_INTERNAL void TROWMAX_IMPL(TileDataOut &dst, TileDataIn &src, TileDataTmp &tmp) {
        (void)tmp;
        unsigned row = src.GetValidRow();
        unsigned col = src.GetValidCol();
        TRowmax_Impl<TileDataOut, TileDataIn>(dst.data(), src.data(), row, col);
    }
}

#endif
