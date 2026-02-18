/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TGATHER_HPP
#define TGATHER_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <pto/common/pto_tile.hpp>
#include <pto/common/type.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto {

PTO_INLINE bool MaskSelect(MaskPattern pat, unsigned idx)
{
    constexpr uint16_t INDEX_TWO = 2;
    constexpr uint16_t INDEX_THREE = 3;
    switch (pat) {
        case MaskPattern::P0101: return (idx % 2u) == 0;
        case MaskPattern::P1010: return (idx % 2u) == 1;
        case MaskPattern::P0001: return (idx % 4u) == 0;
        case MaskPattern::P0010: return (idx % 4u) == 1;
        case MaskPattern::P0100: return (idx % 4u) == INDEX_TWO;
        case MaskPattern::P1000: return (idx % 4u) == INDEX_THREE;
        case MaskPattern::P1111: return true;
        default: return true;
    }
}

template <typename IndexT>
PTO_INLINE bool IndexInBounds(IndexT raw, std::size_t n)
{
    if constexpr (std::is_signed_v<IndexT>) {
        if (raw < 0) {
            return false;
        }
    }
    return static_cast<std::size_t>(raw) < n;
}

template <typename DstTileData, typename Src0TileData, typename Src1TileData>
PTO_INTERNAL void CheckValid()
{
    static_assert(
        (sizeof(typename DstTileData::DType) == 2) || (sizeof(typename DstTileData::DType) == 4), "expect b16/b32");
    static_assert((sizeof(typename Src1TileData::DType) == 4), "expect b32");
    static_assert((std::is_same<typename DstTileData::DType, typename Src0TileData::DType>::value),
        "expect same size for indice and dst");
}

template <typename TileDataD, typename TileDataS0, typename TileDataS1>
PTO_INTERNAL void TGather(typename TileDataD::TileDType dst, typename TileDataS0::TileDType src0,
    typename TileDataS1::TileDType src1, unsigned validCol, unsigned validRow)
{
    const std::size_t numel0 = static_cast<std::size_t>(TileDataS0::Rows) * static_cast<std::size_t>(TileDataS0::Cols);
    for (unsigned r = 0; r < validRow; r++) {
        for (unsigned c = 0; c < validCol; c++) {
            const size_t idx1 = GetTileElementOffset<TileDataS1>(r, c);
            const auto raw = src1[idx1];
            const size_t didx = GetTileElementOffset<TileDataD>(r, c);
            if (!IndexInBounds(raw, numel0)) {
                dst[didx] = static_cast<typename TileDataD::DType>(0);
                continue;
            }
            const std::size_t flat = static_cast<std::size_t>(raw);
            const std::size_t srcR = flat / static_cast<std::size_t>(TileDataS0::Cols);
            const std::size_t srcC = flat % static_cast<std::size_t>(TileDataS0::Cols);
            const size_t sidx = GetTileElementOffset<TileDataS0>(srcR, srcC);
            dst[didx] = static_cast<typename TileDataD::DType>(src0[sidx]);
        }
    }
}

template <typename TileDataD, typename TileDataS0, typename TileDataS1>
PTO_INTERNAL void TGATHER_IMPL(TileDataD &dst, TileDataS0 &src0, TileDataS1 &src1)
{
    CheckValid<TileDataD, TileDataS0, TileDataS1>();

    unsigned validCol = dst.GetValidCol();
    unsigned validRow = dst.GetValidRow();

    TGather<TileDataD, TileDataS0, TileDataS1>(dst.data(), src0.data(), src1.data(), validCol, validRow);
}

template <typename DstTileData, typename SrcTileData, MaskPattern maskPattern>
PTO_INTERNAL void TGather(typename DstTileData::TileDType dst, typename SrcTileData::TileDType src,
    unsigned validRow, unsigned validCol)
{
    size_t didx = GetTileElementOffset<DstTileData>(0, 0);
    for (unsigned r = 0; r < validRow; r++) {
        for (unsigned c = 0; c < validCol; c++) {
            if (!MaskSelect(maskPattern, c))
                continue;
            const size_t sidx = GetTileElementOffset<SrcTileData>(r, c);
            dst[didx] = static_cast<typename DstTileData::DType>(src[sidx]);
            didx++;
        }
    }
}

template <typename DstTileData, typename SrcTileData, MaskPattern maskPattern>
PTO_INTERNAL void TGATHER_IMPL(DstTileData &dst, SrcTileData &src)
{
    using T = typename SrcTileData::DType;
    static_assert(sizeof(T) == 2 || sizeof(T) == 4, "TGATHER: src element type must be 16 or 32-bit wide");
    static_assert(
        (DstTileData::Loc == TileType::Vec) && (SrcTileData::Loc == TileType::Vec), "TGATHER: expect vec TileType");
    static_assert((DstTileData::isRowMajor && SrcTileData::isRowMajor), "TGATHER: expect row major");
    static_assert((sizeof(typename DstTileData::DType) == sizeof(T)), "TGATHER: expect same type size for dst and src");
    assert(dst.GetValidCol() == DstTileData::Cols);
    TGather<DstTileData, SrcTileData, maskPattern>(dst.data(), src.data(), src.GetValidRow(), src.GetValidCol());
}

} // namespace pto

#endif // TGATHER_HPP
