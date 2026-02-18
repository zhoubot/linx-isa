/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSTORE_HPP
#define TSTORE_HPP
#include "common.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#endif

namespace pto {
template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreUb2gmInstr(typename GlobalData::DType *dst, __ubuf__ typename TileData::DType *src,
    uint16_t nBurst, uint32_t lenBurst, uint32_t gmGap, uint32_t ubGap)
{
    if constexpr (sizeof(typename TileData::DType) == 1) {
        copy_ubuf_to_gm_align_b8(dst, src, 0, nBurst, lenBurst, 0, 0, ubGap, gmGap);
    } else if constexpr (sizeof(typename TileData::DType) == 2) {
        copy_ubuf_to_gm_align_b16(dst, src, 0, nBurst, lenBurst, 0, 0, ubGap, gmGap);
    } else if constexpr (sizeof(typename TileData::DType) == 4 || sizeof(typename TileData::DType) == 8) {
        copy_ubuf_to_gm_align_b32(dst, src, 0, nBurst, lenBurst, 0, 0, ubGap, gmGap);
    }
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreUb2gmNd2nd(typename GlobalData::DType *dstAddr, __ubuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(validCol == gShape4, "The validCol of TileData must be equal to the 5th dim(Shape4) of ND shape!");
    PTO_ASSERT(validRow == gShape0 * gShape1 * gShape2 * gShape3,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape3) of ND shape!");
    PTO_ASSERT(gShape3 < 4096, "The gshape3 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape3;
    uint32_t lenBurst = validCol * sizeof(typename TileData::DType);
    uint32_t gmGap = (gStride3 - gShape4) * sizeof(typename TileData::DType);
    uint32_t ubGap = ((TileData::Cols - validCol) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    typename GlobalData::DType *dstGlobalAddr = dstAddr;
    __ubuf__ typename TileData::DType *srcTileAddr = srcAddr;

    int64_t srcStride2 = gShape3 * TileData::Cols;
    int64_t srcStride1 = gShape2 * srcStride2;
    int64_t srcStride0 = gShape1 * srcStride1;
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * gStride0;
        int64_t srcAddr0 = i * srcStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t dstAddr1 = j * gStride1;
            int64_t srcAddr1 = j * srcStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                dstGlobalAddr = dstAddr + dstAddr0 + dstAddr1 + k * gStride2;
                srcTileAddr = srcAddr + srcAddr0 + srcAddr1 + k * srcStride2;
                TStoreUb2gmInstr<GlobalData, TileData>(dstGlobalAddr, srcTileAddr, nBurst, lenBurst, gmGap, ubGap);
            }
        }
    }
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreUb2gmDn2dn(typename GlobalData::DType *dstAddr, __ubuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(validRow == gShape3, "The validCol of TileData must be equal to the 4th dim(Shape3) of DN shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape2 * gShape4,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape4) of DN shape!");
    PTO_ASSERT(gShape4 < 4096, "The gshape4 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape4;
    uint32_t lenBurst = validRow * sizeof(typename TileData::DType);
    uint32_t gmGap = (gStride4 - gShape3) * sizeof(typename TileData::DType);
    uint32_t ubGap = ((TileData::Rows - gShape3) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    typename GlobalData::DType *dstGlobalAddr = dstAddr;
    __ubuf__ typename TileData::DType *srcTileAddr = srcAddr;
    int64_t srcStride2 = TileData::Rows * gShape4;
    int64_t srcStride1 = gShape2 * srcStride2;
    int64_t srcStride0 = gShape1 * srcStride1;

    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t srcAddr0 = i * srcStride0;
        int64_t dstAddr0 = i * gStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t srcAddr1 = j * srcStride1;
            int64_t dstAddr1 = j * gStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                dstGlobalAddr = dstAddr + dstAddr0 + dstAddr1 + k * gStride2;
                srcTileAddr = srcAddr + srcAddr0 + srcAddr1 + k * srcStride2;
                TStoreUb2gmInstr<GlobalData, TileData>(dstGlobalAddr, srcTileAddr, nBurst, lenBurst, gmGap, ubGap);
            }
        }
    }
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreUb2gmNz2nz(typename GlobalData::DType *dstAddr, __ubuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
     static_assert(GlobalData::staticShape[3] == FRACTAL_NZ_ROW &&
 	                       GlobalData::staticShape[4] == BLOCK_BYTE_SIZE / sizeof(typename TileData::DType),
 	         "When TileData is NZ format, the last 2 dim must be static and satisfy [16, 32 / sizeof(DataType)]");
    PTO_ASSERT(validRow == gShape2 * gShape3, "The validRow of TileData must be equal to Shape2 * Shape3 of NZ shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape4,
        "The validCol of TileData must be equal to Shape0 * Shape1 * Shape4 of NZ shape!");
    PTO_ASSERT(gShape1 < 4096, "The gshape1 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape1;
    uint32_t lenBurst = validRow * C0_SIZE_BYTE;
    uint32_t gmGap = (gStride1 - gShape2 * gShape3 * gShape4) * sizeof(typename TileData::DType);
    uint32_t ubGap = TileData::Rows - validRow;

    typename GlobalData::DType *dstGlobalAddr = dstAddr;
    __ubuf__ typename TileData::DType *srcTileAddr = srcAddr;

    int64_t tileStride = TileData::Rows * gShape1 * gShape4;
    for (uint32_t i = 0; i < gShape0; i++) {
        dstGlobalAddr = dstAddr + i * gStride0;
        srcTileAddr = srcAddr + i * tileStride;
        TStoreUb2gmInstr<GlobalData, TileData>(dstGlobalAddr, srcTileAddr, nBurst, lenBurst, gmGap, ubGap);
    }
}

template <typename GlobalData, typename TileData, AtomicType currentAtomicType = AtomicType::AtomicNone>
__tf__ PTO_INTERNAL void TStore(typename GlobalData::DType __out__ *dst, typename TileData::TileDType __in__ src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    __ubuf__ typename TileData::DType *srcAddr = (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(src);
    typename GlobalData::DType *dstAddr = dst;

    if constexpr (TileData::isRowMajor & (TileData::SFractal == SLayout::NoneBox)) {
        TStoreUb2gmNd2nd<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (!TileData::isRowMajor & (TileData::SFractal == SLayout::NoneBox)) {
        TStoreUb2gmDn2dn<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (!TileData::isRowMajor & (TileData::SFractal == SLayout::RowMajor)) {
        TStoreUb2gmNz2nz<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreMat2GmInstr(typename GlobalData::DType *dstAddr, __cbuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gStride0, int gStride1, int gStride2, int64_t srcStride2,
    uint16_t nBurst, uint16_t lenBurst, uint16_t dstStride, uint16_t srcStride)
{
    typename GlobalData::DType *dstAddrP = dstAddr;
    __cbuf__ typename TileData::DType *srcAddrP = srcAddr;

    int64_t srcStride1 = gShape2 * srcStride2;
    int64_t srcStride0 = gShape1 * srcStride1;
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * gStride0;
        int64_t srcAddr0 = i * srcStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t dstAddr1 = j * gStride1;
            int64_t srcAddr1 = j * srcStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                srcAddrP = srcAddr + srcAddr0 + srcAddr1 + k * srcStride2;
                dstAddrP = dstAddr + dstAddr0 + dstAddr1 + k * gStride2;
                copy_cbuf_to_gm(dstAddrP, srcAddrP, (uint8_t)0, nBurst, lenBurst, srcStride, dstStride);
            }
        }
    }
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreMat2GmNd2Nd(typename GlobalData::DType *dstAddr, __cbuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(gShape4 * sizeof(typename TileData::DType) % BLOCK_BYTE_SIZE == 0,
        "The 5th dim of ND shape must be 32 bytes aligned!");
    PTO_ASSERT(validCol == gShape4, "The validCol of TileData must be equal to the 5th dim(Shape4) of ND shape!");
    PTO_ASSERT(validRow == gShape0 * gShape1 * gShape2 * gShape3,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape3) of ND shape!");
    PTO_ASSERT(gShape3 < 4096, "The gshape3 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape3;
    uint16_t lenBurst = (validCol * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t dstStride = ((gStride3 - gShape4) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t srcStride = ((TileData::Cols - validCol) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;

    int64_t srcStride2 = gShape3 * TileData::Cols;
    TStoreMat2GmInstr<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gStride0, gStride1, gStride2,
        srcStride2, nBurst, lenBurst, dstStride, srcStride);
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreMat2GmDn2Dn(typename GlobalData::DType *dstAddr, __cbuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(gShape3 * sizeof(typename TileData::DType) % BLOCK_BYTE_SIZE == 0,
        "The 4th dim of DN shape must be 32 bytes aligned!");
    PTO_ASSERT(validRow == gShape3, "The validCol of TileData must be equal to the 4th dim(Shape3) of DN shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape2 * gShape4,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape4) of DN shape!");
    PTO_ASSERT(gShape4 < 4096, "The gshape4 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape4;
    uint16_t lenBurst = (validRow * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t dstStride = ((gStride4 - gShape3) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t srcStride = ((TileData::Rows - gShape3) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;

    int64_t srcStride2 = gShape4 * TileData::Rows;
    TStoreMat2GmInstr<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gStride0, gStride1, gStride2,
        srcStride2, nBurst, lenBurst, dstStride, srcStride);
}

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TStoreMat2GmNz2Nz(typename GlobalData::DType *dstAddr, __cbuf__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    static_assert(GlobalData::staticShape[3] == FRACTAL_NZ_ROW &&
                      GlobalData::staticShape[4] == BLOCK_BYTE_SIZE / sizeof(typename TileData::DType),
        "When TileData is NZ format, the last 2 dim must be static and satisfy [16, 32 / sizeof(DataType)]");
    PTO_ASSERT(gShape1 < 4096, "The gshape1 (which equals nBurst) must be less than 4096 for A2/A3");
    PTO_ASSERT(validRow == gShape2 * gShape3, "The validRow of TileData must be equal to Shape2 * Shape3 of NZ shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape4,
        "The validCol of TileData must be equal to Shape0 * Shape1 * Shape4 of NZ shape!");
    uint16_t nBurst = gShape1;
    uint32_t lenBurst = validRow;
    uint32_t dstStride =
        ((gStride1 - gShape2 * gShape3 * gShape4) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint32_t srcStride = TileData::Rows - validRow;

    typename GlobalData::DType *dstAddrP = dstAddr;
    __cbuf__ typename TileData::DType *srcAddrP = srcAddr;

    int64_t tileStride = TileData::Rows * gShape1 * gShape4;

    for (uint32_t i = 0; i < gShape0; i++) {
        dstAddrP = dstAddr + i * gStride0;
        srcAddrP = srcAddr + i * tileStride;
        copy_cbuf_to_gm(dstAddrP, srcAddrP, 0, nBurst, lenBurst, srcStride, dstStride);
    }
}

template <typename GlobalData, typename TileData, AtomicType atomicType = AtomicType::AtomicNone>
__tf__ AICORE void TStoreMat(typename GlobalData::DType __out__ *dst, typename TileData::TileDType __in__ src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    __cbuf__ typename TileData::DType *srcAddr = (__cbuf__ typename TileData::DType *)__cce_get_tile_ptr(src);
    typename GlobalData::DType *dstAddr = dst;

    if constexpr (TileData::isRowMajor & (TileData::SFractal == SLayout::NoneBox)) {
        TStoreMat2GmNd2Nd<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (!TileData::isRowMajor & (TileData::SFractal == SLayout::NoneBox)) {
        TStoreMat2GmDn2Dn<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (!TileData::isRowMajor & (TileData::SFractal == SLayout::RowMajor)) {
        TStoreMat2GmNz2Nz<GlobalData, TileData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename T>
PTO_INTERNAL void SetAtomicAdd()
{
    static_assert((std::is_same<T, __gm__ half>::value) || (std::is_same<T, __gm__ float>::value) ||
                      (std::is_same<T, __gm__ int16_t>::value) || (std::is_same<T, __gm__ int32_t>::value) ||
                      (std::is_same<T, __gm__ int8_t>::value) || (std::is_same<T, __gm__ bfloat16_t>::value),
        "Dst and src must be half / float / int16_t / int32_t / int8_t / bfloat16_t.");
    if constexpr (std::is_same<T, __gm__ float>::value) {
        set_atomic_f32();
    } else if constexpr (std::is_same<T, __gm__ half>::value) {
        set_atomic_f16();
    } else if constexpr (std::is_same<T, __gm__ int16_t>::value) {
        set_atomic_s16();
    } else if constexpr (std::is_same<T, __gm__ int32_t>::value) {
        set_atomic_s32();
    } else if constexpr (std::is_same<T, __gm__ int8_t>::value) {
        set_atomic_s8();
    } else if constexpr (std::is_same<T, __gm__ bfloat16_t>::value) {
        set_atomic_bf16();
    }
    set_atomic_add();
}

PTO_INTERNAL void SetAtomicNone()
{
    set_atomic_none();
}

template <typename GlobalData, typename TileData, QuantMode_t quantizationMode = QuantMode_t::NoQuant,
    ReluPreMode reluPreMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TStoreAccNz2nd(typename GlobalData::DType *dstAddr, __cc__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(gShape0 == 1 && gShape1 == 1 && gShape2 == 1, "NZ2ND only supports 2D-to-2D conversions.");
    PTO_ASSERT(validCol == gShape4, "The validCol of TileData must be equal to the 5th dim(Shape4) of ND shape!");
    PTO_ASSERT(validRow == gShape3, "The validRow of TileData must be equal to Shape3 of ND shape!");
    PTO_ASSERT(validRow >= 1 && validRow <= 8192, "When GlobalData is ND format, the range of validRow is [1, 8192].");
    uint16_t mSize = validRow;
    uint16_t nSize = validCol;

    uint16_t srcStride = TileData::Rows;
    uint32_t dstD = gStride3;

    uint16_t ndNum = validCol / gShape4;
    constexpr uint8_t c0 = 16;
    uint16_t srcNdStride = TileData::Rows * c0 * gShape4;
    if constexpr (TileData::Compact == CompactMode::Normal) {
        srcStride = (validRow + FRACTAL_NZ_ROW - 1) / FRACTAL_NZ_ROW * FRACTAL_NZ_ROW;
        srcNdStride = srcStride * gShape4 * c0;
    }

    uint16_t dstNdStride = gStride2;

    constexpr uint8_t nz2ndEn = 1;
    constexpr uint8_t unitFlagCtrl = 0;

    uint64_t xmReg =
        ((nSize & 0xfff) << 4) |                          // Xm[15:4] the n-direction size of the matrix
        (static_cast<uint64_t>(mSize & 0xffff) << 16) |   // Xm[31:16] the m-direction size of the matrix
        (static_cast<uint64_t>(dstD & 0xffffffff) << 32); // Xm[63:32] destination stride between the start addr
    uint64_t xtReg = srcStride |                          // Xt[15:0] the source stride between the start addr
                     (static_cast<uint64_t>(unitFlagCtrl & 0x3) << 32) |      // Xt[33:32] unit flag control bit
                     (static_cast<uint64_t>(quantizationMode & 0x1f) << 34) | // Xt[38:34] pre-stage quantization mode
                     ((static_cast<uint64_t>(reluPreMode) & 0x7) << 39) |     //  Xt[41:39] relu pre mode
                     (static_cast<uint64_t>(nz2ndEn & 0x1) << 43);            // Xt[43] nz2nd control bit
    uint64_t ndParaSPR = 0;
    ndParaSPR = ndNum |                                               // ND_PARA[15:0] the number of source nd
                (static_cast<uint64_t>(srcNdStride & 0xffff) << 16) | // ND_PARA[31:16] the stride of source nd
                (static_cast<uint64_t>(dstNdStride & 0xffff) << 32);  // ND_PARA[47:32] the stride of destination nd
    set_nd_para(ndParaSPR);
    copy_matrix_cc_to_gm(dstAddr, srcAddr, xmReg, xtReg);
}

template <typename GlobalData, typename TileData, QuantMode_t quantizationMode = QuantMode_t::NoQuant,
    ReluPreMode reluPreMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TStoreAccNz2nz(typename GlobalData::DType *dstAddr, __cc__ typename TileData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(validRow == gShape2 * gShape3, "The validRow of TileData must be equal to Shape2 * Shape3 of NZ shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape4,
        "The validCol of TileData must be equal to Shape0 * Shape1 * Shape4 of NZ shape!");
    PTO_ASSERT(validRow >= 1 && validRow <= 65535 && validCol % 16 == 0,
        "When GlobalData is NZ format, the range of validRow is [1, 65535] and validCol must be an integer multiple of "
        "16.");

    static_assert(GlobalData::staticShape[3] == FRACTAL_NZ_ROW,
        "When GlobalData is NZ format, the second-to-last dimension shall be 16.");
    static_assert(
        (std::is_same_v<typename GlobalData::DType, __gm__ float> &&
            (GlobalData::staticShape[4] == 8 || GlobalData::staticShape[4] == 16)) ||
            (std::is_same_v<typename GlobalData::DType, __gm__ int32_t> && GlobalData::staticShape[4] == 16) ||
            (GlobalData::staticShape[4] == BLOCK_BYTE_SIZE / sizeof(typename GlobalData::DType)),
        "When GlobalData is in NZ format: if DstType is float, the last dimension must be either 8 or 16, "
        "and the dimension value is 8 if and only if Channel Split is enabled; if DstType is int32_t, the "
        "last dimension must be exactly 16. In addition, the last dimension must be static and satisfy 32 / "
        "sizeof(DstType).");

    uint16_t mSize = validRow;
    uint16_t nSize = validCol;

    uint32_t c0Size = sizeof(typename GlobalData::DType) * gShape4;
    uint16_t srcStride = TileData::Rows;
    if constexpr (CompactMode::Normal == TileData::Compact) {
        srcStride = (FRACTAL_NZ_ROW + validRow - 1) / FRACTAL_NZ_ROW * FRACTAL_NZ_ROW;
    }
    uint32_t dstStride = gShape2 * gShape3 * c0Size >> SHIFT_BLOCK_BYTE;

    constexpr uint8_t unitFlagCtrl = 0;
    uint8_t channelSplitEn = 0;
    if (std::is_same_v<typename TileData::DType, float> && std::is_same_v<typename GlobalData::DType, __gm__ float>) {
        if (gShape4 == 8) {
            channelSplitEn = 1;
        }
    }

    uint64_t xmReg =

        (static_cast<uint64_t>(nSize & 0xfff) << 4) |          // Xm[15:4] nSize
        (static_cast<uint64_t>(mSize & 0xffff) << 16) |        // Xm[31:16] mSize
        (static_cast<uint64_t>(dstStride & 0xffffffff) << 32); // Xm[63:32] destination stride between the start addr

    uint64_t xtReg = srcStride | // Xt[15:0] the source stride between the start addr
                     (static_cast<uint64_t>(unitFlagCtrl & 0x3) << 32) |      // Xt[33:32] unit flag control bit
                     (static_cast<uint64_t>(quantizationMode & 0x1f) << 34) | // Xt[38:34] pre-stage quantization mode
                     ((static_cast<uint64_t>(reluPreMode) & 0x7) << 39) |     //  Xt[41:39] relu pre mode
                     (static_cast<uint64_t>(channelSplitEn & 0x1) << 42);     // Xt[42] channel split control bit

    copy_matrix_cc_to_gm(dstAddr, srcAddr, xmReg, xtReg);
}

template <typename GlobalData, typename TileData, QuantMode_t quantizationMode = QuantMode_t::NoQuant,
    ReluPreMode reluPreMode = ReluPreMode::NoRelu>
__tf__ AICORE void TStoreAcc(typename GlobalData::DType __out__ *dst, typename TileData::TileDType __in__ src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    __cc__ typename TileData::DType *srcAddr = (__cc__ typename TileData::DType *)__cce_get_tile_ptr(src);
    typename GlobalData::DType *dstAddr = dst;

    if constexpr (GlobalData::layout == pto::Layout::ND) {
        TStoreAccNz2nd<GlobalData, TileData, quantizationMode, reluPreMode>(dstAddr, srcAddr, gShape0, gShape1, gShape2,
            gShape3, gShape4, gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::NZ) {
        TStoreAccNz2nz<GlobalData, TileData, quantizationMode, reluPreMode>(dstAddr, srcAddr, gShape0, gShape1, gShape2,
            gShape3, gShape4, gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename GlobalData, typename TileData, typename FpTileData,
    QuantMode_t quantizationMode = QuantMode_t::NoQuant, ReluPreMode reluPreMode = ReluPreMode::NoRelu>
__tf__ AICORE void TStoreAccFp(typename GlobalData::DType __out__ *dst, typename TileData::TileDType __in__ src,
    typename FpTileData::TileDType __in__ fp, int gShape0, int gShape1, int gShape2, int gShape3, int gShape4,
    int gStride0, int gStride1, int gStride2, int gStride3, int gStride4, int validRow, int validCol)
{
    __fbuf__ typename FpTileData::DType *fpDstAddr = (__fbuf__ typename FpTileData::DType *)__cce_get_tile_ptr(fp);
    uint64_t deqTensorAddr = ((uint64_t)fpDstAddr >> static_cast<uint64_t>(7)) << 8;
    set_fpc(deqTensorAddr);
    pipe_barrier(PIPE_FIX);
    if constexpr (GlobalData::layout == pto::Layout::ND) {
        TStoreAccNz2nd<GlobalData, TileData, quantizationMode, reluPreMode>(dst, src, gShape0, gShape1, gShape2,
            gShape3, gShape4, gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::NZ) {
        TStoreAccNz2nz<GlobalData, TileData, quantizationMode, reluPreMode>(dst, src, gShape0, gShape1, gShape2,
            gShape3, gShape4, gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void CheckStaticForVecAndMat()
{
    static_assert(
        std::is_same_v<typename TileData::DType, int8_t> || std::is_same_v<typename TileData::DType, uint8_t> ||
            std::is_same_v<typename TileData::DType, int16_t> || std::is_same_v<typename TileData::DType, uint16_t> ||
            std::is_same_v<typename TileData::DType, int32_t> || std::is_same_v<typename TileData::DType, uint32_t> ||
            std::is_same_v<typename TileData::DType, int64_t> || std::is_same_v<typename TileData::DType, uint64_t> ||
            std::is_same_v<typename TileData::DType, half> || std::is_same_v<typename TileData::DType, bfloat16_t> ||
            std::is_same_v<typename TileData::DType, float>,
        "Data type must be int8_t/uint8_t/int16_t/uint16_t/int32_t/uint32_t/int64_t/uint64_t/half/bfloat16_t/float!");
    static_assert(sizeof(typename TileData::DType) == sizeof(typename GlobalData::DType),
        "Source dtype must be same with dst dtype!");
    static_assert(((GlobalData::layout == pto::Layout::ND) &&
                      (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox))) ||
                      ((GlobalData::layout == pto::Layout::DN) &&
                          (!TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox))) ||
                      ((GlobalData::layout == pto::Layout::NZ) &&
                          (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) ||
                      (TileData::Rows == 1) || (TileData::Cols == 1),
        "Src and dst layout must be same, only support ND/DN/NZ or the special case of one row/one column!");
    if constexpr (std::is_same_v<typename TileData::DType, int64_t> ||
                  std::is_same_v<typename TileData::DType, uint64_t>) {
        static_assert((GlobalData::layout == pto::Layout::ND &&
                          (TileData::isRowMajor && TileData::SFractal == SLayout::NoneBox)) ||
                          (GlobalData::layout == pto::Layout::DN &&
                              (!TileData::isRowMajor && TileData::SFractal == SLayout::NoneBox)),
            "TSTORE(GlobalTensor, VecTile) only support ND2ND/DN2DN for b64!");
    }
}

template <typename TileData, typename GlobalData, bool isQuant>
PTO_INTERNAL void CheckAcc2gm(GlobalData &dst, TileData &src)
{
    static_assert((GlobalData::layout == pto::Layout::ND || GlobalData::layout == pto::Layout::NZ),
        "The output data layout must be ND or NZ.");
    static_assert(std::is_same_v<typename TileData::DType, int32_t> || std::is_same_v<typename TileData::DType, float>,
        "The input data type must be restricted to int32_t/float!");
    if constexpr (!isQuant) {
        static_assert(std::is_same_v<typename GlobalData::DType, __gm__ int32_t> ||
                          std::is_same_v<typename GlobalData::DType, __gm__ float> ||
                          std::is_same_v<typename GlobalData::DType, __gm__ half> ||
                          std::is_same_v<typename GlobalData::DType, __gm__ bfloat16_t>,
            "The output data type must be restricted to int32_t/float/half/bfloat16_t!");
    } else if constexpr (isQuant) {
        if constexpr (std::is_same_v<typename TileData::DType, float>) {
            static_assert(std::is_same<typename GlobalData::DType, __gm__ int8_t>::value ||
                              std::is_same<typename GlobalData::DType, __gm__ uint8_t>::value,
                "The output data type must be restricted to int8_t/uint8_t.");
        } else if constexpr (std::is_same_v<typename TileData::DType, __gm__ int32_t>) {
            static_assert(std::is_same<typename GlobalData::DType, __gm__ int8_t>::value ||
                              std::is_same<typename GlobalData::DType, __gm__ uint8_t>::value ||
                              std::is_same<typename GlobalData::DType, __gm__ half>::value,
                "The output data type must be restricted to half/int8_t/uint8_t.");
        }
    }
    static_assert(TileData::Cols >= 1 && TileData::Cols <= 4095, "The range of Cols is [1, 4095].");
    static_assert((GlobalData::layout == pto::Layout::ND && TileData::Rows >= 1 && TileData::Rows <= 8192) ||
                      (GlobalData::layout == pto::Layout::NZ && TileData::Rows >= 1 && TileData::Rows <= 65535 &&
                          TileData::Cols % 16 == 0),
        "When GlobalData is ND format, the range of Rows is [1, 8192]."
        "When GlobalData is NZ format, the range of Rows is [1, 65535] and Cols "
        "must be an integer multiple of 16.");
    PTO_ASSERT(src.GetValidCol() >= 1 && src.GetValidCol() <= 4095, "The range of validCol is [1, 4095].");
    PTO_ASSERT(dst.GetShape(pto::GlobalTensorDim::DIM_0) > 0 && dst.GetShape(pto::GlobalTensorDim::DIM_1) > 0 &&
                   dst.GetShape(pto::GlobalTensorDim::DIM_2) > 0 && dst.GetShape(pto::GlobalTensorDim::DIM_3) > 0 &&
                   dst.GetShape(pto::GlobalTensorDim::DIM_4) > 0 && src.GetValidRow() > 0 && src.GetValidCol() > 0,
        "The shape of src and dst must be greater than 0!");
}

template <typename TileData, typename GlobalData, AtomicType currentAtomicType = AtomicType::AtomicNone>
PTO_INTERNAL void TSTORE_IMPL(GlobalData &dst, TileData &src)
{
    static_assert(TileData::Loc == pto::TileType::Vec || TileData::Loc == pto::TileType::Acc ||
                      TileData::Loc == pto::TileType::Mat,
        "Source TileType only suport Vec/Acc/Mat!");
    PTO_ASSERT(dst.GetShape(pto::GlobalTensorDim::DIM_0) > 0 && dst.GetShape(pto::GlobalTensorDim::DIM_1) > 0 &&
                   dst.GetShape(pto::GlobalTensorDim::DIM_2) > 0 && dst.GetShape(pto::GlobalTensorDim::DIM_3) > 0 &&
                   dst.GetShape(pto::GlobalTensorDim::DIM_4) > 0 && src.GetValidRow() > 0 && src.GetValidCol() > 0,
        "The shape of src and dst must be greater than 0!");
    if constexpr (currentAtomicType == AtomicType::AtomicAdd) {
        SetAtomicAdd<typename GlobalData::DType>();
    }
    if constexpr (TileData::Loc == pto::TileType::Vec) {
        CheckStaticForVecAndMat<TileData, GlobalData>();
        TStore<GlobalData, TileData>(dst.data(), src.data(), dst.GetShape(pto::GlobalTensorDim::DIM_0),
            dst.GetShape(pto::GlobalTensorDim::DIM_1), dst.GetShape(pto::GlobalTensorDim::DIM_2),
            dst.GetShape(pto::GlobalTensorDim::DIM_3), dst.GetShape(pto::GlobalTensorDim::DIM_4),
            dst.GetStride(pto::GlobalTensorDim::DIM_0), dst.GetStride(pto::GlobalTensorDim::DIM_1),
            dst.GetStride(pto::GlobalTensorDim::DIM_2), dst.GetStride(pto::GlobalTensorDim::DIM_3),
            dst.GetStride(pto::GlobalTensorDim::DIM_4), src.GetValidRow(), src.GetValidCol());
    } else if constexpr (TileData::Loc == pto::TileType::Acc) {
        CheckAcc2gm<TileData, GlobalData, false>(dst, src);
        constexpr QuantMode_t quantMode = GetCastPreQuantMode<typename TileData::DType, typename GlobalData::DType>();
        TStoreAcc<GlobalData, TileData, quantMode>(dst.data(), src.data(), dst.GetShape(pto::GlobalTensorDim::DIM_0),
            dst.GetShape(pto::GlobalTensorDim::DIM_1), dst.GetShape(pto::GlobalTensorDim::DIM_2),
            dst.GetShape(pto::GlobalTensorDim::DIM_3), dst.GetShape(pto::GlobalTensorDim::DIM_4),
            dst.GetStride(pto::GlobalTensorDim::DIM_0), dst.GetStride(pto::GlobalTensorDim::DIM_1),
            dst.GetStride(pto::GlobalTensorDim::DIM_2), dst.GetStride(pto::GlobalTensorDim::DIM_3),
            dst.GetStride(pto::GlobalTensorDim::DIM_4), src.GetValidRow(), src.GetValidCol());
    } else if constexpr (TileData::Loc == pto::TileType::Mat) {
        CheckStaticForVecAndMat<TileData, GlobalData>();
        TStoreMat<GlobalData, TileData>(dst.data(), src.data(), dst.GetShape(pto::GlobalTensorDim::DIM_0),
            dst.GetShape(pto::GlobalTensorDim::DIM_1), dst.GetShape(pto::GlobalTensorDim::DIM_2),
            dst.GetShape(pto::GlobalTensorDim::DIM_3), dst.GetShape(pto::GlobalTensorDim::DIM_4),
            dst.GetStride(pto::GlobalTensorDim::DIM_0), dst.GetStride(pto::GlobalTensorDim::DIM_1),
            dst.GetStride(pto::GlobalTensorDim::DIM_2), dst.GetStride(pto::GlobalTensorDim::DIM_3),
            dst.GetStride(pto::GlobalTensorDim::DIM_4), src.GetValidRow(), src.GetValidCol());
    }
    if constexpr (currentAtomicType == AtomicType::AtomicAdd) {
        SetAtomicNone();
    }
}

template <typename TileData, typename GlobalData, AtomicType currentAtomicType = AtomicType::AtomicNone,
    ReluPreMode reluPreMode>
PTO_INTERNAL void TSTORE_IMPL(GlobalData &dst, TileData &src)
{
    static_assert(TileData::Loc == pto::TileType::Acc, "Source TileType only suport Acc!");
    CheckAcc2gm<TileData, GlobalData, false>(dst, src);
    if constexpr (currentAtomicType == AtomicType::AtomicAdd) {
        SetAtomicAdd<typename GlobalData::DType>();
    }
    constexpr QuantMode_t quantMode = GetCastPreQuantMode<typename TileData::DType, typename GlobalData::DType>();
    TStoreAcc<GlobalData, TileData, quantMode, reluPreMode>(dst.data(), src.data(),
        dst.GetShape(pto::GlobalTensorDim::DIM_0), dst.GetShape(pto::GlobalTensorDim::DIM_1),
        dst.GetShape(pto::GlobalTensorDim::DIM_2), dst.GetShape(pto::GlobalTensorDim::DIM_3),
        dst.GetShape(pto::GlobalTensorDim::DIM_4), dst.GetStride(pto::GlobalTensorDim::DIM_0),
        dst.GetStride(pto::GlobalTensorDim::DIM_1), dst.GetStride(pto::GlobalTensorDim::DIM_2),
        dst.GetStride(pto::GlobalTensorDim::DIM_3), dst.GetStride(pto::GlobalTensorDim::DIM_4),
        src.GetValidRow(), src.GetValidCol());
    if constexpr (currentAtomicType == AtomicType::AtomicAdd) {
        SetAtomicNone();
    }
}

template <typename TileData, typename GlobalData, AtomicType currentAtomicType = AtomicType::AtomicNone,
    ReluPreMode reluPreMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TSTORE_IMPL(GlobalData &dst, TileData &src, uint64_t preQuantScalar)
{
    static_assert(TileData::Loc == pto::TileType::Acc, "Source TileType only suport Acc!");
    CheckAcc2gm<TileData, GlobalData, true>(dst, src);
    if constexpr (currentAtomicType == AtomicType::AtomicAdd) {
        SetAtomicAdd<typename GlobalData::DType>();
    }
    set_quant_pre(preQuantScalar);
    constexpr QuantMode_t quantMode = GetScalarPreQuantMode<typename TileData::DType, typename GlobalData::DType>();
    TStoreAcc<GlobalData, TileData, quantMode, reluPreMode>(dst.data(), src.data(),
        dst.GetShape(pto::GlobalTensorDim::DIM_0), dst.GetShape(pto::GlobalTensorDim::DIM_1),
        dst.GetShape(pto::GlobalTensorDim::DIM_2), dst.GetShape(pto::GlobalTensorDim::DIM_3),
        dst.GetShape(pto::GlobalTensorDim::DIM_4), dst.GetStride(pto::GlobalTensorDim::DIM_0),
        dst.GetStride(pto::GlobalTensorDim::DIM_1), dst.GetStride(pto::GlobalTensorDim::DIM_2),
        dst.GetStride(pto::GlobalTensorDim::DIM_3), dst.GetStride(pto::GlobalTensorDim::DIM_4), src.GetValidRow(),
        src.GetValidCol());
    if constexpr (AtomicType::AtomicAdd == currentAtomicType) {
        SetAtomicNone();
    }
}

template <typename TileData, typename GlobalData, typename FpTileData,
    AtomicType currentAtomicType = AtomicType::AtomicNone, ReluPreMode reluPreMode = ReluPreMode::NoRelu>
PTO_INTERNAL void TSTORE_IMPL(GlobalData &dst, TileData &src, FpTileData &fp)
{
    static_assert(TileData::Loc == pto::TileType::Acc, "Source TileType only suport Acc!");
    CheckAcc2gm<TileData, GlobalData, true>(dst, src);
    if constexpr (AtomicType::AtomicAdd == currentAtomicType) {
        SetAtomicAdd<typename GlobalData::DType>();
    }
    constexpr QuantMode_t quantMode = GetVectorPreQuantMode<typename TileData::DType, typename GlobalData::DType>();
    TStoreAccFp<GlobalData, TileData, FpTileData, quantMode, reluPreMode>(dst.data(), src.data(), fp.data(),
        dst.GetShape(pto::GlobalTensorDim::DIM_0), dst.GetShape(pto::GlobalTensorDim::DIM_1),
        dst.GetShape(pto::GlobalTensorDim::DIM_2), dst.GetShape(pto::GlobalTensorDim::DIM_3),
        dst.GetShape(pto::GlobalTensorDim::DIM_4), dst.GetStride(pto::GlobalTensorDim::DIM_0),
        dst.GetStride(pto::GlobalTensorDim::DIM_1), dst.GetStride(pto::GlobalTensorDim::DIM_2),
        dst.GetStride(pto::GlobalTensorDim::DIM_3), dst.GetStride(pto::GlobalTensorDim::DIM_4), src.GetValidRow(),
        src.GetValidCol());
    if constexpr (currentAtomicType == AtomicType::AtomicAdd) {
        SetAtomicNone();
    }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} // namespace pto
#endif
