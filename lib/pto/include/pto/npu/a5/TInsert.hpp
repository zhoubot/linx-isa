/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TINSERT_HPP
#define TINSERT_HPP
#include "common.hpp"

namespace pto {
template <typename DstTileData, typename SrcTileData, QuantMode_t QuantPre, ReluPreMode reluMode>
__tf__ PTO_INTERNAL void TInsertAccToMat(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t validRow, uint16_t validCol, uint16_t indexRow, uint16_t indexCol)
{
    using dstType = typename DstTileData::DType;
    constexpr bool channelSplitEnable = (!DstTileData::isRowMajor && (DstTileData::SFractal == SLayout::RowMajor)) &&
                                        (std::is_same_v<dstType, float>) &&
                                        (DstTileData::SFractalSize == 512);
    constexpr int32_t c0Size = (!channelSplitEnable) && (DstTileData::SFractalSize == 1024) ?
                                    2 * C0_SIZE_BYTE / sizeof(dstType) :
                                    C0_SIZE_BYTE / sizeof(dstType);
    uint32_t dstOffset = DstTileData::Rows * c0Size * (indexCol / c0Size) + (indexRow * c0Size + (indexCol % c0Size));
    constexpr uint32_t dstStride = DstTileData::Rows * c0Size;
    uint16_t nSize = CeilDivision(validCol, c0Size) * c0Size;
    __cbuf__ dstType *dstAddr = (__cbuf__ dstType *)__cce_get_tile_ptr(dst) + dstOffset;
    __cc__ typename SrcTileData::DType *srcData = (__cc__ typename SrcTileData::DType *)(src);

    copy_matrix_cc_to_cbuf(dstAddr, srcData, 0, nSize, SrcTileData::Rows, dstStride, SrcTileData::Rows, 
        0, 0, 0, QuantPre, reluMode, channelSplitEnable, false, 0, 0, false, false, 0, false, false,
        false, false, false, false);
}

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    static_assert((DstTileData::Loc == TileType::Mat), "Destination TileType only support Mat.");
    static_assert((!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::RowMajor),
        "Dst fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    CheckTMovAccValid<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType>();
    constexpr QuantMode_t quantPre =
        GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>(); 
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, ReluPreMode::NoRelu>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}

template <typename FpTileData>
__tf__ PTO_INTERNAL void SetFPCInsert(typename FpTileData::TileDType __in__ fp)
{
    __fbuf__ typename FpTileData::DType *dstAddrFp = (__fbuf__ typename FpTileData::DType *)__cce_get_tile_ptr(fp);
    uint64_t deqTensorAddr = ((uint64_t)dstAddrFp >> static_cast<uint64_t>(7)) << 8;
    set_fpc(deqTensorAddr);
}

// relu
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTMovAccValid<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType>();
    static_assert((DstTileData::Loc == TileType::Mat), "Destination TileType only support Mat.");
    static_assert((!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::RowMajor),
        "Dst fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    constexpr QuantMode_t quantPre = GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}

// scalar quant
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint64_t preQuantScalar,
    uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTMovAccValid<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
    static_assert((DstTileData::Loc == TileType::Mat), "Destination TileType only support Mat.");
    static_assert((!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::RowMajor),
        "Dst fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    constexpr QuantMode_t quantPre = GetScalarPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    set_quant_pre(preQuantScalar);
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}

// vector quant
template <typename DstTileData, typename SrcTileData, typename FpTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, FpTileData &fp,
     uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    static_assert((DstTileData::Loc == TileType::Mat), "Destination TileType only support Mat.");
    CheckTMovAccValid<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
    static_assert((!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::RowMajor),
        "Dst fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    constexpr QuantMode_t quantPre = GetVectorPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    SetFPCInsert<FpTileData>(fp.data());
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}
}  // namespace pto
#endif  // TInsert_HPP