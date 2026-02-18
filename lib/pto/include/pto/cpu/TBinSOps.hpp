/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TBINS_HPP
#define TBINS_HPP

#include <algorithm>
#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto {
    template <typename TileData, typename Function>
    void TBinSOp(typename TileData::TileDType dst, 
                typename TileData::TileDType src, 
                typename TileData::DType scalar,
                unsigned validRow, 
                unsigned validCol,
                Function&& function) {
        for(size_t c=0; c<validCol; c++) {
            for(size_t r=0; r<validRow; r++) {
                size_t idx = GetTileElementOffset<TileData>(r,c);
                dst[idx] = function(src[idx], scalar);
            }
        }
    }

    template <typename TileData>
    PTO_INTERNAL void TADDS_IMPL(TileData &dst, TileData &src, typename TileData::DType scalar) {
        auto lambda = [](typename TileData::DType x, typename TileData::DType y) {
            return x+y;
        };
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TBinSOp<TileData>(dst.data(), src.data(), scalar, row, col, lambda);
    }
    template <typename TileData>
    PTO_INTERNAL void TMULS_IMPL(TileData &dst, TileData &src, typename TileData::DType scalar) {
        auto lambda = [](typename TileData::DType x, typename TileData::DType y) {
            return x*y;
        };
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TBinSOp<TileData>(dst.data(), src.data(), scalar, row, col, lambda);
    }
    template <typename TileData>
    PTO_INTERNAL void TDIVS_IMPL(TileData &dst, TileData &src, typename TileData::DType scalar) {
        auto lambda = [](typename TileData::DType x, typename TileData::DType y) {
            return x / y;
        };
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TBinSOp<TileData>(dst.data(), src.data(), scalar, row, col, lambda);
    }
    template <typename TileData>
    PTO_INTERNAL void TDIVS_IMPL(TileData &dst, typename TileData::DType scalar, TileData &src) {
        auto lambda = [](typename TileData::DType x, typename TileData::DType y) {
            return y / x;
        };
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TBinSOp<TileData>(dst.data(), src.data(), scalar, row, col, lambda);
    }
    template <typename TileData>
    PTO_INTERNAL void TMINS_IMPL(TileData &dst, TileData &src, typename TileData::DType scalar) {
        auto lambda = [](typename TileData::DType x, typename TileData::DType y) {
            return std::min(x, y);
        };
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        TBinSOp<TileData>(dst.data(), src.data(), scalar, row, col, lambda);
    }
}

#endif 