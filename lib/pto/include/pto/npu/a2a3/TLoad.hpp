/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TLOAD_HPP
#define TLOAD_HPP

namespace pto {
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadInstrGm2ub(__ubuf__ typename TileData::DType *dst, typename GlobalData::DType *src,
    uint16_t nBurst, uint32_t lenBurst, uint32_t gmGap, uint32_t ubGap, uint32_t ubPad) {
    if constexpr (sizeof(typename TileData::DType) == 1) {
        copy_gm_to_ubuf_align_b8(dst, src, 0, nBurst, lenBurst, 0, ubPad, gmGap, ubGap);
    } else if constexpr (sizeof(typename TileData::DType) == 2) {
        copy_gm_to_ubuf_align_b16(dst, src, 0, nBurst, lenBurst, 0, ubPad, gmGap, ubGap);
    } else if constexpr (sizeof(typename TileData::DType) == 4) {
        copy_gm_to_ubuf_align_b32(dst, src, 0, nBurst, lenBurst, 0, ubPad, gmGap, ubGap);
    } else if constexpr (sizeof(typename TileData::DType) == 8) {
        copy_gm_to_ubuf_align_b32(dst, src, 0, nBurst, lenBurst, 0, ubPad * 2, gmGap, ubGap);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadNd2nzInstr(__cbuf__ typename TileData::DType *dst, typename GlobalData::DType *src,
    uint16_t ndNum, uint16_t nValue, uint16_t dValue, uint16_t srcNdMatrixStride, uint16_t srcDValue,
    uint16_t dstNzC0Stride, uint16_t dstNzNStride, uint16_t dstNzMatrixStride) {
    // Parameter list:
    // dst, src, sid, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue,
    // dstNzC0Stride, dstNzNStride, dstNzMatrixStride
    if constexpr (sizeof(typename TileData::DType) == 1) {
        copy_gm_to_cbuf_multi_nd2nz_b8(dst, src, 0, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue, dstNzC0Stride,
            dstNzNStride, dstNzMatrixStride);
    } else if constexpr (sizeof(typename TileData::DType) == 2) {
        copy_gm_to_cbuf_multi_nd2nz_b16(dst, src, 0, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue, dstNzC0Stride,
            dstNzNStride, dstNzMatrixStride);
    } else if constexpr (sizeof(typename TileData::DType) == 4) {
        copy_gm_to_cbuf_multi_nd2nz_b32s(dst, src, 0, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue,
            dstNzC0Stride, dstNzNStride, dstNzMatrixStride);
    } else if constexpr (sizeof(typename TileData::DType) == 8) {
        auto dstCast = reinterpret_cast<__cbuf__ uint32_t*>(dst);
        auto srcCast = reinterpret_cast<__gm__ uint32_t*>(src);
        uint16_t dValueb64 = dValue * 2;
        uint16_t srcDValueb64 = srcDValue * 2;
        copy_gm_to_cbuf_multi_nd2nz_b32s(dstCast, srcCast, 0, ndNum, nValue, dValueb64, srcNdMatrixStride, srcDValueb64,
            dstNzC0Stride, dstNzNStride, dstNzMatrixStride);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadInstrGm2L1(__cbuf__ typename TileData::DType *dst, typename GlobalData::DType *src,
    uint16_t nBurst, uint16_t lenBurst, uint16_t gmGap, uint16_t l1Gap) {
    copy_gm_to_cbuf(dst, src, (uint8_t)0, nBurst, lenBurst, gmGap, l1Gap, (pad_t)0);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2ubNd2nd(__ubuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    static_assert(TileData::Rows < 4096, "Fix: TLOAD Rows>=4095 not supported in A2/A3");
    PTO_ASSERT(validCol == gShape4, "The validCol of TileData must be equal to the 5th dim(Shape4) of ND shape!");
    PTO_ASSERT(validRow == gShape0 * gShape1 * gShape2 * gShape3,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape3) of ND shape!");
    PTO_ASSERT(gShape3 < 4096, "The gshape3 (which equals nBurst) must be less than 4096 for A2/A3");
    constexpr uint32_t blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileData::DType);
    uint16_t nBurst = gShape3;
    uint32_t lenBurst = validCol * sizeof(typename TileData::DType);
    uint64_t gmGapValue = (gStride3 - gShape4) * sizeof(typename TileData::DType);
    uint32_t gmGap = (uint32_t)gmGapValue;
    uint32_t ubGapElement = (TileData::Cols - validCol);
    uint32_t ubGap = (ubGapElement * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint32_t ubPad = 0;
    if constexpr (TileData::PadVal != PadValue::Null) {
        ubPad = ubGapElement % blockSizeElem;
        set_mov_pad_val(GetPadValue<TileData>());
    }
    __ubuf__ typename TileData::DType *dstAddrP = dstAddr;
    typename GlobalData::DType *srcAddrP = srcAddr;
    int64_t dstStride2 = gShape3 * TileData::Cols;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;

    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t srcAddr0 = i * gStride0;
        int64_t dstAddr0 = i * dstStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t srcAddr1 = j * gStride1;
            int64_t dstAddr1 = j * dstStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                srcAddrP = srcAddr + srcAddr0 + srcAddr1 + k * gStride2;
                dstAddrP = dstAddr + dstAddr0 + dstAddr1 + k * dstStride2;
                TLoadInstrGm2ub<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmGap, ubGap, ubPad);
            }
        }
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2ubDn2dn(__ubuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    PTO_ASSERT(validRow == gShape3, "The validCol of TileData must be equal to the 4th dim(Shape3) of DN shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape2 * gShape4,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape4) of DN shape!");
    PTO_ASSERT(gShape4 < 4096, "The gshape4 (which equals nBurst) must be less than 4096 for A2/A3");
    constexpr uint32_t blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileData::DType);
    uint16_t nBurst = gShape4;
    uint32_t lenBurst = validRow * sizeof(typename TileData::DType);
    uint64_t gmGapValue = (gStride4 - gShape3) * sizeof(typename TileData::DType);
    uint32_t gmGap = (uint32_t)gmGapValue;
    uint32_t ubGapElement = (TileData::Rows - gShape3);
    uint32_t ubGap = (ubGapElement * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint32_t ubPad = 0;
    if constexpr (TileData::PadVal != PadValue::Null) {
        ubPad = ubGapElement % blockSizeElem;
        set_mov_pad_val(GetPadValue<TileData>());
    }
    typename GlobalData::DType *srcAddrP = srcAddr;
    __ubuf__ typename TileData::DType *dstAddrP = dstAddr;

    int64_t dstStride2 = gShape4 * TileData::Rows;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * dstStride0;
        int64_t srcAddr0 = i * gStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t dstAddr1 = j * dstStride1;
            int64_t srcAddr1 = j * gStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                dstAddrP = dstAddr + dstAddr0 + dstAddr1 + k * dstStride2;
                srcAddrP = srcAddr + srcAddr0 + srcAddr1 + k * gStride2;
                TLoadInstrGm2ub<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmGap, ubGap, ubPad);
            }
        }
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void CheckNzFormat(
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int validRow, int validCol) {
    static_assert(GlobalData::staticShape[3] == FRACTAL_NZ_ROW &&
                      GlobalData::staticShape[4] == C0_SIZE_BYTE / sizeof(typename TileData::DType),
        "Fix: When TileData is NZ format, the last 2 dim must be static and satisfy [16, 32 / sizeof(DataType)]");
    PTO_ASSERT(validRow == gShape2 * gShape3, "The validRow of TileData must be equal to Shape2 * Shape3 of NZ shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape4,
        "The validCol of TileData must be equal to Shape0 * Shape1 * Shape4 of NZ shape!");
    PTO_ASSERT(gShape1 < 4096, "The gshape1 (which equals nBurst) must be less than 4096 for A2/A3");
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2ubNz2nz(__ubuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    CheckNzFormat<TileData, GlobalData>(gShape0, gShape1, gShape2, gShape3, gShape4, validRow, validCol);
    uint16_t nBurst = gShape1;
    uint32_t lenBurst = validRow * C0_SIZE_BYTE;
    uint32_t gmGap = (gStride1 - gShape2 * gShape3 * gShape4) * sizeof(typename TileData::DType);
    uint32_t ubGap = TileData::Rows - validRow;
    typename GlobalData::DType *srcAddrP = srcAddr;
    __ubuf__ typename TileData::DType *dstAddrP = dstAddr;
    int64_t tileStride = TileData::Rows * gShape1 * gShape4;
    for (uint32_t i = 0; i < gShape0; i++) {
        srcAddrP = srcAddr + i * gStride0;
        dstAddrP = dstAddr + i * tileStride;
        TLoadInstrGm2ub<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmGap, ubGap, 0);
    }
}

template <typename TileData, typename GlobalData>
__tf__ AICORE void TLoadGm2ub(typename TileData::TileDType __out__ dst, typename GlobalData::DType __in__ *src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    __ubuf__ typename TileData::DType *dstAddr = (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(dst);
    typename GlobalData::DType *srcAddr = src;
    if constexpr (GetTileLayoutCustom<TileData>() == TileLayoutCustom::ND) {
        TLoadGm2ubNd2nd<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (GetTileLayoutCustom<TileData>() == TileLayoutCustom::DN) {
        TLoadGm2ubDn2dn<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (GetTileLayoutCustom<TileData>() == TileLayoutCustom::NZ) {
        TLoadGm2ubNz2nz<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2L1Nd2nd(__cbuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    PTO_ASSERT(gShape4 * sizeof(typename TileData::DType) % BLOCK_BYTE_SIZE == 0,
        "The 5th dim of ND shape must be 32 bytes aligned!");
    PTO_ASSERT(validCol == gShape4, "The validCol of TileData must be equal to the 5th dim(Shape4) of ND shape!");
    PTO_ASSERT(validRow == gShape0 * gShape1 * gShape2 * gShape3,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape3) of ND shape!");
    PTO_ASSERT(gShape3 < 4096, "The gshape3 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape3;
    uint16_t lenBurst = (validCol * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t gmGap = ((gStride3 - gShape4) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t l1Gap = ((TileData::Cols - validCol) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;

    int64_t dstStride2 = gShape3 * TileData::Cols;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;
    typename GlobalData::DType *srcAddrP = srcAddr;
    __cbuf__ typename TileData::DType *dstAddrP = dstAddr;
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t srcAddr0 = i * gStride0;
        int64_t dstAddr0 = i * dstStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t dstAddr1 = j * dstStride1;
            int64_t srcAddr1 = j * gStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                srcAddrP = srcAddr + srcAddr0 + srcAddr1 + k * gStride2;
                dstAddrP = dstAddr + dstAddr0 + dstAddr1 + k * dstStride2;
                TLoadInstrGm2L1<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmGap, l1Gap);
            }
        }
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2L1Dn2dn(__cbuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    PTO_ASSERT(gShape3 * sizeof(typename TileData::DType) % BLOCK_BYTE_SIZE == 0,
        "The 4th dim of DN shape must be 32 bytes aligned!");
    PTO_ASSERT(validRow == gShape3, "The validCol of TileData must be equal to the 4th dim(Shape3) of DN shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape2 * gShape4,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape4) of DN shape!");
    PTO_ASSERT(gShape4 < 4096, "The gshape4 (which equals nBurst) must be less than 4096 for A2/A3");
    uint16_t nBurst = gShape4;
    uint16_t lenBurst = (validRow * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t gmGap = ((gStride4 - gShape3) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint16_t l1Gap = ((TileData::Rows - gShape3) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    __cbuf__ typename TileData::DType *dstAddrP = dstAddr;
    typename GlobalData::DType *srcAddrP = srcAddr;

    int64_t dstStride2 = gShape4 * TileData::Rows;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * dstStride0;
        int64_t srcAddr0 = i * gStride0;
        for (uint32_t j = 0; j < gShape1; j++) {
            int64_t srcAddr1 = j * gStride1;
            int64_t dstAddr1 = j * dstStride1;
            for (uint32_t k = 0; k < gShape2; k++) {
                srcAddrP = srcAddr + srcAddr0 + srcAddr1 + k * gStride2;
                dstAddrP = dstAddr + dstAddr0 + dstAddr1 + k * dstStride2;
                TLoadInstrGm2L1<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmGap, l1Gap);
            }
        }
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2L1Nz2nz(__cbuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    CheckNzFormat<TileData, GlobalData>(gShape0, gShape1, gShape2, gShape3, gShape4, validRow, validCol);
    uint16_t nBurst = gShape1;
    uint32_t lenBurst = validRow;
    uint32_t gmGap = ((gStride1 - gShape2 * gShape3 * gShape4) * sizeof(typename TileData::DType)) >> SHIFT_BLOCK_BYTE;
    uint32_t l1Gap = TileData::Rows - validRow;
    typename GlobalData::DType *srcAddrP = srcAddr;
    __cbuf__ typename TileData::DType *dstAddrP = dstAddr;
    int64_t tileStride = TileData::Rows * gShape1 * gShape4;

    for (uint32_t i = 0; i < gShape0; i++) {
        srcAddrP = srcAddr + i * gStride0;
        dstAddrP = dstAddr + i * tileStride;
        TLoadInstrGm2L1<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmGap, l1Gap);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2L1VectorInND(__cbuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    PTO_ASSERT(validCol == gShape4, "The validCol of TileData must be equal to the 5th dim(Shape4) of ND shape!");
    PTO_ASSERT(validRow == gShape0 * gShape1 * gShape2 * gShape3,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape3) of ND shape!");
    static_assert(GlobalData::staticShape[0] == 1 && GlobalData::staticShape[1] == 1 && GlobalData::staticShape[2] == 1,
        "Fix: GlobalTensor ony support 2 dim when using vector input!");
    uint16_t nValue = gShape3;
    uint16_t dValue = gShape4;
    uint16_t srcDValue = gStride3;
    typename GlobalData::DType *srcAddrP = srcAddr;
    __cbuf__ typename TileData::DType *dstAddrP = dstAddr;
    // Parameter list:
    // dst, src, sid, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue,
    // dstNzC0Stride, dstNzNStride, dstNzMatrixStride
    TLoadNd2nzInstr<TileData, GlobalData>(dstAddrP, srcAddrP, 1, nValue, dValue, 0, srcDValue, TileData::Rows, 1, 1);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadGm2L1VectorInDn(__cbuf__ typename TileData::DType *dstAddr, typename GlobalData::DType *srcAddr,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    static_assert(GlobalData::staticShape[0] == 1 && GlobalData::staticShape[1] == 1 && GlobalData::staticShape[2] == 1,
        "Fix: GlobalTensor ony support 2 dim when using vector input!");
    PTO_ASSERT(validRow == gShape3, "The validCol of TileData must be equal to the 4th dim(Shape3) of DN shape!");
    PTO_ASSERT(validCol == gShape0 * gShape1 * gShape2 * gShape4,
        "The validRow of TileData must be equal to (Shape0 * Shape1 * Shape2 * Shape4) of DN shape!");
    uint16_t nValue = gShape4;
    uint16_t dValue = gShape3;
    uint16_t srcDValue = gStride3;
    typename GlobalData::DType *srcAddrP = srcAddr;
    __cbuf__ typename TileData::DType *dstAddrP = dstAddr;
    // Parameter list:
    // dst, src, sid, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue,
    // dstNzC0Stride, dstNzNStride, dstNzMatrixStride
    TLoadNd2nzInstr<TileData, GlobalData>(dstAddrP, srcAddrP, 1, nValue, dValue, 0, srcDValue, TileData::Cols, 1, 1);
}

template <typename TileData, typename GlobalData>
__tf__ AICORE void TLoadGm2L1(typename TileData::TileDType __out__ dst, typename GlobalData::DType __in__ *src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    __cbuf__ typename TileData::DType *dstAddr = (__cbuf__ typename TileData::DType *)__cce_get_tile_ptr(dst);
    typename GlobalData::DType *srcAddr = src;
    if constexpr (GetTileLayoutCustom<TileData>() == TileLayoutCustom::ND) {
        if constexpr (TileData::Rows == 1) {
            TLoadGm2L1VectorInND<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4,
                gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
        } else {
            TLoadGm2L1Nd2nd<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4,
                gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
        }
    } else if constexpr (GetTileLayoutCustom<TileData>() == TileLayoutCustom::DN) {
        if constexpr (TileData::Cols == 1) {
            TLoadGm2L1VectorInDn<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4,
                gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
        } else {
            TLoadGm2L1Dn2dn<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4,
                gStride0, gStride1, gStride2, gStride3, gStride4, validRow, validCol);
        }
    } else if constexpr (GetTileLayoutCustom<TileData>() == TileLayoutCustom::NZ) {
        TLoadGm2L1Nz2nz<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename TileData, typename GlobalData>
__tf__ AICORE void TLoadGm2L1Nd2nz(typename TileData::TileDType __out__ dst, typename GlobalData::DType __in__ *src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol)
{
    __cbuf__ typename TileData::DType *dstAddr = (__cbuf__ typename TileData::DType *)__cce_get_tile_ptr(dst);
    typename GlobalData::DType *srcAddr = src;
    static_assert(GlobalData::staticShape[0] == 1 && GlobalData::staticShape[1] == 1 && GlobalData::staticShape[2] == 1,
        "Fix: GlobalTensor ony support 2 dim when ND2NZ!");
    static_assert(TileData::SFractalSize == 512 || TileData::SFractalSize == 32,
        "Fix: TileData ony support SFractalSize = 512Bytes or 32Bytes!");
    PTO_ASSERT(gShape3 > 0 && gShape3 <= 16384, "The Shape3 of GlobalTensor must be in range of [1, 16384]!");
    PTO_ASSERT(gShape4 > 0 && gShape4 <= 65535, "The Shape4 of GlobalTensor must be must be in range of [1, 65535]!");
    PTO_ASSERT(
        gStride3 > 0 && gStride3 <= 65535, "The Stride3 of GlobalTensor must be must be in range of [1, 65535]!");
    static_assert(TileData::Rows <= 16384, "Fix: The Rows of TileData must be less than 16384!");

    uint16_t nValue = gShape3;
    uint16_t dValue = gShape4;
    uint16_t srcDValue = gStride3;
    // Parameter list:
    // dst, src, sid, ndNum, nValue, dValue, srcNdMatrixStride, srcDValue,
    // dstNzC0Stride, dstNzNStride, dstNzMatrixStride
    TLoadNd2nzInstr<TileData, GlobalData>(dstAddr, srcAddr, 1, nValue, dValue, 0, srcDValue, TileData::Rows, 1, 1);
}

template <typename TileData, typename GlobalData>
__tf__ AICORE void TLoadGm2L1Dn2zn(typename TileData::TileDType __out__ dst, typename GlobalData::DType __in__ *src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    static_assert(GlobalData::staticShape[0] == 1 && GlobalData::staticShape[1] == 1 && GlobalData::staticShape[2] == 1,
        "Fix: GlobalTensor ony support 2 dim when DN2ZN!");
    static_assert(TileData::SFractalSize == 512, "Fix: TileData ony support SFractalSize = 512Bytes!");
    __cbuf__ typename TileData::DType *dstAddr = (__cbuf__ typename TileData::DType *)__cce_get_tile_ptr(dst);
    typename GlobalData::DType *srcAddr = src;
    PTO_ASSERT(gShape4 > 0 && gShape4 <= 16384, "The Shape4 of GlobalTensor must be in range of [1, 16384]!");
    PTO_ASSERT(gShape3 > 0 && gShape3 <= 65535, "The Shape3 of GlobalTensor must be must be in range of [1, 65535]!");
    PTO_ASSERT(
        gStride4 > 0 && gStride4 <= 65535, "The Stride3 of GlobalTensor must be must be in range of [1, 65535]!");
    static_assert(TileData::Cols <= 16384, "Fix: The Cols of TileData must be less than 16384!");

    uint16_t nValue = gShape4;
    uint16_t dValue = gShape3;
    uint16_t srcDValue = gStride4;
    TLoadNd2nzInstr<TileData, GlobalData>(dstAddr, srcAddr, 1, nValue, dValue, 0, srcDValue, TileData::Cols, 1, 1);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void CheckTloadData(TileData &dst, GlobalData &src) {
    static_assert(
        std::is_same_v<typename TileData::DType, int8_t> || std::is_same_v<typename TileData::DType, uint8_t> ||
            std::is_same_v<typename TileData::DType, int16_t> || std::is_same_v<typename TileData::DType, uint16_t> ||
            std::is_same_v<typename TileData::DType, int32_t> || std::is_same_v<typename TileData::DType, uint32_t> ||
            std::is_same_v<typename TileData::DType, int64_t> || std::is_same_v<typename TileData::DType, uint64_t> ||
            std::is_same_v<typename TileData::DType, half> || std::is_same_v<typename TileData::DType, bfloat16_t> ||
            std::is_same_v<typename TileData::DType, float>,
        "Fix: Data type must be "
        "int8_t/uint8_t/int16_t/uint16_t/int32_t/uint32_t/half/bfloat16_t/float/int64_t/uint64_t!");
    static_assert(TileData::Loc == pto::TileType::Vec || TileData::Loc == pto::TileType::Mat,
        "Fix: Dst TileType must be Vec or Mat!");
    static_assert(sizeof(typename TileData::DType) == sizeof(typename GlobalData::DType),
        "Fix: Source dtype must be same with dst dtype!");

    if constexpr (std::is_same_v<typename TileData::DType, int64_t> ||
                  std::is_same_v<typename TileData::DType, uint64_t>) {
        static_assert(
            (GlobalData::layout == pto::Layout::ND && GetTileLayoutCustom<TileData>() == TileLayoutCustom::ND) ||
                (GlobalData::layout == pto::Layout::DN && GetTileLayoutCustom<TileData>() == TileLayoutCustom::DN),
            "Fix: TLOAD only support ND2ND/DN2DN for b64!");
    }
    PTO_ASSERT(src.GetShape(pto::GlobalTensorDim::DIM_0) > 0 && src.GetShape(pto::GlobalTensorDim::DIM_1) > 0 &&
                   src.GetShape(pto::GlobalTensorDim::DIM_2) > 0 && src.GetShape(pto::GlobalTensorDim::DIM_3) > 0 &&
                   src.GetShape(pto::GlobalTensorDim::DIM_4) > 0 && dst.GetValidRow() > 0 && dst.GetValidCol() > 0,
        "The shape of src and dst must be greater than 0!");
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLOAD_IMPL(TileData &dst, GlobalData &src) {
    CheckTloadData<TileData, GlobalData>(dst, src);
    constexpr bool isSameLayout =
        (GlobalData::layout == pto::Layout::ND && GetTileLayoutCustom<TileData>() == TileLayoutCustom::ND) ||
        (GlobalData::layout == pto::Layout::DN && GetTileLayoutCustom<TileData>() == TileLayoutCustom::DN) ||
        (GlobalData::layout == pto::Layout::NZ && GetTileLayoutCustom<TileData>() == TileLayoutCustom::NZ);
    if constexpr (TileData::Loc == pto::TileType::Vec) {
        static_assert(isSameLayout, "Fix: TLOAD(VecTile, GlobalTensor) only support ND2ND/DN2DN/NZ2NZ!");
        TLoadGm2ub<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(pto::GlobalTensorDim::DIM_0),
            src.GetShape(pto::GlobalTensorDim::DIM_1), src.GetShape(pto::GlobalTensorDim::DIM_2),
            src.GetShape(pto::GlobalTensorDim::DIM_3), src.GetShape(pto::GlobalTensorDim::DIM_4),
            src.GetStride(pto::GlobalTensorDim::DIM_0), src.GetStride(pto::GlobalTensorDim::DIM_1),
            src.GetStride(pto::GlobalTensorDim::DIM_2), src.GetStride(pto::GlobalTensorDim::DIM_3),
            src.GetStride(pto::GlobalTensorDim::DIM_4), dst.GetValidRow(), dst.GetValidCol());
    } else if constexpr (TileData::Loc == pto::TileType::Mat) {
        static_assert(
            isSameLayout ||
                (GlobalData::layout == pto::Layout::ND && GetTileLayoutCustom<TileData>() == TileLayoutCustom::NZ) ||
                (GlobalData::layout == pto::Layout::DN && GetTileLayoutCustom<TileData>() == TileLayoutCustom::ZN),
            "Fix: TLOAD(MatTile, GlobalTensor) only support ND2ND/DN2DN/NZ2NZ/ND2NZ/DN2ZN!");
        if constexpr (isSameLayout) {
            TLoadGm2L1<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(pto::GlobalTensorDim::DIM_0),
                src.GetShape(pto::GlobalTensorDim::DIM_1), src.GetShape(pto::GlobalTensorDim::DIM_2),
                src.GetShape(pto::GlobalTensorDim::DIM_3), src.GetShape(pto::GlobalTensorDim::DIM_4),
                src.GetStride(pto::GlobalTensorDim::DIM_0), src.GetStride(pto::GlobalTensorDim::DIM_1),
                src.GetStride(pto::GlobalTensorDim::DIM_2), src.GetStride(pto::GlobalTensorDim::DIM_3),
                src.GetStride(pto::GlobalTensorDim::DIM_4), dst.GetValidRow(), dst.GetValidCol());
        } else if constexpr (GlobalData::layout == pto::Layout::ND &&
                             GetTileLayoutCustom<TileData>() == TileLayoutCustom::NZ) {
            TLoadGm2L1Nd2nz<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(pto::GlobalTensorDim::DIM_0),
                src.GetShape(pto::GlobalTensorDim::DIM_1), src.GetShape(pto::GlobalTensorDim::DIM_2),
                src.GetShape(pto::GlobalTensorDim::DIM_3), src.GetShape(pto::GlobalTensorDim::DIM_4),
                src.GetStride(pto::GlobalTensorDim::DIM_0), src.GetStride(pto::GlobalTensorDim::DIM_1),
                src.GetStride(pto::GlobalTensorDim::DIM_2), src.GetStride(pto::GlobalTensorDim::DIM_3),
                src.GetStride(pto::GlobalTensorDim::DIM_4), dst.GetValidRow(), dst.GetValidCol());
        } else if constexpr (GlobalData::layout == pto::Layout::DN &&
                             GetTileLayoutCustom<TileData>() == TileLayoutCustom::ZN) {
            TLoadGm2L1Dn2zn<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(pto::GlobalTensorDim::DIM_0),
                src.GetShape(pto::GlobalTensorDim::DIM_1), src.GetShape(pto::GlobalTensorDim::DIM_2),
                src.GetShape(pto::GlobalTensorDim::DIM_3), src.GetShape(pto::GlobalTensorDim::DIM_4),
                src.GetStride(pto::GlobalTensorDim::DIM_0), src.GetStride(pto::GlobalTensorDim::DIM_1),
                src.GetStride(pto::GlobalTensorDim::DIM_2), src.GetStride(pto::GlobalTensorDim::DIM_3),
                src.GetStride(pto::GlobalTensorDim::DIM_4), dst.GetValidRow(), dst.GetValidCol());
        }
    }
}
} // namespace pto
#endif // TLOAD_HPP
