/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TMOV_HPP
#define TMOV_HPP
#include "common.hpp"
#include "TExtract.hpp"
#include "TCopy.hpp"

namespace pto {
template <typename DstTileData, typename SrcTileData>
__tf__ AICORE void TMovToBt(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src)
{
    using SrcType = typename SrcTileData::DType;
    using DstType = typename DstTileData::DType;
    constexpr int32_t srcRow = SrcTileData::Rows;
    constexpr int32_t srcCol = SrcTileData::Cols;
    constexpr const int BURST_LEN_UNIT = 64;

    if constexpr (std::is_same<SrcType, int32_t>::value || std::is_same<SrcType, float>::value) {
        static_assert(
            std::is_same<DstType, SrcType>::value, "TMov: Destination and Source tile data types must be the same.");
    } else if constexpr (std::is_same<SrcType, half>::value) {
        static_assert(std::is_same<DstType, float>::value,
            "TMov: When Source tile data types is half, dst tile data types must be float");
    }
    static_assert(SrcTileData::Rows == 1, "TMov: When TileType is Bias, row must be 1");
    static_assert(SrcTileData::Cols * sizeof(SrcType) % BURST_LEN_UNIT == 0,
        "TMov: When TileType is Bias, col * sizeof(srcDType) must be aligned to 64");

    __cbuf__ SrcType *srcAddrP = (__cbuf__ SrcType *)(src);
    uint64_t dstAddrP = (uint64_t)dst;

    uint16_t convControl = 0;
    constexpr uint16_t burstLen = srcRow * srcCol * sizeof(SrcType) / BURST_LEN_UNIT;

    if constexpr (std::is_same_v<SrcType, half> && std::is_same_v<DstType, float>) {
        convControl = 1;
    }
    copy_cbuf_to_bt(dstAddrP, srcAddrP, convControl, (uint16_t)1, burstLen, (uint16_t)0, (uint16_t)0);
}

template <typename DstTileData, typename SrcTileData>
__tf__ AICORE void TMovToFb(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src)
{
    using SrcType = typename SrcTileData::DType;
    using DstType = typename DstTileData::DType;
    constexpr int32_t srcRow = SrcTileData::Rows;
    constexpr int32_t srcCol = SrcTileData::Cols;
    constexpr const int BURST_LEN_UNIT = 128;
    constexpr const int RELU_BIT = 16;

    static_assert(
        std::is_same<DstType, SrcType>::value, "TMov: Destination and Source tile data types must be the same.");
    static_assert(std::is_same<DstType, uint64_t>::value, "TMov: Invalid data type.");
    static_assert(SrcTileData::Rows == 1, "TMov: When TileType is Scaling, row must be 1");
    static_assert(SrcTileData::Cols * sizeof(SrcType) % BURST_LEN_UNIT == 0,
        "TMov: When TileType is Scaling, col * sizeof(srcType) must be aligned to 128");

    __cbuf__ SrcType *srcAddrP = (__cbuf__ SrcType *)(src);
    __fbuf__ DstType *dstAddr = (__fbuf__ DstType *)(dst);
    constexpr bool isRelu = 0;
    __fbuf__ DstType *dstAddrP = (__fbuf__ DstType *)(dstAddr || (isRelu << RELU_BIT));

    constexpr uint16_t burstLen = srcRow * srcCol * sizeof(SrcType) / BURST_LEN_UNIT;
    copy_cbuf_to_fbuf(dstAddrP, srcAddrP, (uint16_t)1, burstLen, (uint16_t)0, (uint16_t)0);
}

template <typename DstTileData, typename SrcTileData>
AICORE void TMovToVec(DstTileData &dst, SrcTileData &src)
{
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename SrcTileData::DType);
    constexpr unsigned srcStride = SrcTileData::RowStride;
    constexpr unsigned dstStride = DstTileData::RowStride;
    uint64_t validSrcRow = src.GetValidRow();
    uint64_t validSrcCol = src.GetValidCol();
    uint64_t validDstRow = dst.GetValidRow();
    uint64_t validDstCol = dst.GetValidCol();
    uint64_t validRow = (validSrcRow < validDstRow) ? validSrcRow : validDstRow;
    uint64_t validCol = (validSrcCol < validDstCol) ? validSrcCol : validDstCol;
    TCopy<DstTileData, SrcTileData, blockSizeElem, srcStride, dstStride>(dst.data(), src.data(), validRow, validCol);
}

