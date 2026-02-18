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
#include "common.hpp"

namespace pto {
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadInstr(__ubuf__ typename TileData::DType *dst, typename GlobalData::DType *src, uint32_t nBurst,
    uint32_t lenBurst, uint64_t gmStride, uint32_t ubStride, bool enableUBPad) {
    if constexpr (sizeof(typename TileData::DType) == 1) {
        copy_gm_to_ubuf_align_v2(reinterpret_cast<__ubuf__ uint8_t *>(dst), reinterpret_cast<__gm__ uint8_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, 0 /*right padding count*/,
            enableUBPad /*data select bit*/, 0 /*l2 cache ctl*/, gmStride, ubStride);
    } else if constexpr (sizeof(typename TileData::DType) == 2) {
        copy_gm_to_ubuf_align_v2(reinterpret_cast<__ubuf__ uint16_t *>(dst), reinterpret_cast<__gm__ uint16_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, 0 /*right padding count*/,
            enableUBPad /*data select bit*/, 0 /*l2 cache ctl*/, gmStride, ubStride);
    } else if constexpr (sizeof(typename TileData::DType) == 4) {
        copy_gm_to_ubuf_align_v2(reinterpret_cast<__ubuf__ uint32_t *>(dst), reinterpret_cast<__gm__ uint32_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, 0 /*right padding count*/,
            enableUBPad /*data select bit*/, 0 /*l2 cache ctl*/, gmStride, ubStride);
    } else if constexpr (sizeof(typename TileData::DType) == 8) {
        copy_gm_to_ubuf_align_v2(reinterpret_cast<__ubuf__ uint32_t *>(dst), reinterpret_cast<__gm__ uint32_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, 0 /*right padding count*/,
            enableUBPad /*data select bit*/, 0 /*l2 cache ctl*/, gmStride, ubStride);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadVecND2ND(typename TileData::TileDType dstAddr, typename GlobalData::DType *srcAddr, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol, bool enableUBPad) {
    typename GlobalData::DType *srcAddrP = srcAddr;
    __ubuf__ typename TileData::DType *dstAddrP = dstAddr;
    uint32_t nBurst = gShape3;
    uint32_t lenBurst = GetByteSize<typename TileData::DType>(validCol);
    uint64_t gmStride = GetByteSize<typename TileData::DType>(gStride3);
    uint32_t ubStride = GetByteSize<typename TileData::DType>(TileData::Cols);

    int64_t dstStride2 = gShape3 * TileData::Cols;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        dstStride0 = dstStride0 >> 1; // fp4 dstAddr offset need divide 2 as use b8 to move
        gStride0 = gStride0 >> 1;     // fp4 srcAddr offset need divide 2 as use b8 to move
    }
    uint64_t loop2 = gShape1;
    uint64_t loop1 = gShape2;
    uint64_t loop2_src_stride = GetByteSize<typename TileData::DType>(gStride1);
    uint64_t loop1_src_stride = GetByteSize<typename TileData::DType>(gStride2);
    uint64_t loop2_dst_stride = GetByteSize<typename TileData::DType>(dstStride1);
    uint64_t loop1_dst_stride = GetByteSize<typename TileData::DType>(dstStride2);
    set_loop2_stride_outtoub(loop2_dst_stride << 40 | loop2_src_stride);
    set_loop1_stride_outtoub(loop1_dst_stride << 40 | loop1_src_stride);
    set_loop_size_outtoub(loop2 << 21 | loop1);
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * dstStride0;
        int64_t srcAddr0 = i * gStride0;
        dstAddrP = dstAddr + dstAddr0;
        srcAddrP = srcAddr + srcAddr0;
        TLoadInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, ubStride, enableUBPad);
    }
}
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadVecDN2DN(typename TileData::TileDType dstAddr, typename GlobalData::DType *srcAddr, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol, bool enableUBPad) {
    uint32_t nBurst = gShape4;
    uint32_t lenBurst = GetByteSize<typename TileData::DType>(validRow);
    uint64_t gmStride = GetByteSize<typename TileData::DType>(gStride4);
    uint32_t ubStride = GetByteSize<typename TileData::DType>(TileData::Rows);

    typename GlobalData::DType *srcAddrP = srcAddr;
    __ubuf__ typename TileData::DType *dstAddrP = dstAddr;

    int64_t dstStride2 = gShape4 * TileData::Rows;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;

    uint64_t loop2 = gShape1;
    uint64_t loop1 = gShape2;
    uint64_t loop2_src_stride = GetByteSize<typename TileData::DType>(gStride1);
    uint64_t loop1_src_stride = GetByteSize<typename TileData::DType>(gStride2);
    uint64_t loop2_dst_stride = GetByteSize<typename TileData::DType>(dstStride1);
    uint64_t loop1_dst_stride = GetByteSize<typename TileData::DType>(dstStride2);
    set_loop2_stride_outtoub(loop2_dst_stride << 40 | loop2_src_stride);
    set_loop1_stride_outtoub(loop1_dst_stride << 40 | loop1_src_stride);
    set_loop_size_outtoub(loop2 << 21 | loop1);
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        dstStride0 = dstStride0 >> 1; // fp4 dstAddr offset need divide 2 as use b8 to move
        gStride0 = gStride0 >> 1;     // fp4 srcAddr offset need divide 2 as use b8 to move
    }

    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * dstStride0;
        int64_t srcAddr0 = i * gStride0;
        dstAddrP = dstAddr + dstAddr0;
        srcAddrP = srcAddr + srcAddr0;
        TLoadInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, ubStride, enableUBPad);
    }
}
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadVecNZ2NZ(typename TileData::TileDType dstAddr, typename GlobalData::DType *srcAddr, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint32_t nBurst = gShape1;
    uint32_t lenBurst = validRow * C0_SIZE_BYTE;
    uint32_t gmStride = GetByteSize<typename TileData::DType>(gStride1);
    uint32_t ubStride = TileData::Rows * C0_SIZE_BYTE;

    typename GlobalData::DType *srcAddrP = srcAddr;
    __ubuf__ typename TileData::DType *dstAddrP = dstAddr;

    int64_t tileStride = gShape1 * TileData::Rows * gShape4;
    set_loop_size_outtoub(1ULL << 21 | 1ULL);
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        tileStride = tileStride >> 1; // fp4 dstAddr offset need divide 2 as use b8 to move
        gStride0 = gStride0 >> 1;     // fp4 srcAddr offset need divide 2 as use b8 to move
    }
    for (uint32_t i = 0; i < gShape0; i++) {
        srcAddrP = srcAddr + i * gStride0;
        dstAddrP = dstAddr + i * tileStride;
        TLoadInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, ubStride, 0);
    }
}

