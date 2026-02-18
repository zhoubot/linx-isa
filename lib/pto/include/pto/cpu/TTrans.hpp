/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TTRANS_HPP
#define TTRANS_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
namespace pto
{
    template <typename DstTileData, typename SrcTileData>
    void TTrans_Impl(typename DstTileData::TileDType dst,
                            typename SrcTileData::TileDType src,
                            unsigned validRow, unsigned validCol
                        ) {
        for(size_t c=0; c< validCol; c++) {
            size_t subTileSrcC = c / SrcTileData::InnerCols;
            size_t innerSrcC = c % SrcTileData::InnerCols;
            size_t subTileDstC = c / DstTileData::InnerCols;
            size_t innerDstC = c % DstTileData::InnerCols;

            for(size_t r=0; r<validRow; r++) {
                size_t srcTileIdx, dstTileIdx;
                if constexpr (SrcTileData::SFractal == SLayout::NoneBox)
                    srcTileIdx = GetTileElementOffsetPlain<SrcTileData>(r, c);
                else {
                    size_t subTileR = r / SrcTileData::InnerRows;
                    size_t innerR = r % SrcTileData::InnerRows;
                    srcTileIdx = GetElementOffsetSubfractals<SrcTileData>(subTileSrcC,innerSrcC,subTileR,innerR);
                }

                if constexpr (DstTileData::SFractal == SLayout::NoneBox)
                    dstTileIdx = GetTileElementOffsetPlain<DstTileData>(c, r);
                else {
                    size_t subTileR = r / DstTileData::InnerRows;
                    size_t innerR = r % DstTileData::InnerRows;
                    dstTileIdx = GetElementOffsetSubfractals<DstTileData>(subTileR,innerR, subTileDstC,innerDstC);
                }
                dst[dstTileIdx] = src[srcTileIdx];
            }
        }
    }

    template <typename DstTileData, typename SrcTileData, typename TmpTileData>
    PTO_INTERNAL void TTRANS_IMPL(DstTileData &dst, SrcTileData &src, TmpTileData &tmp) {
        static_assert (SrcTileData::ValidRow == DstTileData::ValidCol && SrcTileData::ValidCol == DstTileData::ValidRow);
        unsigned validRow = src.GetValidRow();
        unsigned validCol = src.GetValidCol();
        TTrans_Impl<DstTileData, SrcTileData>(dst.data(), src.data(), validRow, validCol);
    }
} 


#endif  // TTRANS_HPP
