/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TEXPANDS_HPP
#define TEXPANDS_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"
#include <cmath>

namespace pto{

    template <typename tile_shape>
    void TExpands_Impl(typename tile_shape::TileDType dst,
                            typename tile_shape::DType src,
                            unsigned validRow, unsigned validCol
                        ) {
        if constexpr (tile_shape::SFractal == SLayout::NoneBox) {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    const std::size_t base = r * tile_shape::Cols;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t c = 0; c < validCol; ++c) {
                        dst[base + c] = src;
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    const std::size_t base = c * tile_shape::Rows;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t r = 0; r < validRow; ++r) {
                        dst[base + r] = src;
                    }
                });
            }
        } else {
            cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                for (std::size_t c = 0; c < validCol; ++c) {
                    const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                    dst[idx] = src;
                }
            });
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TEXPANDS_IMPL(tile_shape &dst, typename tile_shape::DType &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TExpands_Impl<tile_shape>(dst.data(), src, row, col);
    }
}

#endif