template <typename TileData, typename GlobalData>
__tf__ PTO_INTERNAL OP_NAME(TLOAD) OP_TYPE(memory) void TLoad(typename TileData::TileDType __out__ dst,
    typename GlobalData::DType __in__ *src, int gShape0, int gShape1, int gShape2, int gShape3, int gShape4,
    int gStride0, int gStride1, int gStride2, int gStride3, int gStride4, int validRow, int validCol) {
    __ubuf__ typename TileData::DType *dstAddr = (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(dst);
    typename GlobalData::DType *srcAddr = src;
    constexpr bool enableUBPad = TileData::PadVal != PadValue::Null;
    if constexpr (enableUBPad) {
        set_mov_pad_val(GetPadValue<TileData>());
    }
    if constexpr (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)) {
        TLoadVecND2ND<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol, enableUBPad);
    } else if constexpr (!TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)) {
        TLoadVecDN2DN<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol, enableUBPad);
    } else if constexpr (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor)) {
        TLoadVecNZ2NZ<TileData, GlobalData>(dstAddr, srcAddr, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeCheck() {
    // support ND2NZ DN2NZ ND2ND DN2DN NZ2NZ DN2ZN
    static_assert(((GlobalData::layout == pto::Layout::ND) &&
                      (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) ||
                      ((GlobalData::layout == pto::Layout::DN) &&
                          (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) ||
                      ((GlobalData::layout == pto::Layout::NZ) &&
                          (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) ||
                      (((GlobalData::layout == pto::Layout::ND) &&
                          (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)))) ||
                      (((GlobalData::layout == pto::Layout::DN) &&
                          (!TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)))) ||
                      (((GlobalData::layout == pto::Layout::DN) &&
                          (TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor)))),
        "Fix: now only support ND2NZ DN2NZ ND2ND DN2DN NZ2NZ DN2ZN in current platform");

    // L1 space check
    static_assert(TileData::Rows <= 16384, "Fix: TileData::Rows must less than 16384 in L1");
    static_assert(
        TileData::Rows * TileData::Cols <= 512 * 1024, "Fix: TileData static shape must less than 512KB in L1");

    // ND2NZ or DN2NZ
    if constexpr ((GlobalData::layout == pto::Layout::ND || GlobalData::layout == pto::Layout::DN) &&
                  (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
        static_assert(TileData::SFractalSize == 512, "Fix: TileData SFractalSize must be 512 of NZ format in L1");
        static_assert(sizeof(typename TileData::DType) != 8, "Fix: DType not support b64 in ND2NZ or DN2NZ");
        // globaltensor only support 2 dim
        static_assert(
            GlobalData::staticShape[0] == 1 && GlobalData::staticShape[1] == 1 && GlobalData::staticShape[2] == 1,
            "Fix: GlobalTensor input shape now only support 2 dim");
        if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                      std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
            static_assert(GlobalData::layout != pto::Layout::DN &&
                              !(TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor)),
                "Fix: DN2NZ not support if input dtype is fp4");
        }
    }

    // NZ2NZ
    if constexpr ((GlobalData::layout == pto::Layout::NZ) &&
                  (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
        if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                      std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
            static_assert(BLOCK_BYTE_SIZE * 2 == GlobalData::staticShape[4] && BLOCK_LEN == GlobalData::staticShape[3],
                "Fix: Src GlobalTensor staticShape[3][4] must be satisfied with NZ format require!");
        } else {
            static_assert(BLOCK_BYTE_SIZE / sizeof(typename GlobalData::DType) == GlobalData::staticShape[4] &&
                              BLOCK_LEN == GlobalData::staticShape[3],
                "Fix: Src GlobalTensor staticShape[3][4] must be satisfied with NZ format require!");
        }
    }
}

template <typename TileData, typename GlobalData, Layout Layout = Layout::ND>
PTO_INTERNAL void TLoadCubeInstr(typename TileData::TileDType dst, typename GlobalData::DType *src,
    uint64_t loop1SrcStride, uint16_t nValue, uint32_t dValue) {
    if constexpr (Layout == Layout::ND) {
        if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                      std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
            copy_gm_to_cbuf_multi_nd2nz(reinterpret_cast<__cbuf__ uint8_t *>(dst),
                reinterpret_cast<__gm__ uint8_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        } else if constexpr (sizeof(typename TileData::DType) == 1) {
            copy_gm_to_cbuf_multi_nd2nz(reinterpret_cast<__cbuf__ uint8_t *>(dst),
                reinterpret_cast<__gm__ uint8_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        } else if constexpr (sizeof(typename TileData::DType) == 2) {
            copy_gm_to_cbuf_multi_nd2nz(reinterpret_cast<__cbuf__ uint16_t *>(dst),
                reinterpret_cast<__gm__ uint16_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        } else if constexpr (sizeof(typename TileData::DType) == 4) {
            copy_gm_to_cbuf_multi_nd2nz(reinterpret_cast<__cbuf__ uint32_t *>(dst),
                reinterpret_cast<__gm__ uint32_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        }
    } else {
        if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                      std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
            copy_gm_to_cbuf_multi_dn2nz(reinterpret_cast<__cbuf__ uint8_t *>(dst),
                reinterpret_cast<__gm__ uint8_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        } else if constexpr (sizeof(typename TileData::DType) == 1) {
            copy_gm_to_cbuf_multi_dn2nz(reinterpret_cast<__cbuf__ uint8_t *>(dst),
                reinterpret_cast<__gm__ uint8_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        } else if constexpr (sizeof(typename TileData::DType) == 2) {
            copy_gm_to_cbuf_multi_dn2nz(reinterpret_cast<__cbuf__ uint16_t *>(dst),
                reinterpret_cast<__gm__ uint16_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        } else if constexpr (sizeof(typename TileData::DType) == 4) {
            copy_gm_to_cbuf_multi_dn2nz(reinterpret_cast<__cbuf__ uint32_t *>(dst),
                reinterpret_cast<__gm__ uint32_t *>(src), 0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
        }
    }
}
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeInstr(typename TileData::TileDType dst, typename GlobalData::DType *src, uint32_t nBurst,
    uint32_t lenBurst, uint64_t srcStride, uint32_t dstStride, uint32_t padCount) {
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        copy_gm_to_cbuf_align_v2(reinterpret_cast<__cbuf__ uint8_t *>(dst), reinterpret_cast<__gm__ uint8_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, padCount /*right padding count*/,
            true /*data select bit*/, 0 /*l2 cache ctl*/, srcStride, dstStride);
    } else if constexpr (sizeof(typename TileData::DType) == 1) {
        copy_gm_to_cbuf_align_v2(reinterpret_cast<__cbuf__ uint8_t *>(dst), reinterpret_cast<__gm__ uint8_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, padCount /*right padding count*/,
            true /*data select bit*/, 0 /*l2 cache ctl*/, srcStride, dstStride);
    } else if constexpr (sizeof(typename TileData::DType) == 2) {
        copy_gm_to_cbuf_align_v2(reinterpret_cast<__cbuf__ uint16_t *>(dst), reinterpret_cast<__gm__ uint16_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, padCount /*right padding count*/,
            true /*data select bit*/, 0 /*l2 cache ctl*/, srcStride, dstStride);
    } else if constexpr (sizeof(typename TileData::DType) == 4) {
        copy_gm_to_cbuf_align_v2(reinterpret_cast<__cbuf__ uint32_t *>(dst), reinterpret_cast<__gm__ uint32_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, padCount /*right padding count*/,
            true /*data select bit*/, 0 /*l2 cache ctl*/, srcStride, dstStride);
    } else if constexpr (sizeof(typename TileData::DType) == 8) {
        copy_gm_to_cbuf_align_v2(reinterpret_cast<__cbuf__ uint32_t *>(dst), reinterpret_cast<__gm__ uint32_t *>(src),
            0 /*sid*/, nBurst, lenBurst, 0 /*left padding count*/, padCount * 2 /*right padding count*/,
            true /*data select bit*/, 0 /*l2 cache ctl*/, srcStride, dstStride);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeND2NZ(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint16_t nValue = gShape3;
    uint32_t dValue = validCol;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        dValue = dValue >> 1; // move fp4 as b8, need to be divided by 2
    }

    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride3);
    if constexpr (GlobalData::layout == pto::Layout::DN) {
        loop1SrcStride = GetByteSize<typename TileData::DType>(gStride4);
    }
    constexpr uint16_t ndNum = 1;
    uint16_t loop2DstStride = 1;
    uint16_t loop3DstStride = TileData::Rows;                          // unit is 32B
    uint16_t loop4DstStride = 0;                                       // because ndNum = 1
    uint64_t mte2NzPara = static_cast<uint64_t>(loop4DstStride) << 48; // MTE2_NZ_PARA[63:48]
    mte2NzPara |= static_cast<uint64_t>(loop3DstStride) << 32;         // MTE2_NZ_PARA[47:32]
    mte2NzPara |= static_cast<uint64_t>(loop2DstStride) << 16;         // MTE2_NZ_PARA[31:16]
    mte2NzPara |= static_cast<uint64_t>(ndNum);                        // MTE2_NZ_PARA[15:0]
    set_mte2_nz_para(mte2NzPara);                                      // only set once

    TLoadCubeInstr<TileData, GlobalData, GlobalData::layout>(dst, src, loop1SrcStride, nValue, dValue);
}
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeNZ2NZ(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    typename TileData::TileDType dstAddrP = dst;
    typename GlobalData::DType *srcAddrP = src;
    uint32_t nBurst = gShape1;
    uint32_t lenBurst = validRow * BLOCK_BYTE_SIZE;
    uint64_t gmStride = GetByteSize<typename TileData::DType>(gStride1);
    uint32_t dstStride = TileData::Rows * BLOCK_BYTE_SIZE;

    int64_t tileStride = gShape1 * TileData::Rows * gShape4;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        gStride0 = gStride0 >> 1;     // fp4 srcAddr offset need divide 2 as use b8 to move
        tileStride = tileStride >> 1; // fp4 dstAddr offset need divide 2 as use b8 to move
    }
    set_loop_size_outtol1(1ULL << 21 | 1ULL);
    for (uint32_t i = 0; i < gShape0; i++) {
        srcAddrP = src + i * gStride0;
        dstAddrP = dst + i * tileStride;
        TLoadCubeInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, dstStride, 0);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeND2ND(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    typename TileData::TileDType dstAddrP = dst;
    typename GlobalData::DType *srcAddrP = src;
    uint32_t nBurst = gShape3;
    uint32_t lenBurst = GetByteSize<typename TileData::DType>(validCol);
    uint64_t gmStride = GetByteSize<typename TileData::DType>(gStride3);
    uint32_t dstStride = GetByteSize<typename TileData::DType>(TileData::Cols);

    constexpr uint32_t blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileData::DType);
    uint32_t gapElement = (TileData::Cols - validCol);
    uint32_t padCount = gapElement % blockSizeElem;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        padCount = padCount >> 1;
    }
    set_pad_val_outtol1(GetPadValue<TileData>());

    int64_t dstStride2 = gShape3 * TileData::Cols;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;

    uint64_t loop2 = gShape1;
    uint64_t loop1 = gShape2;
    uint64_t loop2SrcStride = GetByteSize<typename TileData::DType>(gStride1);
    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride2);
    uint64_t loop2DstStride = GetByteSize<typename TileData::DType>(dstStride1);
    uint64_t loop1DstStride = GetByteSize<typename TileData::DType>(dstStride2);

    set_loop2_stride_outtol1(loop2DstStride << 40 | loop2SrcStride);
    set_loop1_stride_outtol1(loop1DstStride << 40 | loop1SrcStride);
    set_loop_size_outtol1(loop2 << 21 | loop1);
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        dstStride0 = dstStride0 >> 1; // fp4 dstAddr offset need divide 2 as use b8 to move
        gStride0 = gStride0 >> 1;     // fp4 srcAddr offset need divide 2 as use b8 to move
    }
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * dstStride0;
        int64_t srcAddr0 = i * gStride0;
        dstAddrP = dst + dstAddr0;
        srcAddrP = src + srcAddr0;
        TLoadCubeInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, dstStride, padCount);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeDN2DN(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    typename TileData::TileDType dstAddrP = dst;
    typename GlobalData::DType *srcAddrP = src;
    uint32_t nBurst = gShape4;
    uint32_t lenBurst = GetByteSize<typename TileData::DType>(validRow);
    uint64_t gmStride = GetByteSize<typename TileData::DType>(gStride4);
    uint32_t dstStride = GetByteSize<typename TileData::DType>(TileData::Rows);

    constexpr uint32_t blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileData::DType);
    uint32_t gapElement = (TileData::Rows - validRow);
    uint32_t padCount = gapElement % blockSizeElem;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        padCount = padCount >> 1;
    }
    set_pad_val_outtol1(GetPadValue<TileData>());

    int64_t dstStride2 = gShape4 * TileData::Rows;
    int64_t dstStride1 = gShape2 * dstStride2;
    int64_t dstStride0 = gShape1 * dstStride1;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        dstStride0 = dstStride0 >> 1; // fp4 dstAddr offset need divide 2 as use b8 to move
        gStride0 = gStride0 >> 1;     // fp4 srcAddr offset need divide 2 as use b8 to move
    }
    uint64_t loop2 = gShape1;
    uint64_t loop1 = gShape2;
    uint64_t loop2SrcStride = GetByteSize<typename TileData::DType>(gStride1);
    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride2);
    uint64_t loop2DstStride = GetByteSize<typename TileData::DType>(dstStride1);
    uint64_t loop1DstStride = GetByteSize<typename TileData::DType>(dstStride2);

    set_loop2_stride_outtol1(loop2DstStride << 40 | loop2SrcStride);
    set_loop1_stride_outtol1(loop1DstStride << 40 | loop1SrcStride);
    set_loop_size_outtol1(loop2 << 21 | loop1);
    for (uint32_t i = 0; i < gShape0; i++) {
        int64_t dstAddr0 = i * dstStride0;
        int64_t srcAddr0 = i * gStride0;
        dstAddrP = dst + dstAddr0;
        srcAddrP = src + srcAddr0;
        TLoadCubeInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, dstStride, padCount);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadCubeDN2ZN(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint16_t nValue = gShape4;
    uint32_t dValue = validRow;
    if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                  std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
        dValue = dValue >> 1; // move fp4 as b8, need to be divided by 2
    }

    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride4);

    constexpr uint16_t ndNum = 1;
    uint16_t loop2DstStride = 1;
    uint16_t loop3DstStride = TileData::Cols;                          // unit is 32B
    uint16_t loop4DstStride = 0;                                       // because ndNum = 1
    uint64_t mte2NzPara = static_cast<uint64_t>(loop4DstStride) << 48; // MTE2_NZ_PARA[63:48]
    mte2NzPara |= static_cast<uint64_t>(loop3DstStride) << 32;         // MTE2_NZ_PARA[47:32]
    mte2NzPara |= static_cast<uint64_t>(loop2DstStride) << 16;         // MTE2_NZ_PARA[31:16]
    mte2NzPara |= static_cast<uint64_t>(ndNum);                        // MTE2_NZ_PARA[15:0]
    set_mte2_nz_para(mte2NzPara);                                      // only set once

    // use nd2nz
    TLoadCubeInstr<TileData, GlobalData, pto::Layout::ND>(dst, src, loop1SrcStride, nValue, dValue);
}

