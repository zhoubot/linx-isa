/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TINSERT_HPP
#define TINSERT_HPP

#include <cstdint>

#include "pto/cpu/tile_offsets.hpp"

namespace pto {

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    const std::size_t srcRows = static_cast<std::size_t>(src.GetValidRow());
    const std::size_t srcCols = static_cast<std::size_t>(src.GetValidCol());
    if (srcRows == 0 || srcCols == 0) {
        return;
    }

    for (std::size_t r = 0; r < srcRows; ++r) {
        for (std::size_t c = 0; c < srcCols; ++c) {
            const std::size_t dr = static_cast<std::size_t>(indexRow) + r;
            const std::size_t dc = static_cast<std::size_t>(indexCol) + c;
            dst.data()[GetTileElementOffset<DstTileData>(dr, dc)] =
                static_cast<typename DstTileData::DType>(src.data()[GetTileElementOffset<SrcTileData>(r, c)]);
        }
    }
}

} // namespace pto

#endif

