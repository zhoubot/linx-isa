/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TILE_OFFSETS_HPP
#define TILE_OFFSETS_HPP

#include <unistd.h>
namespace pto {
    template <typename TileData>
    using TypeSum = std::conditional_t<std::is_same_v<typename TileData::DType, half>, float, typename TileData::DType>;

    template <typename TileData>
    size_t GetTileElementOffsetSubfractals( size_t subTileR, size_t innerR, size_t subTileC, size_t innerC) {
        if constexpr(!TileData::isRowMajor & (TileData::SFractal == SLayout::RowMajor)) {
            // Nz
            return subTileC*TileData::Rows*TileData::InnerCols +
                subTileR*TileData::InnerNumel + innerR*TileData::InnerCols + innerC;
        } else if constexpr(TileData::isRowMajor & (TileData::SFractal == SLayout::ColMajor)) {
            // Zn
            return subTileR*TileData::Cols*TileData::InnerRows +
                subTileC*TileData::InnerNumel + innerC*TileData::InnerRows + innerR;
        } else if constexpr(TileData::isRowMajor & (TileData::SFractal == SLayout::RowMajor)) {
            // Zz
            return subTileR*TileData::Cols*TileData::InnerRows +
                subTileC*TileData::InnerNumel + innerR*TileData::InnerCols + innerC;
        } else {
            static_assert(false, "Invalid layout");
        }
    }

    template <typename TileData>
    size_t GetTileElementOffsetPlain(size_t r, size_t c) {
        if constexpr(TileData::isRowMajor) {
            return r*TileData::Cols+c;
        } else {
            return c*TileData::Rows+r;            
        }
    }

    template <typename TileData>
    size_t GetTileElementOffset(size_t r, size_t c) {
        if constexpr (TileData::SFractal == SLayout::NoneBox)
            return GetTileElementOffsetPlain<TileData>(r,c);
        else {
            size_t subTileR = r / TileData::InnerRows;
            size_t innerR = r % TileData::InnerRows;
            return GetTileElementOffsetSubfractals<TileData>(r/TileData::InnerRows, r%TileData::InnerRows,
                                                            c/TileData::InnerCols, c%TileData::InnerCols);
        }
    }

}
#endif
