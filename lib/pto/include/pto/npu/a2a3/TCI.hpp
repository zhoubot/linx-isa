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

#include <pto/common/constants.hpp>

namespace pto {
template <typename TileData, typename T>
PTO_INTERNAL void CheckValid() {
    static_assert((std::is_same<typename TileData::DType, T>::value), "Fix: TCI expect src and dst same datatype");
    static_assert((sizeof(typename TileData::DType) == 4 || (sizeof(typename TileData::DType) == 2)),
        "Fix: TCI expect datatype is b32 or b16");
    static_assert((TileData::Cols != 1), "Fix: TCI expect tile row is 1");
}

template <typename TileData, typename T, int descending>
__tf__ AICORE void TCI(typename TileData::TileDType __out__ dst, T start, unsigned validCol) {
    __ubuf__ typename TileData::DType *dstPtr = (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(dst);

    // scalar
    if constexpr (descending) {
        for (int32_t i = 0; i < validCol; i++) {
            *(dstPtr + i) = start - i;
        }
    } else {
        for (int32_t i = 0; i < validCol; i++) {
            *(dstPtr + i) = start + i;
        }
    }
}

template <typename TileData, typename T, int descending>
PTO_INTERNAL void TCI_IMPL(TileData &dst, T start) {
    CheckValid<TileData, T>();

    unsigned validCol = dst.GetValidCol();

    TCI<TileData, T, descending>(dst.data(), start, validCol);
}
} // namespace pto
#endif
