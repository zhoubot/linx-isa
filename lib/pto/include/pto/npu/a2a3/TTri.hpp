/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TTRI_HPP
#define TTRI_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

namespace pto {

// lower-triangular
template <typename T, int diagonal, unsigned rowStride>
PTO_INTERNAL void TTril(__ubuf__ T *dstPtr, unsigned validRow, unsigned validCol) {
    T one = static_cast<T>(1);
    T zero = static_cast<T>(0);
    for (unsigned i = 0; i < validRow; ++i) {
        __ubuf__ T *drow = dstPtr + i * rowStride;

        // write full zero first
        set_vector_mask(0, validCol);
        vector_dup(drow, zero, 1, 1, 1, 8, 0);
        pipe_barrier(PIPE_V);

        // write one
        int lastCol = static_cast<int>(i) + static_cast<int>(diagonal);
        if (lastCol >= 0) {
            int want = lastCol + 1;
            unsigned fillCol = (want >= static_cast<int>(validCol) ? validCol : static_cast<unsigned>(want));
            set_vector_mask(0, fillCol);
            vector_dup(drow, one, 1, 1, 1, 8, 0);
            pipe_barrier(PIPE_V);
        }
    }
}

// upper-triangular
template <typename T, int diagonal, unsigned rowStride>
PTO_INTERNAL void TTriu(__ubuf__ T *dstPtr, unsigned validRow, unsigned validCol) {
    T one = static_cast<T>(1);
    T zero = static_cast<T>(0);
    for (unsigned i = 0; i < validRow; ++i) {
        __ubuf__ T *drow = dstPtr + i * rowStride;

        // write full one first
        set_vector_mask(0, validCol);
        vector_dup(drow, one, 1, 1, 1, 8, 0);
        pipe_barrier(PIPE_V);

        // write zero
        int lastCol = static_cast<int>(i) + static_cast<int>(diagonal);
        if (lastCol >= 0) {
            unsigned fillCol = (lastCol >= static_cast<int>(validCol) ? validCol : static_cast<unsigned>(lastCol));
            set_vector_mask(0, fillCol);
            vector_dup(drow, zero, 1, 1, 1, 8, 0);
            pipe_barrier(PIPE_V);
        }
    }
}

template <typename TileData, int isUpperOrLower, int diagonal, unsigned rowStride>
__tf__ PTO_INTERNAL void TTri(typename TileData::TileDType __out__ dst, unsigned validRow, unsigned validCol) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);

    set_mask_count();
    if constexpr (isUpperOrLower == 0) {
        TTril<T, diagonal, rowStride>(dstPtr, validRow, validCol);
    } else {
        TTriu<T, diagonal, rowStride>(dstPtr, validRow, validCol);
    }
    set_mask_norm();
    set_vector_mask(-1, -1);
    return;
}

template <typename TileData, int isUpperOrLower>
PTO_INTERNAL void TTriCheck(const TileData &dst) {
    using T = typename TileData::DType;
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value || std::is_same<T, int16_t>::value ||
                      std::is_same<T, uint32_t>::value || std::is_same<T, uint16_t>::value ||
                      std::is_same<T, half>::value || std::is_same<T, float16_t>::value ||
                      std::is_same<T, float>::value || std::is_same<T, float32_t>::value,
        "Fix: TTRI has invalid data type.");
    static_assert(isUpperOrLower == 0 || isUpperOrLower == 1, "Fix: isUpperOrLower must be 0 or 1.");
    static_assert(TileData::isRowMajor, "Fix: TTRI only support row major layout.");
}

template <typename TileData, int isUpperOrLower, int diagonal>
PTO_INTERNAL void TTRI_IMPL(TileData &dst) {
    TTriCheck<TileData, isUpperOrLower>(dst);
    constexpr unsigned rowStride = TileData::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    TTri<TileData, isUpperOrLower, diagonal, rowStride>(dst.data(), validRow, validCol);
}

} // namespace pto
#endif