/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TCI_HPP
#define TCI_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto {
    template <typename TileData, typename T, int descending>
    PTO_INTERNAL void Tci_IMPL(typename TileData::TileDType dst, T start, unsigned validCol) {
        for(int j = 0; j < validCol; j++){
            int idx = GetTileElementOffset<TileData>(0, j);
            if constexpr (descending == 1)
                dst[idx] = start-j;
            else 
                dst[idx] = start+j;
        }
    }

    template <typename TileData, typename T, int descending>
    PTO_INTERNAL void TCI_IMPL(TileData &dst, T index) {
        static_assert((TileData::Rows == 1), "TCI only support 1 row tile");
        static_assert((std::is_same<typename TileData::DType, T>::value), "TCI data type must match tile data type");
        Tci_IMPL<TileData, T, descending>(dst.data(), index, dst.GetValidCol());
    }
}
#endif
