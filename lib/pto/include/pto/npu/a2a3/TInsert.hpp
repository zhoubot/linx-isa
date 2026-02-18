/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
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
    using SrcType = typename SrcTileData::DType;
    using DstType = typename DstTileData::DType;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(DstType);
    uint32_t dstOffset = DstTileData::Rows * c0Size * (indexCol / c0Size) + (indexRow * c0Size + (indexCol % c0Size));
    __cc__ SrcType *srcAddr = (__cc__ SrcType *)__cce_get_tile_ptr(src);
    __cbuf__ DstType *dstAddr = (__cbuf__ DstType *)__cce_get_tile_ptr(dst) + dstOffset;

    constexpr uint32_t dstStrideD = DstTileData::Rows;
    constexpr uint16_t srcStride = SrcTileData::Rows;
    uint16_t nSize = CeilDivision(validCol, c0Size) * c0Size;
    copy_matrix_cc_to_cbuf(
        dstAddr, srcAddr, 0, nSize, SrcTileData::Rows, dstStrideD,
        srcStride, 0, QuantPre, reluMode, false, false);
}

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
    PTO_ASSERT(indexRow + SrcTileData::Rows <= DstTileData::Rows,
        "The sum of indexRow and srcRow should be less than dstRow!");
    PTO_ASSERT(indexCol + SrcTileData::Cols <= DstTileData::Cols,
        "The sum of indexCol and srcCol should be less than dstCol!");
    if constexpr (SrcTileData::Loc == TileType::Acc && DstTileData::Loc == TileType::Mat) {
        constexpr QuantMode_t quantPre =
            GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>(); 
        TInsertAccToMat<DstTileData, SrcTileData, quantPre, ReluPreMode::NoRelu>(dst.data(), src.data(),
            src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
    }
}

// relu
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
    PTO_ASSERT(indexRow + SrcTileData::Rows <= DstTileData::Rows,
        "The sum of indexRow and srcRow should be less than dstRow!");
    PTO_ASSERT(indexCol + SrcTileData::Cols <= DstTileData::Cols,
        "The sum of indexCol and srcCol should be less than dstCol!");
    constexpr QuantMode_t quantPre = GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}

// scalar quant
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, uint64_t preQuantScalar,
    uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, false>();
    PTO_ASSERT(indexRow + SrcTileData::Rows <= DstTileData::Rows,
        "The sum of indexRow and srcRow should be less than dstRow!");
    PTO_ASSERT(indexCol + SrcTileData::Cols <= DstTileData::Cols,
        "The sum of indexCol and srcCol should be less than dstCol!");
    constexpr QuantMode_t quantPre = GetScalarPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    set_quant_pre(preQuantScalar);
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}

// vector quant
template <typename FpTileData>
__tf__ PTO_INTERNAL void SetFPCInsert(typename FpTileData::TileDType __in__ fp)
{
    using FpType = typename FpTileData::DType;
    __fbuf__ FpType *dstAddrFp = (__fbuf__ FpType *)__cce_get_tile_ptr(fp);
    uint64_t deqTensorAddr = ((uint64_t)dstAddrFp >> static_cast<uint64_t>(7))
                             << 8;  // fpc[15:8] means Quant_PRE_ADDR, uint of 128(2^7)bytes
    set_fpc(deqTensorAddr);
}

template <typename DstTileData, typename SrcTileData, typename FpTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TINSERT_IMPL(DstTileData &dst, SrcTileData &src, FpTileData &fp,
     uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, false>();
    PTO_ASSERT(indexRow + SrcTileData::Rows <= DstTileData::Rows,
        "The sum of indexRow and srcRow should be less than dstRow!");
    PTO_ASSERT(indexCol + SrcTileData::Cols <= DstTileData::Cols,
        "The sum of indexCol and srcCol should be less than dstCol!");
    static_assert(FpTileData::Loc == TileType::Scaling, "Fp only support Scaling.");
    constexpr QuantMode_t quantPre = GetVectorPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    SetFPCInsert<FpTileData>(fp.data());
    TInsertAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        src.GetValidRow(), src.GetValidCol(), indexRow, indexCol);
}
}  // namespace pto
#endif  // TInsert_HPP