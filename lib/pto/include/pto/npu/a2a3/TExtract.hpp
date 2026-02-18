/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TEXTRACT_HPP
#define TEXTRACT_HPP
#include "common.hpp"

namespace pto {

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol, int32_t dstRow, int32_t dstCol>
PTO_INTERNAL void TExtractToANonTranspose(
    __ca__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow, uint16_t indexCol)
{
    constexpr int config = srcRow | (1u << 16);
    set_fmatrix(config);
    img2colv2_cbuf_to_ca(
        dstAddr, srcAddr, dstCol, dstRow, indexCol, indexRow, 1, 1, 1, 1, 1, 1, false, false, false, false, srcCol);
}

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol, int32_t dstRow, int32_t dstCol>
PTO_INTERNAL void TExtractToATranspose(
    __ca__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow, uint16_t indexCol)
{
    // b8采用Load2D转置
    if constexpr (sizeof(SrcType) == 1) {
        constexpr uint16_t fractNum = 2;
        constexpr uint16_t srcColNum = srcCol >> (SHIFT_BLOCK_LEN + fractNum - 1);
        constexpr uint16_t dstColNum = dstCol * sizeof(SrcType) >> SHIFT_BLOCK_BYTE;
        constexpr uint16_t dstRowNum = dstRow >> (SHIFT_BLOCK_LEN + fractNum - 1);
        uint16_t dstGap = 0;
        uint16_t dstFracGap = 0;
        uint16_t startIdx0 = (indexCol >> (SHIFT_BLOCK_LEN + fractNum - 1)) +
                             (indexRow * srcColNum * sizeof(SrcType) >> SHIFT_BLOCK_BYTE);
        // 判断行优先&列优先的搬运路径，减少for循环次数
        if constexpr (dstRowNum >= dstColNum) {
            dstGap = fractNum * dstColNum - 1;
            dstFracGap = dstColNum - 1;
            for (uint16_t i = 0; i < dstColNum; i++) {
                load_cbuf_to_ca_transpose(
                    dstAddr, srcAddr, startIdx0 + i, dstRowNum, srcColNum, dstGap, false, dstFracGap);
                dstAddr += CUBE_BLOCK_SIZE;
            }
        } else {
            dstFracGap = dstColNum - 1;
            for (uint16_t i = 0; i < dstRowNum; i++) {
                load_cbuf_to_ca_transpose(
                    dstAddr, srcAddr, startIdx0 + i * srcColNum, dstColNum, 1, 0, false, dstFracGap);
                dstAddr += dstColNum * CUBE_BLOCK_SIZE * fractNum;
            }
        }
    } else {
        // b16和b32采用load3DV2转置，减少scalar次数
        constexpr int config = srcCol | (1u << 16);
        set_fmatrix(config);
        img2colv2_cbuf_to_ca(
            dstAddr, srcAddr, dstRow, dstCol, indexRow, indexCol, 1, 1, 1, 1, 1, 1, false, false, true, false, srcRow);
    }
}
template <typename DstTileData, typename SrcTileData, bool Transpose>
__tf__ AICORE void TExtractToA(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t indexRow, uint16_t indexCol)
{
    using SrcType = std::conditional_t<(sizeof(typename SrcTileData::DType) == 2), half, typename SrcTileData::DType>;
    using DstType = std::conditional_t<(sizeof(typename DstTileData::DType) == 2), half, typename DstTileData::DType>;
    __cbuf__ SrcType *srcAddr = (__cbuf__ SrcType *)__cce_get_tile_ptr(src);
    __ca__ DstType *dstAddr = (__ca__ DstType *)__cce_get_tile_ptr(dst);

    constexpr int32_t srcRow = SrcTileData::Rows;
    constexpr int32_t srcCol = SrcTileData::Cols;
    constexpr int32_t dstRow = DstTileData::Rows;
    constexpr int32_t dstCol = DstTileData::Cols;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(SrcType);
    constexpr int32_t fractalSize = (sizeof(SrcType) == 1) ? 32 : 16;

    if constexpr (!Transpose) {
        // srcRow/srcCol/dstRow/dstCol对齐校验
        static_assert((srcRow % 16) == 0, "srcRow must be aligned to 16");
        static_assert((srcCol % c0Size) == 0, "srcCol must be aligned to C0Size");
        static_assert((dstRow % 16) == 0, "dstRow must be aligned to 16");
        static_assert((dstCol % c0Size) == 0, "dstCol must be aligned to C0Size");
        PTO_ASSERT((indexRow % 16) == 0, "indexRow must be aligned to 16");
        PTO_ASSERT((indexCol % c0Size) == 0, "indexCol must be aligned to C0Size");
        TExtractToANonTranspose<SrcType, DstType, srcRow, srcCol, dstRow, dstCol>(dstAddr, srcAddr, indexRow, indexCol);
    } else {
        // L1->L0A:load_cbuf_to_ca_transpose
        static_assert((srcRow % fractalSize) == 0, "srcRow must be aligned");
        static_assert((srcCol % fractalSize) == 0, "srcCol must be aligned");
        static_assert((dstRow % fractalSize) == 0, "dstRow must be aligned");
        static_assert((dstCol % fractalSize) == 0, "dstCol must be aligned");
        PTO_ASSERT((indexRow % fractalSize) == 0, "indexRow must be aligned");
        PTO_ASSERT((indexCol % fractalSize) == 0, "indexCol must be aligned");
        TExtractToATranspose<SrcType, DstType, srcRow, srcCol, dstRow, dstCol>(dstAddr, srcAddr, indexRow, indexCol);
    }
}

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol, int32_t dstRow, int32_t dstCol>
PTO_INTERNAL void TExtractToBNonTranspose(
    __cb__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow, uint16_t indexCol)
{
    uint16_t dstGap = 0;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(SrcType);
    constexpr uint16_t dstRowNum = (dstRow * sizeof(DstType)) >> SHIFT_BLOCK_BYTE; // 分型个数
    constexpr uint16_t dstColNum = dstCol >> SHIFT_BLOCK_LEN;
    constexpr uint16_t srcColNum = srcCol >> SHIFT_BLOCK_LEN;
    constexpr uint16_t srcRowNum = (srcRow * sizeof(SrcType)) >> SHIFT_BLOCK_BYTE;
    // 计算源矩阵、目标矩阵行列中512B小分型矩阵的个数
    uint16_t blockNum = CUBE_BLOCK_SIZE >> (sizeof(SrcType) == 1    ? 0 :
                                               sizeof(SrcType) == 2 ? 1 :
                                               sizeof(SrcType) == 4 ? 2 :
                                                                      0);
    uint16_t startIdx0 =
        (indexRow * sizeof(SrcType) * srcColNum >> SHIFT_BLOCK_BYTE) + (indexCol >> SHIFT_BLOCK_LEN);
    if constexpr (dstRowNum >= dstColNum) {
        dstGap = dstColNum - 1;
        for (uint16_t i = 0; i < dstColNum; i++) {
            load_cbuf_to_cb(
                dstAddr, srcAddr, startIdx0 + i, dstRowNum, srcColNum, dstGap, 0, false, addr_cal_mode_t(0));
            dstAddr += blockNum;
        }
    } else {
        for (uint16_t i = 0; i < dstRowNum; i++) {
            load_cbuf_to_cb(dstAddr, srcAddr, startIdx0 + i * srcColNum, dstColNum, 1, 0, 0, false, addr_cal_mode_t(0));
            dstAddr += dstCol * c0Size;
        }
    }
}

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol, int32_t dstRow, int32_t dstCol>
PTO_INTERNAL void TExtractToBTranspose(
    __cb__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow, uint16_t indexCol)
{
    // b8使用Load2D
    if constexpr (sizeof(SrcType) == 1) {
        constexpr uint16_t fractNum = 2;
        // 计算源矩阵、目标矩阵行列中方块矩阵的个数
        constexpr uint16_t srcColNum = srcCol * sizeof(SrcType) >> SHIFT_BLOCK_BYTE;
        constexpr uint16_t srcRowNum = srcRow >> (SHIFT_BLOCK_LEN + fractNum - 1);
        constexpr uint16_t dstColNum = dstCol >> (SHIFT_BLOCK_LEN + fractNum - 1);
        constexpr uint16_t dstRowNum = dstRow * sizeof(DstType) >> SHIFT_BLOCK_BYTE;
        uint16_t dstGap = 0;
        uint16_t startIdx0 = (indexRow >> (SHIFT_BLOCK_LEN + fractNum - 1)) +
                             (indexCol * sizeof(SrcType) * srcRowNum >> SHIFT_BLOCK_BYTE);
        if constexpr (dstRowNum >= dstColNum) {
            dstGap = fractNum * dstColNum - 1;
            for (uint16_t i = 0; i < dstColNum; i++) {
                load_cbuf_to_cb_transpose(dstAddr, srcAddr, startIdx0 + i * srcRowNum, dstRowNum, 1, dstGap, false, 0);
                dstAddr += fractNum * CUBE_BLOCK_SIZE;
            }
        } else {
            dstGap = fractNum - 1;
            for (uint16_t i = 0; i < dstRowNum; i++) {
                load_cbuf_to_cb_transpose(dstAddr, srcAddr, startIdx0 + i, dstColNum, srcRowNum, dstGap, false, 0);
                dstAddr += dstColNum * fractNum * CUBE_BLOCK_SIZE;
            }
        }
    } else {
        // b16&b32使用Load3DV2
        constexpr int config = srcRow | (1u << 16);
        set_fmatrix_b(config);
        img2colv2_cbuf_to_cb(
            dstAddr, srcAddr, dstCol, dstRow, indexCol, indexRow, 1, 1, 1, 1, 1, 1, false, false, false, true, srcCol);
    }
}

template <typename DstTileData, typename SrcTileData, bool Transpose>
__tf__ AICORE void TExtractToB(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t indexRow, uint16_t indexCol)
{
    using SrcType = std::conditional_t<(sizeof(typename SrcTileData::DType) == 2), half, typename SrcTileData::DType>;
    using DstType = std::conditional_t<(sizeof(typename DstTileData::DType) == 2), half, typename DstTileData::DType>;
    constexpr int32_t srcRow = SrcTileData::Rows;
    constexpr int32_t srcCol = SrcTileData::Cols;
    constexpr int32_t dstRow = DstTileData::Rows;
    constexpr int32_t dstCol = DstTileData::Cols;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(SrcType);
    constexpr int32_t fractalSize = (sizeof(SrcType) == 1) ? 32 : 16;
    __cbuf__ SrcType *srcAddr = (__cbuf__ SrcType *)__cce_get_tile_ptr(src);
    __cb__ DstType *dstAddr = (__cb__ DstType *)__cce_get_tile_ptr(dst);
    if constexpr (!Transpose) {
        static_assert((srcRow % c0Size) == 0, "srcRow must be aligned to C0Size");
        static_assert((srcCol % 16) == 0, "srcCol must be aligned to 16");
        static_assert((dstRow % c0Size) == 0, "dstRow must be aligned to C0Size");
        static_assert((dstCol % 16) == 0, "dstCol must be aligned to 16");
        PTO_ASSERT((indexRow % c0Size) == 0, "indexRow must be aligned to c0Size");
        PTO_ASSERT((indexCol % 16) == 0, "indexCol must be aligned to 16");
        TExtractToBNonTranspose<SrcType, DstType, srcRow, srcCol, dstRow, dstCol>(dstAddr, srcAddr, indexRow, indexCol);
    } else {
        static_assert((srcRow % fractalSize) == 0, "srcRow must be aligned");
        static_assert((srcCol % fractalSize) == 0, "srcCol must be aligned");
        static_assert((dstRow % fractalSize) == 0, "dstRow must be aligned");
        static_assert((dstCol % fractalSize) == 0, "dstCol must be aligned");
        PTO_ASSERT((indexRow % fractalSize) == 0, "indexRow must be aligned");
        PTO_ASSERT((indexCol % fractalSize) == 0, "indexCol must be aligned");
        TExtractToBTranspose<SrcType, DstType, srcRow, srcCol, dstRow, dstCol>(dstAddr, srcAddr, indexRow, indexCol);
    }
}

/************************compact Mode*****************************/
template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol>
PTO_INTERNAL void TExtractToANonTransposeCompact(__ca__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow,
    uint16_t indexCol, uint16_t dstValidRowAlign, uint16_t dstValidColAlign)
{
    constexpr int config = srcRow | (1u << 16);
    set_fmatrix(config);
    img2colv2_cbuf_to_ca(
        dstAddr, srcAddr, dstValidColAlign, dstValidRowAlign,
        indexCol, indexRow, 1, 1, 1, 1, 1, 1, false, false, false, false, srcCol);
}

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol>
PTO_INTERNAL void TExtractToATransposeCompact(__ca__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow,
    uint16_t indexCol, uint16_t dstValidRowAlign, uint16_t dstValidColAlign)
{
    // b8   Load2D
    if constexpr (sizeof(SrcType) == 1) {
        constexpr uint16_t fractNum = 2;
        constexpr uint16_t srcColNum = srcCol >> (SHIFT_BLOCK_LEN + fractNum - 1);
        uint16_t dstColNum = dstValidColAlign >> SHIFT_BLOCK_BYTE;
        uint16_t dstRowNum = dstValidRowAlign >> (SHIFT_BLOCK_LEN + fractNum - 1);
        uint16_t dstGap = 0;
        uint16_t dstFracGap = 0;
        uint16_t startIdx0 = (indexCol >> (SHIFT_BLOCK_LEN + fractNum - 1)) +
                             (indexRow * srcColNum * sizeof(SrcType) >> SHIFT_BLOCK_BYTE);
        if (dstRowNum >= dstColNum) {
            dstGap = fractNum * dstColNum - 1;
            dstFracGap = dstColNum - 1;
            for (uint16_t i = 0; i < dstColNum; i++) {
                load_cbuf_to_ca_transpose(
                    dstAddr, srcAddr, startIdx0 + i, dstRowNum, srcColNum, dstGap, false, dstFracGap);
                dstAddr = dstAddr + CUBE_BLOCK_SIZE;
            }
        } else {
            dstFracGap = dstColNum - 1;
            for (uint16_t i = 0; i < dstRowNum; i++) {
                load_cbuf_to_ca_transpose(
                    dstAddr, srcAddr, startIdx0 + i * srcColNum, dstColNum, 1, 0, false, dstFracGap);
                dstAddr = dstAddr + dstColNum * CUBE_BLOCK_SIZE * fractNum;
            }
        }
    } else {
        // b16&b32 Load3D
        constexpr int config = srcCol | (1u << 16);
        set_fmatrix(config);
        img2colv2_cbuf_to_ca(
            dstAddr, srcAddr, dstValidRowAlign, dstValidColAlign,
            indexRow, indexCol, 1, 1, 1, 1, 1, 1, false, false, true, false, srcRow);
    }
}

template <typename DstTileData, typename SrcTileData, bool Transpose>
__tf__ AICORE void TExtractToACompact(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t indexRow, uint16_t indexCol, uint16_t dstValidRow, uint16_t dstValidCol, bool isKAligned)
{
    using SrcType = std::conditional_t<(sizeof(typename SrcTileData::DType) == 2), half, typename SrcTileData::DType>;
    using DstType = std::conditional_t<(sizeof(typename DstTileData::DType) == 2), half, typename DstTileData::DType>;
    __cbuf__ SrcType *srcAddr = (__cbuf__ SrcType *)__cce_get_tile_ptr(src);
    __ca__ DstType *dstAddr = (__ca__ DstType *)__cce_get_tile_ptr(dst);

    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(SrcType);
    constexpr int32_t fractalSize = (sizeof(SrcType) == 1) ? 32 : 16;
    if constexpr (!Transpose) {
        // srcRow/srcCol/dstRow/dstCol check
        static_assert((SrcTileData::Rows % 16) == 0, "srcRow must be aligned to 16");
        static_assert((SrcTileData::Cols % c0Size) == 0, "srcCol must be aligned to C0Size");
        PTO_ASSERT((indexRow % 16) == 0, "indexRow must be aligned to 16");
        PTO_ASSERT((indexCol % c0Size) == 0, "indexCol must be aligned to C0Size");
        uint16_t dstValidRowAlign = CeilDivision(dstValidRow, FRACTAL_NZ_ROW) * FRACTAL_NZ_ROW;
        uint16_t dstValidColAlign = CeilDivision(dstValidCol, c0Size) * c0Size;
        if (isKAligned && (std::is_same<DstType, float>::value)) {
            dstValidColAlign = CeilDivision(dstValidCol, FRACTAL_NZ_ROW) * FRACTAL_NZ_ROW;
        }
        TExtractToANonTransposeCompact<SrcType, DstType, SrcTileData::Rows, SrcTileData::Cols>(
            dstAddr, srcAddr, indexRow, indexCol, dstValidRowAlign, dstValidColAlign);
    } else {
        // L1->L0A:load_cbuf_to_ca_transpose
        static_assert((SrcTileData::Rows % fractalSize) == 0, "srcRow must be aligned");
        static_assert((SrcTileData::Cols % fractalSize) == 0, "srcCol must be aligned");
        PTO_ASSERT((indexRow % fractalSize) == 0, "indexRow must be aligned");
        PTO_ASSERT((indexCol % fractalSize) == 0, "indexCol must be aligned");
        uint16_t dstValidRowAlign = CeilDivision(dstValidRow, fractalSize) * fractalSize;
        uint16_t dstValidColAlign = CeilDivision(dstValidCol, fractalSize) * fractalSize;
        TExtractToATransposeCompact<SrcType, DstType, SrcTileData::Rows, SrcTileData::Cols>(
            dstAddr, srcAddr, indexRow, indexCol, dstValidRowAlign, dstValidColAlign);
    }
}

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol>
PTO_INTERNAL void TExtractToBNonTransposeCompact(__cb__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow,
    uint16_t indexCol, uint16_t dstValidRowAlign, uint16_t dstValidColAlign)
{
    uint16_t dstGap = 0;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(SrcType);
    uint16_t dstRowNum = (dstValidRowAlign * sizeof(DstType)) >> SHIFT_BLOCK_BYTE;
    uint16_t dstColNum = dstValidColAlign >> SHIFT_BLOCK_LEN;
    constexpr uint16_t srcColNum = srcCol >> SHIFT_BLOCK_LEN;
    constexpr uint16_t srcRowNum = (srcRow * sizeof(SrcType)) >> SHIFT_BLOCK_BYTE;
    uint16_t blockNum = CUBE_BLOCK_SIZE >> (sizeof(SrcType) == 1      ? 0
                                               : sizeof(SrcType) == 2 ? 1
                                               : sizeof(SrcType) == 4 ? 2
                                                                      : 0);
    uint16_t startIdx0 =
        (indexRow * sizeof(SrcType) * srcColNum >> SHIFT_BLOCK_BYTE) + (indexCol >> SHIFT_BLOCK_LEN);
    if (dstRowNum >= dstColNum) {
        dstGap = dstColNum - 1;
        for (uint16_t i = 0; i < dstColNum; i++) {
            load_cbuf_to_cb(
                dstAddr, srcAddr, startIdx0 + i, dstRowNum, srcColNum, dstGap, 0, false, addr_cal_mode_t(0));
            dstAddr += blockNum;
        }
    } else {
        for (uint16_t i = 0; i < dstRowNum; i++) {
            load_cbuf_to_cb(dstAddr, srcAddr, startIdx0 + i * srcColNum, dstColNum, 1, 0, 0, false, addr_cal_mode_t(0));
            dstAddr += dstValidColAlign * c0Size;
        }
    }
}

template <typename DstType, typename SrcType, int32_t srcRow, int32_t srcCol>
PTO_INTERNAL void TExtractToBTransposeCompact(__cb__ DstType *dstAddr, __cbuf__ SrcType *srcAddr, uint16_t indexRow,
    uint16_t indexCol, uint16_t dstValidRowAlign, uint16_t dstValidColAlign, uint16_t dstValidCol)
{
    // b8 Load2D
    if constexpr (sizeof(SrcType) == 1) {
        constexpr uint16_t fractNum = 2;
        constexpr uint16_t srcColNum = srcCol * sizeof(SrcType) >> SHIFT_BLOCK_BYTE;
        constexpr uint16_t srcRowNum = srcRow >> (SHIFT_BLOCK_LEN + fractNum - 1);
        uint16_t dstColNum = dstValidColAlign >> (SHIFT_BLOCK_LEN + fractNum - 1);
        uint16_t dstRowNum = dstValidRowAlign * sizeof(DstType) >> SHIFT_BLOCK_BYTE;
        uint16_t dstGap = fractNum - 1;
        uint16_t startIdx0 = (indexRow >> (SHIFT_BLOCK_LEN + fractNum - 1)) +
                             (indexCol * sizeof(SrcType) * srcRowNum >> SHIFT_BLOCK_BYTE);
        uint16_t dstAddrStride = CeilDivision(dstValidCol, FRACTAL_NZ_ROW) * CUBE_BLOCK_SIZE;
        for (uint16_t i = 0; i < dstRowNum; i++) {
            load_cbuf_to_cb_transpose(dstAddr, srcAddr, startIdx0 + i, dstColNum, srcRowNum, dstGap, false, 0);
            dstAddr += dstAddrStride;
        }
    } else {
        // b16&b32 Load3DV2
        constexpr int config = srcRow | (1u << 16);
        set_fmatrix_b(config);
        img2colv2_cbuf_to_cb(
            dstAddr, srcAddr, dstValidColAlign, dstValidRowAlign,
            indexCol, indexRow, 1, 1, 1, 1, 1, 1, false, false, false, true, srcCol);
    }
}

template <typename DstTileData, typename SrcTileData, bool Transpose>
__tf__ AICORE void TExtractToBCompact(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t indexRow, uint16_t indexCol, uint16_t dstValidRow, uint16_t dstValidCol)
{
    using SrcType = std::conditional_t<(sizeof(typename SrcTileData::DType) == 2), half, typename SrcTileData::DType>;
    using DstType = std::conditional_t<(sizeof(typename DstTileData::DType) == 2), half, typename DstTileData::DType>;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(SrcType);
    constexpr int32_t fractalSize = (sizeof(SrcType) == 1) ? 32 : 16;
    __cbuf__ SrcType *srcAddr = (__cbuf__ SrcType *)__cce_get_tile_ptr(src);
    __cb__ DstType *dstAddr = (__cb__ DstType *)__cce_get_tile_ptr(dst);
    static_assert((DstTileData::Rows % c0Size) == 0, "dstRow must be aligned to C0Size");
    static_assert((DstTileData::Cols % 16) == 0, "dstCol must be aligned to 16");
    if constexpr (!Transpose) {
        static_assert((SrcTileData::Rows % c0Size) == 0, "srcRow must be aligned to C0Size");
        static_assert((SrcTileData::Cols % 16) == 0, "srcCol must be aligned to 16");
        PTO_ASSERT((indexRow % c0Size) == 0, "indexRow must be aligned to c0Size");
        PTO_ASSERT((indexCol % 16) == 0, "indexCol must be aligned to 16");
        uint16_t dstValidRowAlign = CeilDivision(dstValidRow, c0Size) * c0Size;
        uint16_t dstValidColAlign = CeilDivision(dstValidCol, FRACTAL_NZ_ROW) * FRACTAL_NZ_ROW;
        TExtractToBNonTransposeCompact<SrcType, DstType, SrcTileData::Rows, SrcTileData::Cols>(
            dstAddr, srcAddr, indexRow, indexCol, dstValidRowAlign, dstValidColAlign);
    } else {
        static_assert((SrcTileData::Rows % fractalSize) == 0, "srcRow must be aligned");
        static_assert((SrcTileData::Cols % fractalSize) == 0, "srcCol must be aligned");
        PTO_ASSERT((indexRow % fractalSize) == 0, "indexRow must be aligned");
        PTO_ASSERT((indexCol % fractalSize) == 0, "indexCol must be aligned");
        uint16_t dstValidRowAlign = CeilDivision(dstValidRow, fractalSize) * fractalSize;
        uint16_t dstValidColAlign = CeilDivision(dstValidCol, fractalSize) * fractalSize;
        TExtractToBTransposeCompact<SrcType, DstType, SrcTileData::Rows, SrcTileData::Cols>(
            dstAddr, srcAddr, indexRow, indexCol, dstValidRowAlign, dstValidColAlign, dstValidCol);
    }
}

template <typename DstTileData, typename SrcTileData, QuantMode_t QuantPre, ReluPreMode reluMode>
__tf__ AICORE void TExtractAccToMat(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint16_t validRow, uint16_t validCol, uint16_t indexRow, uint16_t indexCol)
{
    using SrcType = typename SrcTileData::DType;
    using DstType = typename DstTileData::DType;
    constexpr int32_t c0Size = BLOCK_BYTE_SIZE / sizeof(DstType);
    constexpr int32_t accC0Size = BLOCK_BYTE_SIZE / sizeof(half);
    uint32_t srcOffset = SrcTileData::Rows * accC0Size * (indexCol / accC0Size) + (indexRow * accC0Size + (indexCol % accC0Size));
    __cc__ SrcType *srcAddr = (__cc__ SrcType *)__cce_get_tile_ptr(src) + srcOffset;
    __cbuf__ DstType *dstAddr = (__cbuf__ DstType *)__cce_get_tile_ptr(dst);

    constexpr uint32_t dstStride_dst_D = DstTileData::Rows;
    constexpr uint16_t srcStride = SrcTileData::Rows;
    uint16_t nSize = CeilDivision(validCol, c0Size) * c0Size;
    copy_matrix_cc_to_cbuf(
        dstAddr, srcAddr, 0, nSize, validRow, dstStride_dst_D, 
        srcStride, 0, QuantPre, reluMode, false, false);
}

template <typename DstTileData, typename SrcTileData, typename DstType, typename SrcType>
PTO_INTERNAL void CheckTExtract()
{
    static_assert((SrcTileData::Loc == TileType::Acc) || std::is_same<DstType, SrcType>::value,
        "TExtract: Destination and Source tile data types must be the same.");
    static_assert(std::is_same<DstType, int8_t>::value ||
                      std::is_same<DstType, half>::value ||
                      std::is_same<DstType, bfloat16_t>::value ||
                      std::is_same<DstType, float>::value,
        "TExtract: Invalid data type.");
    static_assert((SrcTileData::SFractal == SLayout::ColMajor && SrcTileData::isRowMajor) ||
                      (SrcTileData::SFractal == SLayout::RowMajor && !SrcTileData::isRowMajor),
        "TExtract: SrcTile Invalid Fractal.");
}

template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TEXTRACT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    CheckTExtract<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType>();
    PTO_ASSERT(indexRow + DstTileData::Rows <= SrcTileData::Rows,
        "The sum of indexRow and dstRow should be less than srcRow!");
    PTO_ASSERT(indexCol + DstTileData::Cols <= SrcTileData::Cols,
        "The sum of indexCol and dstCol should be less than srcCol!");
    if constexpr (DstTileData::Loc == TileType::Left) {
        static_assert(DstTileData::SFractal == SLayout::RowMajor && DstTileData::isRowMajor,
            "TExtract: LeftTile Invalid Fractal.");
        if constexpr (DstTileData::SFractal == SrcTileData::SFractal) {
            if constexpr (DstTileData::Compact == CompactMode::Normal) {
                TExtractToACompact<DstTileData, SrcTileData, false>(
                    dst.data(), src.data(), indexRow, indexCol, dst.GetValidRow(), dst.GetValidCol(), dst.GetKAligned());
            } else {
                TExtractToA<DstTileData, SrcTileData, false>(dst.data(), src.data(), indexRow, indexCol);
            }
        } else {
            if constexpr (DstTileData::Compact == CompactMode::Normal) {
                TExtractToACompact<DstTileData, SrcTileData, true>(
                    dst.data(), src.data(), indexRow, indexCol, dst.GetValidRow(), dst.GetValidCol(), dst.GetKAligned());
            } else {
                TExtractToA<DstTileData, SrcTileData, true>(dst.data(), src.data(), indexRow, indexCol);
            }
        }
    } else if constexpr (DstTileData::Loc == TileType::Right) {
        static_assert(DstTileData::SFractal == SLayout::ColMajor && DstTileData::isRowMajor,
            "TExtract: RightTile Invalid Fractal.");
        if constexpr (DstTileData::SFractal == SrcTileData::SFractal) {
            if constexpr (DstTileData::Compact == CompactMode::Normal) {
                TExtractToBCompact<DstTileData, SrcTileData, false>(
                    dst.data(), src.data(), indexRow, indexCol, dst.GetValidRow(), dst.GetValidCol());
            } else {
                TExtractToB<DstTileData, SrcTileData, false>(dst.data(), src.data(), indexRow, indexCol);
            }
        } else {
            if constexpr (DstTileData::Compact == CompactMode::Normal) {
                TExtractToBCompact<DstTileData, SrcTileData, true>(
                    dst.data(), src.data(), indexRow, indexCol, dst.GetValidRow(), dst.GetValidCol());
            } else {
                TExtractToB<DstTileData, SrcTileData, true>(dst.data(), src.data(), indexRow, indexCol);
            }
        }
    }  else if constexpr (SrcTileData::Loc == TileType::Acc && DstTileData::Loc == TileType::Mat) {
        CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
        constexpr QuantMode_t quantPre =
            GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>(); 
        TExtractAccToMat<DstTileData, SrcTileData, quantPre, ReluPreMode::NoRelu>(dst.data(), src.data(),
            dst.GetValidRow(), dst.GetValidCol(), indexRow, indexCol);
    }
}

