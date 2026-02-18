/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSEL_HPP
#define TSEL_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

namespace pto {
enum class SELMODE : uint8_t {
    VSEL_CMPMASK_SPR = 0,
    VSEL_TENSOR_SCALAR_MODE = 1,
    VSEL_TENSOR_TENSOR_MODE = 2,
};

template <typename TileData, typename MaskTile>
__tf__ PTO_INTERNAL void TSelScalar(typename TileData::TileDType __out__ dst, typename MaskTile::TileDType __in__ selMask,
    typename TileData::TileDType __in__ src0, typename TileData::TileDType __in__ src1, unsigned validRow,
    unsigned validCol, unsigned validMaskCol) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
    __ubuf__ uint8_t *maskPtr = (__ubuf__ uint8_t *)__cce_get_tile_ptr(selMask);

    constexpr unsigned dstStride = TileData::RowStride;
    constexpr unsigned src0Stride = TileData::RowStride;
    constexpr unsigned src1Stride = TileData::RowStride;
    constexpr unsigned maskStride = MaskTile::RowStride;

    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            const unsigned maskByte = (c >> 3);
            const unsigned maskBit = (c & 7u);
            const uint8_t packed = maskPtr[r * maskStride + maskByte];
            const uint8_t bit = (packed >> maskBit) & 1u;

            const unsigned di = r * dstStride + c;
            const unsigned s0i = r * src0Stride + c;
            const unsigned s1i = r * src1Stride + c;
            dstPtr[di] = bit ? src0Ptr[s0i] : src1Ptr[s1i];
        }
    }

    // Silence unused param warnings in non-assert builds.
    (void)validMaskCol;
}

template <typename TileData, typename MaskTile>
PTO_INTERNAL void TSEL_IMPL(TileData &dst, MaskTile &selMask, TileData &src0, TileData &src1) {
    static_assert(TileData::isRowMajor, "Fix: TSEL has not supported layout type.");
    static_assert(TileData::Loc == TileType::Vec, "Fix: TSEL only supports Vec tiles.");
    static_assert(MaskTile::Loc == TileType::Vec, "Fix: TSEL mask must be a Vec tile.");
    static_assert(std::is_same_v<typename MaskTile::DType, uint8_t>, "Fix: TSEL expects a packed u8 mask tile.");
    static_assert(TileData::SFractal == SLayout::NoneBox && MaskTile::SFractal == SLayout::NoneBox,
        "Fix: TSEL only supports non-boxed (ND/DN) tiles.");
    static_assert(TileData::isRowMajor && MaskTile::isRowMajor, "Fix: TSEL only supports row-major ND tiles.");
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();
    unsigned validMaskCol = selMask.GetValidCol();
    PTO_ASSERT(validMaskCol == static_cast<unsigned>(CeilDivision(static_cast<int32_t>(validCol), 8)),
        "Fix: TSEL mask validCol must equal ceil(dst validCol / 8).");

    TSelScalar<TileData, MaskTile>(
        dst.data(), selMask.data(), src0.data(), src1.data(), validRow, validCol, validMaskCol);
}
} // namespace pto
#endif
