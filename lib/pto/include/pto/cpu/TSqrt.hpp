/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TSQRT_HPP
#define TSQRT_HPP

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include <cmath>

namespace pto{

    template <typename tile_type>
    void TSqrt_Impl(typename tile_type::TileDType dst,
                            typename tile_type::TileDType src,
                            int validRow, int validCol
                        ) {
        for(size_t c=0; c<(size_t)validCol; c++) {
            for(size_t r=0; r<(size_t)validRow; r++) {
                size_t idx = GetTileElementOffset<tile_type>(r,c);
                dst[idx] = static_cast<typename tile_type::DType>(std::sqrt(static_cast<double>(src[idx])));
            }
        }
    }

    template <typename tile_type>
    PTO_INTERNAL void TSQRT_IMPL(tile_type &dst, tile_type &src) {
        static_assert(std::is_same<typename tile_type::DType, half>::value ||
                      std::is_same<typename tile_type::DType, float>::value,
                      "TSQRT: Invalid data type");
        TSqrt_Impl<tile_type>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
    }
}
#endif  // TSQRT_HPP