// relu
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode>
PTO_INTERNAL void TEXTRACT_IMPL(DstTileData &dst, SrcTileData &src, uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    PTO_ASSERT(indexRow + DstTileData::Rows <= SrcTileData::Rows,
        "The sum of indexRow and dstRow should be less than srcRow!");
    PTO_ASSERT(indexCol + DstTileData::Cols <= SrcTileData::Cols,
        "The sum of indexCol and dstCol should be less than srcCol!");
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, true>();
    constexpr QuantMode_t quantPre = GetCastPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    TExtractAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        dst.GetValidRow(), dst.GetValidCol(), indexRow, indexCol);
}

// scalar quant
template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TEXTRACT_IMPL(DstTileData &dst, SrcTileData &src, uint64_t preQuantScalar,
    uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    PTO_ASSERT(indexRow + DstTileData::Rows <= SrcTileData::Rows,
        "The sum of indexRow and dstRow should be less than srcRow!");
    PTO_ASSERT(indexCol + DstTileData::Cols <= SrcTileData::Cols,
        "The sum of indexCol and dstCol should be less than srcCol!");
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, false>();
    constexpr QuantMode_t quantPre = GetScalarPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    set_quant_pre(preQuantScalar);
    TExtractAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        dst.GetValidRow(), dst.GetValidCol(), indexRow, indexCol);
}

