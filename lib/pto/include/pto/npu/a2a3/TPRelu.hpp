/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPRELU_HPP
#define TPRELU_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

namespace pto {

template <typename T, typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
__tf__ PTO_INTERNAL void TPRelu(typename TileDataDst::TileDType __out__ dstData,
    typename TileDataSrc0::TileDType __in__ src0Data, typename TileDataSrc1::TileDType __in__ src1Data,
    unsigned validRow, unsigned validCol)
{
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0Data);
    __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1Data);

    constexpr unsigned dstStride = TileDataDst::RowStride;
    constexpr unsigned src0Stride = TileDataSrc0::RowStride;
    constexpr unsigned src1Stride = TileDataSrc1::RowStride;

    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            const unsigned di = r * dstStride + c;
            const unsigned xIdx = r * src0Stride + c;
            const unsigned aIdx = r * src1Stride + c;
            const float x = static_cast<float>(src0Ptr[xIdx]);
            const float a = static_cast<float>(src1Ptr[aIdx]);
            const float y = (x > 0.0f) ? x : (x * a);
            dstPtr[di] = static_cast<T>(y);
        }
    }
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TPRELU_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1)
{
    using T = typename TileDataSrc0::DType;
    static_assert(std::is_same_v<T, typename TileDataDst::DType> && std::is_same_v<T, typename TileDataSrc1::DType>,
        "Fix: TPRELU input/output data types must be consistent.");
    static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc0::Loc == TileType::Vec && TileDataSrc1::Loc == TileType::Vec,
        "Fix: TPRELU only supports Vec tiles.");
    static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc0::SFractal == SLayout::NoneBox &&
                      TileDataSrc1::SFractal == SLayout::NoneBox,
        "Fix: TPRELU only supports non-boxed (ND/DN) Vec tiles.");
    static_assert(TileDataDst::isRowMajor && TileDataSrc0::isRowMajor && TileDataSrc1::isRowMajor,
        "Fix: TPRELU only supports row-major Vec tiles.");
    static_assert(std::is_same_v<T, half> || std::is_same_v<T, float>,
        "Fix: TPRELU only supports f16/f32.");

    const unsigned validRow = dst.GetValidRow();
    const unsigned validCol = dst.GetValidCol();
    PTO_ASSERT(validRow > 0 && validCol > 0, "Fix: TPRELU dst valid shape must be non-zero.");
    PTO_ASSERT(validRow == src0.GetValidRow() && validCol == src0.GetValidCol(),
        "Fix: TPRELU dst/src0 valid shape mismatch.");
    PTO_ASSERT(validRow == src1.GetValidRow() && validCol == src1.GetValidCol(),
        "Fix: TPRELU dst/src1 valid shape mismatch.");

    TPRelu<T, TileDataDst, TileDataSrc0, TileDataSrc1>(dst.data(), src0.data(), src1.data(), validRow, validCol);
}

} // namespace pto

#endif // TPRELU_HPP
