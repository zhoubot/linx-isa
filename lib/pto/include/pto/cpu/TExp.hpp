/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TEXP_HPP
#define TEXP_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"
#include <cmath>
#include <type_traits>

namespace pto{

    template <typename tile_shape>
    void TExp_Impl(typename tile_shape::TileDType dst,
                            typename tile_shape::TileDType src,
                            unsigned validRow, unsigned validCol
                        ) {
        using ElemT = std::remove_reference_t<decltype(dst[0])>;
        if constexpr (tile_shape::SFractal == SLayout::NoneBox && tile_shape::isRowMajor) {
            cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                const std::size_t base = r * tile_shape::Cols;
                PTO_CPU_VECTORIZE_LOOP
                for (std::size_t c = 0; c < validCol; ++c) {
                    const std::size_t idx = base + c;
                    if constexpr (std::is_same_v<typename tile_shape::TileDType, aclFloat16>) {
                        dst[idx] = static_cast<aclFloat16>(expf(static_cast<float>(src[idx])));
                    } else {
                        dst[idx] = static_cast<ElemT>(std::exp(static_cast<double>(src[idx])));
                    }
                }
            });
        } else {
            cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                for (std::size_t c = 0; c < validCol; ++c) {
                    const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                    if constexpr (std::is_same_v<typename tile_shape::TileDType, aclFloat16>) {
                        dst[idx] = static_cast<aclFloat16>(expf(static_cast<float>(src[idx])));
                    } else {
                        dst[idx] = static_cast<ElemT>(std::exp(static_cast<double>(src[idx])));
                    }
                }
            });
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TEXP_IMPL(tile_shape &dst, tile_shape &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TExp_Impl<tile_shape>(dst.data(), src.data(), row, col);
    }
}

#endif