template <typename TileData, typename GlobalData>
__tf__ PTO_INTERNAL void TLoadCube(typename TileData::TileDType __out__ dst, typename GlobalData::DType __in__ *src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
#if defined(__DAV_CUBE__)
    using L1Type = typename TileData::TileDType;
    L1Type dstAddr = (L1Type)__cce_get_tile_ptr(dst);

    // ND2NZ or DN2NZ
    if constexpr ((GlobalData::layout == pto::Layout::ND || GlobalData::layout == pto::Layout::DN) &&
                  (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
        TLoadCubeND2NZ<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);

    } else if constexpr ((GlobalData::layout == pto::Layout::NZ) &&
                         (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
        TLoadCubeNZ2NZ<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr ((GlobalData::layout == pto::Layout::ND &&
                             (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)))) {
        // ND2ND support cols padding
        TLoadCubeND2ND<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::DN &&
                         (!TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox))) {
        // dn support rows padding
        TLoadCubeDN2DN<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::DN &&
                         (TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor))) {
        TLoadCubeDN2ZN<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4, validRow, validCol);
    }
#endif
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL constexpr bool IsScale() {
    if constexpr (GlobalData::layout == pto::Layout::MX_A_ND || GlobalData::layout == pto::Layout::MX_A_DN ||
                  GlobalData::layout == pto::Layout::MX_A_ZZ || GlobalData::layout == pto::Layout::MX_B_ND ||
                  GlobalData::layout == pto::Layout::MX_B_DN || GlobalData::layout == pto::Layout::MX_B_NN) {
        return true;
    }
    return false;
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeCheck() {
    // support ZZ2ZZ NN2NN
    static_assert(((GlobalData::layout == pto::Layout::MX_A_ZZ || GlobalData::layout == pto::Layout::MX_A_ND ||
                       GlobalData::layout == pto::Layout::MX_A_DN) &&
                      (TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) ||
                      ((GlobalData::layout == pto::Layout::MX_B_NN || GlobalData::layout == pto::Layout::MX_B_ND ||
                           GlobalData::layout == pto::Layout::MX_B_DN) &&
                          (!TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor))),
        "Fix: now only support MX_A_ZZ2ZZ/MX_A_ND2ZZ/MX_A_DN2ZZ or MX_B_NN2NN/MX_B_ND2NN/MX_B_DN2NN in current "
        "platform");

    static_assert(std::is_same<typename TileData::DType, float8_e8m0_t>::value &&
                      std::is_same<typename GlobalData::DType, __gm__ float8_e8m0_t>::value,
        "Fix: DType only support float8_e8m0_t in MX_A_ZZ or MX_B_NN");
    static_assert(TileData::SFractalSize == 32, "Fix: TileData SFractalSize must be 32 of Zz or Nn format in L1");

    // L1 space check
    static_assert(
        TileData::Rows * TileData::Cols <= 512 * 1024, "Fix: TileData static shape must less than 512KB in L1");
    // ZZ2ZZ and NN2NN check SFractal shape
    if constexpr (GlobalData::layout == pto::Layout::MX_A_ZZ || GlobalData::layout == pto::Layout::MX_B_NN) {
        // globaltensor only support [16,2] fractal
        static_assert((GlobalData::staticShape[3] == 16 || GlobalData::staticShape[3] == -1) &&
                      (GlobalData::staticShape[4] == 2 || GlobalData::staticShape[4] == -1),
            "Fix: GlobalTensor input SFractal is [16,2] when Layout is MX_AZZ or MX_BNN");
    }
    // check shape
    if constexpr (GlobalData::layout == pto::Layout::MX_A_ZZ) {
        static_assert(
            (TileData::Rows >= GlobalData::staticShape[0] * GlobalData::staticShape[1] * GlobalData::staticShape[3]) &&
                (TileData::Cols >= GlobalData::staticShape[2] * GlobalData::staticShape[4]),
            "Fix: TileData::Rows need >= GlobalTensor inputShape[0] * inputShape[1] * inputShape[3] and TileData::Cols "
            "need "
            ">= GlobalTensor inputShape[2] * inputShape[4], when Layout is MX_A_ZZ");
    }

    if constexpr (GlobalData::layout == pto::Layout::MX_B_NN) {
        static_assert((TileData::Rows >= GlobalData::staticShape[2] * GlobalData::staticShape[4]) &&
                          (TileData::Cols >=
                              GlobalData::staticShape[0] * GlobalData::staticShape[1] * GlobalData::staticShape[3]),
            "Fix: TileData::Rows need >= GlobalTensor inputShape[2] * inputShape[4] and TileData::Cols need "
            ">= GlobalTensor inputShape[0] * inputShape[1] * inputShape[3], when Layout is MX_B_NN");
    }

    if constexpr (GlobalData::layout == pto::Layout::MX_A_ND || GlobalData::layout == pto::Layout::MX_A_DN ||
                  GlobalData::layout == pto::Layout::MX_B_ND || GlobalData::layout == pto::Layout::MX_B_DN) {
        static_assert(((GlobalData::staticShape[0] == 1 || GlobalData::staticShape[0] == -1) &&
            (GlobalData::staticShape[1] == 1 || GlobalData::staticShape[1] == -1)),
            "expect 3D for layout MX_A_ND/MX_A_DN/MX_B_ND/MX_B_DN.");
        static_assert((GlobalData::staticShape[4] == 2 || GlobalData::staticShape[4] == -1),
            "expect gShape4 == 2 for layout MX_A_ND/MX_A_DN/MX_B_ND/MX_B_DN.");
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeNN2NN(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4) {
    // [0   1       2      3   4]
    // [1, N/16, scaleK/2, 16, 2]
    typename TileData::TileDType dstAddrP = dst;
    typename GlobalData::DType *srcAddrP = src;
    uint32_t nBurst = gShape1;
    uint32_t lenBurst = gShape2 * gShape4 * BLOCK_LEN;
    uint64_t gmStride = gStride1;
    uint32_t dstStride = BLOCK_LEN * TileData::Rows;

    int64_t tileStride = TileData::Rows * gShape1 * gShape3; // stitching along the col direction
    set_loop_size_outtol1(1ULL << 21 | 1ULL);
    for (uint32_t i = 0; i < gShape0; i++) {
        srcAddrP = src + i * gStride0;
        dstAddrP = dst + i * tileStride;
        TLoadCubeInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, dstStride, 0);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeZZ2ZZ(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4) {
    // [0   1       2      3   4]
    // [1, M/16, scaleK/2, 16, 2]
    typename TileData::TileDType dstAddrP = dst;
    typename GlobalData::DType *srcAddrP = src;
    uint32_t nBurst = gShape1;
    uint32_t lenBurst = BLOCK_LEN * gShape2 * gShape4;
    uint64_t gmStride = gStride1;
    uint32_t dstStride = BLOCK_LEN * TileData::Cols;

    int64_t tileStride = gShape1 * gShape3 * TileData::Cols; // stitching along the row direction
    set_loop_size_outtol1(1ULL << 21 | 1ULL);
    for (uint32_t i = 0; i < gShape0; i++) {
        srcAddrP = src + i * gStride0;
        dstAddrP = dst + i * tileStride;
        TLoadCubeInstr<TileData, GlobalData>(dstAddrP, srcAddrP, nBurst, lenBurst, gmStride, dstStride, 0);
    }
}

// DN for AND2ZZ && BDN2NN
// ND for ADN2ZZ && BND2NN
template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeAND2ZZ(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint16_t nValue = validCol >> 1;
    uint32_t dValue = validRow;

    constexpr uint16_t loop3DstStride = TileData::Cols >> 1; // unit is 32B
    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride3);

    // MTE2_NZ_PARA[63:48]  loop4DstStride,  MTE2_NZ_PARA[47:32]  loop3DstStride
    // MTE2_NZ_PARA[31:16]  loop2DstStride   MTE2_NZ_PARA[15:0]   ndNum
    uint64_t mte2NzPara = static_cast<uint64_t>(0) << 48 | static_cast<uint64_t>(loop3DstStride) << 32;
    mte2NzPara |= static_cast<uint64_t>(1) << 16 | static_cast<uint64_t>(1);
    set_mte2_nz_para(mte2NzPara); // only set once

    copy_gm_to_cbuf_multi_dn2nz(reinterpret_cast<__cbuf__ uint16_t *>(dst), reinterpret_cast<__gm__ uint16_t *>(src),
        0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeADN2ZZ(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint16_t nValue = validCol >> 1;
    uint32_t dValue = validRow;

    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride4) * sizeof(uint16_t);
    constexpr uint16_t loop3DstStride = TileData::Cols >> 1; // unit is 32B

    // MTE2_NZ_PARA[63:48]  loop4DstStride,  MTE2_NZ_PARA[47:32]  loop3DstStride
    // MTE2_NZ_PARA[31:16]  loop2DstStride   MTE2_NZ_PARA[15:0]   ndNum
    uint64_t mte2NzPara = static_cast<uint64_t>(0) << 48 | static_cast<uint64_t>(loop3DstStride) << 32;
    mte2NzPara |= static_cast<uint64_t>(1) << 16 | static_cast<uint64_t>(1);
    set_mte2_nz_para(mte2NzPara); // only set once

    copy_gm_to_cbuf_multi_nd2nz(reinterpret_cast<__cbuf__ uint16_t *>(dst), reinterpret_cast<__gm__ uint16_t *>(src),
        0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeBND2NN(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint16_t nValue = validRow >> 1;
    uint32_t dValue = validCol;

    constexpr uint16_t loop3DstStride = TileData::Rows >> 1; // unit is 32B
    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride3) * sizeof(uint16_t);

    // MTE2_NZ_PARA[63:48]  loop4DstStride,  MTE2_NZ_PARA[47:32]  loop3DstStride
    // MTE2_NZ_PARA[31:16]  loop2DstStride   MTE2_NZ_PARA[15:0]   ndNum
    uint64_t mte2NzPara = static_cast<uint64_t>(0) << 48 | static_cast<uint64_t>(loop3DstStride) << 32;
    mte2NzPara |= static_cast<uint64_t>(1) << 16 | static_cast<uint64_t>(1);
    set_mte2_nz_para(mte2NzPara); // only set once

    copy_gm_to_cbuf_multi_nd2nz(reinterpret_cast<__cbuf__ uint16_t *>(dst), reinterpret_cast<__gm__ uint16_t *>(src),
        0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLoadMxCubeBDN2NN(typename TileData::TileDType dst, typename GlobalData::DType *src, int gShape0,
    int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2, int gStride3,
    int gStride4, int validRow, int validCol) {
    uint16_t nValue = validRow >> 1;
    uint32_t dValue = validCol;

    uint64_t loop1SrcStride = GetByteSize<typename TileData::DType>(gStride4);
    constexpr uint16_t loop3DstStride = TileData::Rows >> 1; // unit is 32B

    // MTE2_NZ_PARA[63:48]  loop4DstStride,  MTE2_NZ_PARA[47:32]  loop3DstStride
    // MTE2_NZ_PARA[31:16]  loop2DstStride   MTE2_NZ_PARA[15:0]   ndNum
    uint64_t mte2NzPara = static_cast<uint64_t>(0) << 48 | static_cast<uint64_t>(loop3DstStride) << 32;
    mte2NzPara |= static_cast<uint64_t>(1) << 16 | static_cast<uint64_t>(1);
    set_mte2_nz_para(mte2NzPara); // only set once

    copy_gm_to_cbuf_multi_dn2nz(reinterpret_cast<__cbuf__ uint16_t *>(dst), reinterpret_cast<__gm__ uint16_t *>(src),
        0 /*sid*/, loop1SrcStride, 0, nValue, dValue, 0, false);
}

template <typename TileData, typename GlobalData>
__tf__ PTO_INTERNAL void TLoadMxCube(typename TileData::TileDType __out__ dst, typename GlobalData::DType __in__ *src,
    int gShape0, int gShape1, int gShape2, int gShape3, int gShape4, int gStride0, int gStride1, int gStride2,
    int gStride3, int gStride4, int validRow, int validCol) {
    using L1Type = typename TileData::TileDType;
    L1Type dstAddr = (L1Type)__cce_get_tile_ptr(dst);
    // ZZ2ZZ or NN2NN
    if constexpr (GlobalData::layout == pto::Layout::MX_A_ZZ &&
                  (TileData::isRowMajor && TileData::SFractal == SLayout::RowMajor)) {
        TLoadMxCubeZZ2ZZ<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4);
    } else if constexpr (GlobalData::layout == pto::Layout::MX_B_NN &&
                         (!TileData::isRowMajor && TileData::SFractal == SLayout::ColMajor)) {
        TLoadMxCubeNN2NN<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride4);
    } else if constexpr (GlobalData::layout == pto::Layout::MX_A_ND &&
                         (TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
        // newgStride3 -> gStride2;
        TLoadMxCubeAND2ZZ<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride2, gStride4, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::MX_A_DN &&
                         (TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
        // newgStride4 -> gStride2 / gStride3;
        TLoadMxCubeADN2ZZ<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride2 / gStride3, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::MX_B_ND &&
                         (!TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor))) {
        // newgStride3 -> gStride2 / gStride3;
        TLoadMxCubeBND2NN<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride2 / gStride3, gStride4, validRow, validCol);
    } else if constexpr (GlobalData::layout == pto::Layout::MX_B_DN &&
                         (!TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor))) {
        // newgStride4 -> gStride2;
        TLoadMxCubeBDN2NN<TileData, GlobalData>(dstAddr, src, gShape0, gShape1, gShape2, gShape3, gShape4, gStride0,
            gStride1, gStride2, gStride3, gStride2, validRow, validCol);
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void StaticCheck() {
    static_assert((sizeof(typename TileData::DType) == 1) || (sizeof(typename TileData::DType) == 2) ||
                      (sizeof(typename TileData::DType) == 4) || (sizeof(typename TileData::DType) == 8),
        "Fix: Data type must be b8/b16/b32/b64");
    if constexpr (std::is_same<typename TileData::DType, int64_t>::value ||
                  std::is_same<typename TileData::DType, uint64_t>::value) {
        static_assert(TileData::PadVal == PadValue::Null || TileData::PadVal == PadValue::Zero,
            "Fix: TileData::PadVal only support Null or Zero in B64 mode");
    }
    static_assert(sizeof(typename TileData::DType) == sizeof(typename GlobalData::DType),
        "Fix: Source dtype must be same with dst dtype!");

    // for static shape case, enforce the global tensor (tiled) shape matching with vecTile valid shape for xfer
    if constexpr (TileData::Loc == pto::TileType::Vec) {
        static_assert(((GlobalData::layout == pto::Layout::ND) &&
                          (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox))) ||
                          ((GlobalData::layout == pto::Layout::DN) &&
                              (!TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox))) ||
                          ((GlobalData::layout == pto::Layout::NZ) &&
                              (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))),
            "Src and dst layout must be same!");
        if constexpr (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)) {
            if constexpr (TileData::ValidCol > 0 && GlobalData::staticShape[4] > 0) {
                static_assert(TileData::ValidCol == GlobalData::staticShape[4],
                    "Fix: Src GlobalTensor Col and Tile ValidCol must be the same!");
            }
            if constexpr (TileData::ValidRow > 0 && GlobalData::staticShape[0] > 0 && GlobalData::staticShape[1] > 0 &&
                          GlobalData::staticShape[2] > 0 && GlobalData::staticShape[3] > 0) {
                constexpr const int mergedRows = GlobalData::staticShape[0] * GlobalData::staticShape[1] *
                                                 GlobalData::staticShape[2] * GlobalData::staticShape[3];
                static_assert(TileData::ValidRow == mergedRows,
                    "Fix: Src GlobalTensor Row Products and Tile ValidRow must be the same!");
            }
        }
        if constexpr ((GlobalData::layout == pto::Layout::NZ) &&
                      (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor))) {
            if constexpr (std::is_same<typename TileData::DType, float4_e1m2x2_t>::value ||
                          std::is_same<typename TileData::DType, float4_e2m1x2_t>::value) {
                static_assert(
                    BLOCK_BYTE_SIZE * 2 == GlobalData::staticShape[4] && BLOCK_LEN == GlobalData::staticShape[3],
                    "Fix: Src GlobalTensor staticShape[3][4] must be satisfied with NZ format require!");
            } else {
                static_assert(BLOCK_BYTE_SIZE / sizeof(typename GlobalData::DType) == GlobalData::staticShape[4] &&
                                  BLOCK_LEN == GlobalData::staticShape[3],
                    "Fix: Src GlobalTensor staticShape[3][4] must be satisfied with NZ format require!");
            }
        }
    }
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TLOAD_IMPL(TileData &dst, GlobalData &src) {
    StaticCheck<TileData, GlobalData>();
    if constexpr (TileData::Loc == pto::TileType::Vec) {
        TLoad<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(pto::GlobalTensorDim::DIM_0),
            src.GetShape(pto::GlobalTensorDim::DIM_1), src.GetShape(pto::GlobalTensorDim::DIM_2),
            src.GetShape(pto::GlobalTensorDim::DIM_3), src.GetShape(pto::GlobalTensorDim::DIM_4),
            src.GetStride(pto::GlobalTensorDim::DIM_0), src.GetStride(pto::GlobalTensorDim::DIM_1),
            src.GetStride(pto::GlobalTensorDim::DIM_2), src.GetStride(pto::GlobalTensorDim::DIM_3),
            src.GetStride(pto::GlobalTensorDim::DIM_4), dst.GetValidRow(), dst.GetValidCol());
    } else if constexpr (TileData::Loc == pto::TileType::Mat) {
        if constexpr (!IsScale<TileData, GlobalData>()) {
            TLoadCubeCheck<TileData, GlobalData>();
            TLoadCube<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(pto::GlobalTensorDim::DIM_0),
                src.GetShape(pto::GlobalTensorDim::DIM_1), src.GetShape(pto::GlobalTensorDim::DIM_2),
                src.GetShape(pto::GlobalTensorDim::DIM_3), src.GetShape(pto::GlobalTensorDim::DIM_4),
                src.GetStride(pto::GlobalTensorDim::DIM_0), src.GetStride(pto::GlobalTensorDim::DIM_1),
                src.GetStride(pto::GlobalTensorDim::DIM_2), src.GetStride(pto::GlobalTensorDim::DIM_3),
                src.GetStride(pto::GlobalTensorDim::DIM_4), dst.GetValidRow(), dst.GetValidCol());
        } else if constexpr (IsScale<TileData, GlobalData>()) {
            TLoadMxCubeCheck<TileData, GlobalData>();
            TLoadMxCube<TileData, GlobalData>(dst.data(), src.data(), src.GetShape(0), src.GetShape(1), src.GetShape(2),
                src.GetShape(3), src.GetShape(4), src.GetStride(0), src.GetStride(1), src.GetStride(2),
                src.GetStride(3), src.GetStride(4), dst.GetValidRow(), dst.GetValidCol());
        }
    }
}
} // namespace pto
#endif // TLOAD_HPP
