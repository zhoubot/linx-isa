/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCMP_HPP
#define TCMP_HPP

#include <cassert>
#include <cmath>
#include <cstdint>
#include <type_traits>

#include "pto/cpu/tile_offsets.hpp"
#include "pto/common/constants.hpp"

namespace pto {

template <typename T>
AICORE uint8_t CmpVV(T a, T b, CmpMode cmpMode) {
    uint8_t res = 0;
    const double diff = static_cast<double>(a) - static_cast<double>(b);
    switch (static_cast<CmpMode>(cmpMode)) {
        case CmpMode::EQ:
            res = (std::fabs(diff) < 1e-9);
            break;
        case CmpMode::NE:
            res = (std::fabs(diff) > 1e-9);
            break;
        case CmpMode::LT:
            res = (a < b);
            break;
        case CmpMode::GT:
            res = (a > b);
            break;
        case CmpMode::GE:
            res = (a >= b);
            break;
        case CmpMode::LE:
            res = (a <= b);
            break;
        default:
            res = (std::fabs(diff) < 1e-9);
            break;
    }
    return res;
}

// Packed vector-vector compare:
// - dst is a u8 tile holding one bit per src element, packed as bytes along the column dimension.
// - dst valid shape is (srcValidRow, ceil(srcValidCol / 8)).
template <typename TileDataDst, typename TileDataSrc,
          typename = std::enable_if_t<std::is_same_v<typename TileDataDst::DType, uint8_t>>>
PTO_INTERNAL void TCMP_IMPL(TileDataDst &dst, TileDataSrc &src0, TileDataSrc &src1, CmpMode cmpMode) {
    static_assert(std::is_same_v<typename TileDataDst::DType, uint8_t>, "TCMP: dst tile must be u8.");
    static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc::Loc == TileType::Vec, "TCMP: only supports Vec tiles.");

    const std::size_t srcRows = static_cast<std::size_t>(src0.GetValidRow());
    const std::size_t srcCols = static_cast<std::size_t>(src0.GetValidCol());
    assert(src1.GetValidRow() == static_cast<int>(srcRows) && src1.GetValidCol() == static_cast<int>(srcCols));

    const std::size_t dstRows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t dstCols = static_cast<std::size_t>(dst.GetValidCol());
    assert(dstRows == srcRows);
    assert(dstCols == (srcCols + 7) / 8);

    for (std::size_t r = 0; r < srcRows; ++r) {
        for (std::size_t byteIdx = 0; byteIdx < dstCols; ++byteIdx) {
            uint8_t packed = 0;
            for (std::size_t bit = 0; bit < 8; ++bit) {
                const std::size_t c = byteIdx * 8 + bit;
                if (c >= srcCols) {
                    break;
                }
                const auto a = src0.data()[GetTileElementOffset<TileDataSrc>(r, c)];
                const auto b = src1.data()[GetTileElementOffset<TileDataSrc>(r, c)];
                packed |= static_cast<uint8_t>(CmpVV(a, b, cmpMode) << bit);
            }
            dst.data()[GetTileElementOffset<TileDataDst>(r, byteIdx)] = packed;
        }
    }
}

} // namespace pto

#endif