// vector quant
template <typename FpTileData>
__tf__ PTO_INTERNAL void SetFPC(typename FpTileData::TileDType __in__ fp, uint16_t indexCol)
{
    using FpType = typename FpTileData::DType;
    __fbuf__ FpType *dstAddrFp = (__fbuf__ FpType *)__cce_get_tile_ptr(fp) + indexCol;
    uint64_t deqTensorAddr = ((uint64_t)dstAddrFp >> static_cast<uint64_t>(7))
                             << 8;  // fpc[15:8] means Quant_PRE_ADDR, uint of 128(2^7)bytes
    set_fpc(deqTensorAddr);
}

template <typename DstTileData, typename SrcTileData, typename FpTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TEXTRACT_IMPL(DstTileData &dst, SrcTileData &src, FpTileData &fp,
     uint16_t indexRow = 0, uint16_t indexCol = 0)
{
    PTO_ASSERT(indexRow + DstTileData::Rows <= SrcTileData::Rows,
        "The sum of indexRow and dstRow should be less than srcRow!");
    PTO_ASSERT(indexCol + DstTileData::Cols <= SrcTileData::Cols,
        "The sum of indexCol and dstCol should be less than srcCol!");
    CheckTMovAccToMat<DstTileData, SrcTileData, typename DstTileData::DType, typename SrcTileData::DType, false>();
    static_assert(FpTileData::Loc == TileType::Scaling, "Fp only support Scaling.");
    constexpr QuantMode_t quantPre = GetVectorPreQuantMode<typename SrcTileData::DType, typename DstTileData::DType>();
    SetFPC<FpTileData>(fp.data(), indexCol);
    TExtractAccToMat<DstTileData, SrcTileData, quantPre, reluMode>(dst.data(), src.data(),
        dst.GetValidRow(), dst.GetValidCol(), indexRow, indexCol);
}
}  // namespace pto
#endif  // TEXTRACT_HPP
