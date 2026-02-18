/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSEL_HPP
#define TSEL_HPP

#include <pto/common/constants.hpp>
#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include "pto/common/debug.h"
#include <iostream>

namespace pto
{
    template <typename tile_shape, typename mask_tile_shape>
    PTO_INTERNAL void TSel_Impl(typename tile_shape::TileDType dst,
                            typename tile_shape::TileDType src0, typename tile_shape::TileDType src1, typename mask_tile_shape::TileDType selMask,
                            unsigned validRow, unsigned validCol, unsigned validMaskCol
                        ) {
        constexpr uint8_t maskSize = 8;
        size_t maskRowIdx = 0;
        size_t maskColIdx = 0;
        uint8_t bitIdx = 0;
        for (int i = 0; i < validRow; ++i) {
            for (int j = 0; j < validCol; ++j) {
                size_t dstIdx = GetTileElementOffset<tile_shape>(i,j);
                size_t maskIdx = GetTileElementOffset<mask_tile_shape>(maskRowIdx,maskColIdx);
                const uint8_t bit = (selMask[maskIdx] >> bitIdx) & 1;
                bitIdx++;
                if(bitIdx == 8){
                    bitIdx = 0;
                    maskColIdx++;
                    if(maskColIdx == validMaskCol){
                        maskRowIdx++;
                        maskColIdx = 0;
                    }
                }
                dst[dstIdx] = (bit) ? src0[dstIdx] : src1[dstIdx];    
            }
        }
    }
    template <typename tile_shape, typename mask_tile_shape>
    PTO_INTERNAL void TSEL_IMPL(tile_shape &dst, mask_tile_shape &selMask, tile_shape &src0, tile_shape &src1)
    {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        unsigned rowMask = selMask.GetValidRow();
        unsigned colMask = selMask.GetValidCol();
        TSel_Impl<tile_shape, mask_tile_shape>(dst.data(), src0.data(), src1.data(), selMask.data(), row, col, colMask);
    }
}
#endif