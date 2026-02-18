/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSELS_HPP
#define TSELS_HPP

#include "pto/common/pto_tile.hpp"
#include "tile_offsets.hpp"

namespace pto{
    template<typename TileData>
    void TSelS_Impl(typename TileData::TileDType dst,
                            typename TileData::DType scalar,
                            typename TileData::TileDType src0,
                            typename TileData::TileDType src1,
                            unsigned validRow, unsigned validCol
                        ) {
        for(size_t c=0; c<validCol; c++) {
            for(size_t r=0; r<validRow; r++) {
                size_t idx = GetTileElementOffset<TileData>(r,c);
                // if 1: take src1, else: take src2
                dst[idx] = scalar == 1 ? src0[idx] : src1[idx];
            }
        }
    }

    template <typename TileData>
    __aicore__ PTO_INLINE void TSELS_IMPL(TileData &dst, TileData &src0, TileData &src1, uint8_t selectMode) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TSelS_Impl<TileData>(dst.data(), selectMode, src0.data(), src1.data(), row, col);
    }
}
#endif