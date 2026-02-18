/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPARTOP_HPP
#define TPARTOP_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto{
    template<typename InstrOp, typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
    PTO_INTERNAL void TPartInstr(typename TileDataDst::TileDType dst, typename TileDataSrc0::TileDType src0,
                                 typename TileDataSrc1::TileDType src1, int DstValidRow, int DstValidCol,
                                 int Src0ValidRow, int Src0ValidCol, int Src1ValidRow, int Src1ValidCol)
    {
        for (int i = 0; i < DstValidRow; i++) {
            for (int j = 0; j < DstValidCol; j++) {
                const size_t DstOffset = GetTileElementOffset<TileDataDst>(i, j);
                bool InSrc0 = i < Src0ValidRow && j < Src0ValidCol;
                bool InSrc1 = i < Src1ValidRow && j < Src1ValidCol;
                const size_t Src0Offset = InSrc0 ? GetTileElementOffset<TileDataSrc0>(i, j) : -1;
                const size_t Src1Offset = InSrc1 ? GetTileElementOffset<TileDataSrc1>(i, j) : -1;
                if (InSrc0 && InSrc1) {
                    InstrOp::PartInstr(dst, src0, src1, DstOffset, Src0Offset, Src1Offset);
                } else if (InSrc0 && !InSrc1) {
                    dst[DstOffset] = src0[Src0Offset];
                } else if (!InSrc0 && InSrc1) {
                    dst[DstOffset] = src1[Src1Offset];
                } else {
                    dst[DstOffset] = 0;
                }
            }
        }
    }

    template<typename T, typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
    PTO_INTERNAL void TPartCheck(int DstValidRow, int DstValidCol)
    {
        static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, half> ||
                      std::is_same_v<T, float>, "TPARTMAX: Invalid data type.");
        static_assert(std::is_same_v<typename TileDataDst::DType, T> && std::is_same_v<typename TileDataSrc1::DType, T>,
                      "The Src0 data type must be consistent with the dst and src1 data type");
        if (DstValidRow == 0 || DstValidCol == 0) {
            return;
        }
    }
}
#endif