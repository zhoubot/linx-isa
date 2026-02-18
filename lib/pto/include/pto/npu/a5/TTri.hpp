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
#include <pto/npu/a5/common.hpp>
#include <pto/npu/a5/utils.hpp>

namespace pto {
template <typename TileData, unsigned rowStride, int upperOrLower, int diagonal>
__tf__ PTO_INTERNAL void TTri(typename TileData::TileDType __out__ dst, unsigned validRows, unsigned validCols) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    unsigned numRepeatPerRow = CeilDivision(validCols, elementsPerRepeat);
    static constexpr int start_num = (upperOrLower == 0) ? (diagonal + 1) : diagonal;
    __VEC_SCOPE__ {
        RegTensor<T> v_ones, v_zeros, vreg_out;
        vector_s32  vreg_idx;
        vector_bool preg_cmp;
        vbr(v_ones, (T)1);
        vbr(v_zeros, (T)0);
        constexpr auto distValue =
            std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();        
        for (uint16_t i = 0; i < (uint16_t) validRows; ++i) {
            uint32_t num_elements = validCols;
            for (uint16_t j = 0; j < (uint16_t) numRepeatPerRow; ++j){
                vector_bool preg_st = CreatePredicate<T>(num_elements);
                vci(vreg_idx, j * elementsPerRepeat);
                vcmps_lt(preg_cmp, vreg_idx, (int)(i+start_num), preg_st);
                if constexpr (upperOrLower == 0)
                    vsel(vreg_out, v_ones, v_zeros, preg_cmp);
                else
                    vsel(vreg_out, v_zeros, v_ones, preg_cmp);
                vsts(vreg_out, dstPtr, i * rowStride + j * elementsPerRepeat, distValue, preg_st);
            }
        }
    }
}

template <typename TileData, int upperOrLower, int diagonal>
PTO_INTERNAL void TTRI_IMPL(TileData &dst) {
    using T = typename TileData::DType;
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int16_t>::value ||
                      std::is_same<T, int8_t>::value || std::is_same<T, uint32_t>::value ||
                      std::is_same<T, uint16_t>::value || std::is_same<T, uint8_t>::value ||
                      std::is_same<T, half>::value || std::is_same<T, float16_t>::value ||
                      std::is_same<T, float32_t>::value || std::is_same<T, bfloat16_t>::value,
        "Fix: TTRI has invalid data type.");
    TTri<TileData, TileData::RowStride, upperOrLower, diagonal>(dst.data(), dst.GetValidRow(), dst.GetValidCol());
}
} // namespace pto

#endif // TTRI_HPP