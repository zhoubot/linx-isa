/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCVT_HPP
#define TCVT_HPP

#include <pto/common/constants.hpp>
#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include "pto/common/debug.h"
#include <cmath>
#include <type_traits>

namespace pto {
constexpr double CAST_ODD_THRESHHOLD = 0.5;

template <typename T>
constexpr bool is_float_like_v =
    std::is_floating_point_v<T> || std::is_same_v<T, half> ||
    std::is_same_v<T, aclFloat16>;

inline double applyRoundingToIntegral(double v, RoundMode mode)
{
    switch (mode) {
        case RoundMode::CAST_RINT:
            return std::rint(v);

        case RoundMode::CAST_ROUND:
            return std::round(v);

        case RoundMode::CAST_FLOOR:
            return std::floor(v);

        case RoundMode::CAST_CEIL:
            return std::ceil(v);

        case RoundMode::CAST_TRUNC:
            return std::trunc(v);

        case RoundMode::CAST_ODD: {
            const double f = std::floor(v);
            const double frac = v - f;

            if (frac > CAST_ODD_THRESHHOLD) return f + 1;
            if (frac < CAST_ODD_THRESHHOLD) return f;

            // tie (.5) → round to odd
            const auto i = static_cast<long long>(f);
            return (i & 1) ? f : f + 1;
        }

        default:
            return v;
    }
}

template <typename TileDataD, typename TileDataS>
PTO_INTERNAL void TCvt_Impl(typename TileDataD::TileDType dst,
                            typename TileDataS::TileDType src, unsigned validRow, unsigned validCol, RoundMode mode
                        ) {
        for (int i = 0; i < validRow; ++i) {
            for (int j = 0; j < validCol; ++j) {
                size_t dstIdx = GetTileElementOffset<TileDataD>(i,j);
                size_t srcIdx = GetTileElementOffset<TileDataS>(i,j);  
                using D = typename TileDataD::DType;
                using S = typename TileDataS::DType;

                if constexpr (is_float_like_v<S> && std::is_integral_v<D>) {
                    const double dv = static_cast<double>(src[srcIdx]);
                    dst[dstIdx] = static_cast<D>(applyRoundingToIntegral(dv, mode));
                } else {
                    dst[dstIdx] = static_cast<D>(src[srcIdx]);
                }
            }
        }
    }

template <typename TileDataD, typename TileDataS>
PTO_INTERNAL void TCVT_IMPL(TileDataD &dst, TileDataS &src, RoundMode mode)
{
    uint16_t rows = src.GetValidRow();
    uint16_t cols = src.GetValidCol();
    TCvt_Impl<TileDataD, TileDataS>(dst.data(), src.data(), rows, cols, mode);
}

}  // namespace pto
#endif
