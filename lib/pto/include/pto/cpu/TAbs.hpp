/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TABS_HPP
#define TABS_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto{

    template <typename tile_shape>
    void TAbs_Impl(typename tile_shape::TileDType dst,
                            typename tile_shape::TileDType src,
                            unsigned validRow, unsigned validCol
                        ) {
        for(size_t c=0; c<validCol; c++) {
            for(size_t r=0; r<validRow; r++) {
                size_t idx = GetTileElementOffset<tile_shape>(r,c);
                dst[idx] = src[idx] < 0 ? -src[idx] : src[idx];
            }
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TABS_IMPL(tile_shape &dst, tile_shape &src) {
        static_assert(std::is_same<typename tile_shape::DType, int32_t>::value ||
                      std::is_same<typename tile_shape::DType, int>::value ||
                      std::is_same<typename tile_shape::DType, int16_t>::value ||
                      std::is_same<typename tile_shape::DType, half>::value ||
                      std::is_same<typename tile_shape::DType, float>::value,
                      "TABS: Invalid data type");
        TAbs_Impl<tile_shape>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
    }
}
#endif  // TABS_HPP 
