/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TEXTRACT_HPP
#define TEXTRACT_HPP

#include <cassert>

namespace pto
{
    template <typename DstTileData, typename SrcTileData>
    PTO_INTERNAL void TEXTRACT_IMPL(DstTileData &dst, SrcTileData &src, uint32_t idxRow = 0, uint32_t idxCol = 0) {
        assert(src.GetValidRow() - idxRow == dst.GetValidRow() && src.GetValidCol() - idxCol == dst.GetValidCol());
        for(size_t c = idxCol; c < src.GetValidCol(); c++) {
            const size_t subTileSrcC = c / SrcTileData::InnerCols;
            const size_t innerSrcC = c % SrcTileData::InnerCols;
            const size_t cDst = c - idxCol;
            const size_t subTileDstC = cDst / DstTileData::InnerCols;
            const size_t innerDstC = cDst % DstTileData::InnerCols;

            for(size_t r = idxRow; r < src.GetValidRow(); r++) {
                size_t srcTileIdx;
                size_t dstTileIdx;
                if constexpr (SrcTileData::SFractal == SLayout::NoneBox) {
                    srcTileIdx = GetTileElementOffsetPlain<SrcTileData>(r,c);
                } else {
                    const size_t subTileR = r / SrcTileData::InnerRows;
                    const size_t innerR = r % SrcTileData::InnerRows;
                    srcTileIdx = GetTileElementOffsetSubfractals<SrcTileData>(subTileR,innerR,subTileSrcC,innerSrcC);
                }
                const size_t rDst = r - idxRow;

                if constexpr (DstTileData::SFractal == SLayout::NoneBox) {
                    dstTileIdx = GetTileElementOffsetPlain<DstTileData>(rDst,cDst);
                } else {
                    const size_t subTileR = rDst / DstTileData::InnerRows;
                    const size_t innerR = rDst % DstTileData::InnerRows;
                    dstTileIdx = GetTileElementOffsetSubfractals<DstTileData>(subTileR,innerR,subTileDstC,innerDstC);
                }
                dst.data()[dstTileIdx] = src.data()[srcTileIdx];
            }
        }
    }
}
#endif  // TEXTRACT_HPP