template <typename DstTileData, typename SrcTileData, QuantMode_t QuantPre, ReluPreMode reluMode>
__tf__ AICORE void TMovCcToCb(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t validRow, uint16_t validCol)
{
    using SrcType = typename SrcTileData::DType;
    using DstType = typename DstTileData::DType;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(DstType);
    __cc__ SrcType *srcAddr = (__cc__ SrcType *)__cce_get_tile_ptr(src);
    __cbuf__ DstType *dstAddr = (__cbuf__ DstType *)__cce_get_tile_ptr(dst);

    constexpr uint32_t dstStride_dst_D = DstTileData::Rows;
    constexpr uint16_t srcStride = SrcTileData::Rows;
    validCol = CeilDivision(validCol, c0Size) * c0Size;
    copy_matrix_cc_to_cbuf(
        dstAddr, srcAddr, 0, validCol, SrcTileData::Rows, dstStride_dst_D, 
        srcStride, 0, QuantPre, reluMode, false, false);
}

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TMovToLeft(DstTileData &dst, SrcTileData &src)
{
    if constexpr (DstTileData::SFractal == SrcTileData::SFractal) {
        if constexpr (DstTileData::Compact == CompactMode::Normal) {
            TExtractToACompact<DstTileData, SrcTileData, false>(
                dst.data(), src.data(), 0, 0, dst.GetValidRow(), dst.GetValidCol(), dst.GetKAligned());
        } else {
            TExtractToA<DstTileData, SrcTileData, false>(dst.data(), src.data(), 0, 0);
        }
    } else {
        if constexpr (DstTileData::Compact == CompactMode::Normal || sizeof(typename SrcTileData::DType) == 1) {
            TExtractToACompact<DstTileData, SrcTileData, true>(
                dst.data(), src.data(), 0, 0, dst.GetValidRow(), dst.GetValidCol(), dst.GetKAligned());
        } else {
            TExtractToA<DstTileData, SrcTileData, true>(dst.data(), src.data(), 0, 0);
        }
    }
}

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TMovToRight(DstTileData &dst, SrcTileData &src)
{
    if constexpr (DstTileData::SFractal == SrcTileData::SFractal) {
        if constexpr (DstTileData::Compact == CompactMode::Normal) {
            TExtractToBCompact<DstTileData, SrcTileData, false>(
                dst.data(), src.data(), 0, 0, dst.GetValidRow(), dst.GetValidCol());
        } else {
            TExtractToB<DstTileData, SrcTileData, false>(dst.data(), src.data(), 0, 0);
        }
    } else {
        if constexpr (DstTileData::Compact == CompactMode::Normal || sizeof(typename SrcTileData::DType) == 1) {
            TExtractToBCompact<DstTileData, SrcTileData, true>(
                dst.data(), src.data(), 0, 0, dst.GetValidRow(), dst.GetValidCol());
        } else {
            TExtractToB<DstTileData, SrcTileData, true>(dst.data(), src.data(), 0, 0);
        }
    }
}

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src)
{
    static_assert((SrcTileData::Rows == DstTileData::Rows) && ((SrcTileData::Cols == DstTileData::Cols)),
        "TMov: The shape of src needs to be the same as that of dst.");
    static_assert((SrcTileData::Loc == TileType::Mat &&
                      (DstTileData::Loc == TileType::Left || DstTileData::Loc == TileType::Right ||
                          DstTileData::Loc == TileType::Bias || DstTileData::Loc == TileType::Scaling)) ||
                      (DstTileData::Loc == TileType::Vec && SrcTileData::Loc == TileType::Vec) ||
                      (DstTileData::Loc == TileType::Mat && SrcTileData::Loc == TileType::Acc),
        "TMov: Invalid TileType.");
    if constexpr (SrcTileData::Loc == TileType::Mat && DstTileData::Loc == TileType::Left) {
        TMovToLeft<DstTileData, SrcTileData>(dst,src);
    } else if constexpr (SrcTileData::Loc == TileType::Mat && DstTileData::Loc == TileType::Right) {
        TMovToRight<DstTileData, SrcTileData>(dst,src);
    } else if constexpr (SrcTileData::Loc == TileType::Mat && DstTileData::Loc == TileType::Bias) {
        TMovToBt<DstTileData, SrcTileData>(dst.data(), src.data());
    } else if constexpr (SrcTileData::Loc == TileType::Mat && DstTileData::Loc == TileType::Scaling) {
        TMovToFb<DstTileData, SrcTileData>(dst.data(), src.data());
    } else if constexpr (SrcTileData::Loc == TileType::Vec && DstTileData::Loc == TileType::Vec) {
        TMovToVec<DstTileData, SrcTileData>(dst, src);
    } else if constexpr (SrcTileData::Loc == TileType::Acc && DstTileData::Loc == TileType::Mat) {
        CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
        uint16_t m = src.GetValidRow();
        uint16_t n = src.GetValidCol();
        constexpr QuantMode_t quantPre =
            GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
        TMovCcToCb<DstTileData, SrcTileData, quantPre, ReluPreMode::NoRelu>(dst.data(), src.data(), m, n);
    }
}

// relu
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode>
PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
    uint16_t m = src.GetValidRow();
    uint16_t n = src.GetValidCol();
    constexpr QuantMode_t quantPre = GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    TMovCcToCb<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(), m, n);
}

// scalar quant
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src, uint64_t preQuantScalar)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, false>();
    uint16_t m = src.GetValidRow();
    uint16_t n = src.GetValidCol();
    constexpr QuantMode_t quantPre = GetScalarPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    set_quant_pre(preQuantScalar);
    TMovCcToCb<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(), m, n);
}

// vector quant
template <typename FpTileData>
__tf__ PTO_INTERNAL void SetFPC(typename FpTileData::TileDType __in__ fp)
{
    __fbuf__ typename FpTileData::DType *dstAddrFp = (__fbuf__ typename FpTileData::DType *)__cce_get_tile_ptr(fp);
    uint64_t deqTensorAddr = ((uint64_t)dstAddrFp >> static_cast<uint64_t>(7))
                             << 8;  // fpc[15:8] means Quant_PRE_ADDR, uint of 128(2^7)bytes
    set_fpc(deqTensorAddr);
}

template <typename DstTileData, typename SrcTileData, typename FpTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src, FpTileData &fp)
{
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, false>();
    static_assert(FpTileData::Loc == TileType::Scaling, "Fp only support Scaling.");
    constexpr QuantMode_t quantPre = GetVectorPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    uint16_t m = src.GetValidRow();
    uint16_t n = src.GetValidCol();
    SetFPC<FpTileData>(fp.data());
    TMovCcToCb<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(), m, n);
}
}  // namespace pto
#endif  // TMOV_HPP
