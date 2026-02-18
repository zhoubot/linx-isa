/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TGATHERB_HPP
#define TGATHERB_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>

namespace pto {

template <typename T>
struct GatherB {
    PTO_INTERNAL static void GatherBInstr(
        __ubuf__ T *dst, __ubuf__ uint32_t *offset, uint32_t srcAddr, uint16_t dstRepeatStride, uint8_t repeats) {
        vgatherb((__ubuf__ T *)dst, offset, srcAddr, dstRepeatStride, 1, repeats);
    }

    PTO_INTERNAL static void GatherBInstrB8(__ubuf__ uint16_t *dst, __ubuf__ uint32_t *offset, uint32_t srcAddr,
        uint16_t dstRepeatStride, uint8_t repeats) {
        vgatherb((__ubuf__ uint16_t *)dst, offset, srcAddr, dstRepeatStride, 1, repeats);
    }
};

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned offsetRowStride>
PTO_INTERNAL void GatherBlockHead(
    __ubuf__ T *dstPtr, __ubuf__ uint32_t *offsetPtr, uint32_t &srcAddr, unsigned validRow, unsigned numRepeatPerLine) {
    unsigned numLoop = numRepeatPerLine / REPEAT_MAX;
    unsigned remainAfterLoop = numRepeatPerLine % REPEAT_MAX;
    for (int i = 0; i < validRow; i++) {
        if (numLoop) {
            for (int j = 0; j < numLoop; j++) {
                if constexpr (sizeof(T) == 1) {
                    Op::GatherBInstrB8(
                        (__ubuf__ uint16_t *)(dstPtr + i * dstRowStride + j * elementsPerRepeat * REPEAT_MAX),
                        offsetPtr + i * offsetRowStride + j * 8 * REPEAT_MAX, srcAddr, 8, REPEAT_MAX);
                } else {
                    Op::GatherBInstr((__ubuf__ T *)(dstPtr + i * dstRowStride + j * elementsPerRepeat * REPEAT_MAX),
                        offsetPtr + i * offsetRowStride + j * 8 * REPEAT_MAX, srcAddr, 8, REPEAT_MAX);
                }
            }
        }
        if (remainAfterLoop) {
            if constexpr (sizeof(T) == 1) {
                Op::GatherBInstrB8(
                    (__ubuf__ uint16_t *)(dstPtr + i * dstRowStride + numLoop * elementsPerRepeat * REPEAT_MAX),
                    offsetPtr + i * offsetRowStride + numLoop * 8 * REPEAT_MAX, srcAddr, 8, remainAfterLoop);
            } else {
                Op::GatherBInstr((__ubuf__ T *)(dstPtr + i * dstRowStride + numLoop * elementsPerRepeat * REPEAT_MAX),
                    offsetPtr + i * offsetRowStride + numLoop * 8 * REPEAT_MAX, srcAddr, 8, remainAfterLoop);
            }
        }
    }
}

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned offsetRowStride>
PTO_INTERNAL void GatherBlockTail(
    __ubuf__ T *dstPtr, __ubuf__ uint32_t *offsetPtr, uint32_t &srcAddr, unsigned validRow, unsigned validCol) {
    unsigned numLoop = validRow / REPEAT_MAX;
    unsigned remainAfterLoop = validRow % REPEAT_MAX;
    if (numLoop) {
        for (int i = 0; i < numLoop; i++) {
            if constexpr (sizeof(T) == 1) {
                Op::GatherBInstrB8((__ubuf__ uint16_t *)(dstPtr + i * REPEAT_MAX * dstRowStride),
                    offsetPtr + i * REPEAT_MAX * offsetRowStride, srcAddr, dstRowStride / blockSizeElem, REPEAT_MAX);
            } else {
                Op::GatherBInstr((__ubuf__ T *)(dstPtr + i * REPEAT_MAX * dstRowStride),
                    offsetPtr + i * REPEAT_MAX * offsetRowStride, srcAddr, dstRowStride / blockSizeElem, REPEAT_MAX);
            }
        }
    }
    if (remainAfterLoop) {
        if constexpr (sizeof(T) == 1) {
            Op::GatherBInstrB8((__ubuf__ uint16_t *)(dstPtr + numLoop * REPEAT_MAX * dstRowStride),
                offsetPtr + numLoop * REPEAT_MAX * offsetRowStride, srcAddr, dstRowStride / blockSizeElem,
                remainAfterLoop);
        } else {
            Op::GatherBInstr((__ubuf__ T *)(dstPtr + numLoop * REPEAT_MAX * dstRowStride),
                offsetPtr + numLoop * REPEAT_MAX * offsetRowStride, srcAddr, dstRowStride / blockSizeElem,
                remainAfterLoop);
        }
    }
}

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
    unsigned offsetRowStride>
PTO_INTERNAL void TGatherBlock(
    __ubuf__ T *dstPtr, __ubuf__ uint32_t *offsetPtr, uint32_t &srcAddr, unsigned validRow, unsigned validCol) {
    unsigned numRepeatPerLine = validCol / elementsPerRepeat;
    unsigned numRemainPerLine = validCol % elementsPerRepeat;
    if (numRepeatPerLine > 0) {
        GatherBlockHead<Op, T, elementsPerRepeat, blockSizeElem, dstRowStride, offsetRowStride>(
            dstPtr, offsetPtr, srcAddr, validRow, numRepeatPerLine);
    }

    dstPtr += numRepeatPerLine * elementsPerRepeat;
    offsetPtr += numRepeatPerLine * 8;
    if (numRemainPerLine) {
        GatherBlockTail<Op, T, elementsPerRepeat, blockSizeElem, dstRowStride, offsetRowStride>(
            dstPtr, offsetPtr, srcAddr, validRow, validCol);
    }
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset, unsigned elementsPerRepeat,
    unsigned blockSizeElem, unsigned dstRowStride, unsigned offsetRowStride>
__tf__ PTO_INTERNAL void TGatherB(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, typename TileDataOffset::TileDType __in__ offset, unsigned validRow,
    unsigned validCol) {
    __ubuf__ uint32_t *offsetPtr = (__ubuf__ uint32_t *)__cce_get_tile_ptr(offset);
    uint32_t srcAddr = (uint64_t)(__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src);
    if constexpr (sizeof(typename TileDataDst::DType) == 4 || sizeof(typename TileDataDst::DType) == 2) {
        using T = typename std::conditional<sizeof(typename TileDataDst::DType) == 4, uint32_t, uint16_t>::type;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        TGatherBlock<GatherB<T>, T, elementsPerRepeat, blockSizeElem, dstRowStride, offsetRowStride>(
            dstPtr, offsetPtr, srcAddr, validRow, validCol);
    } else if constexpr (sizeof(typename TileDataDst::DType) == 1) {
        using T = typename TileDataDst::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        TGatherBlock<GatherB<T>, T, elementsPerRepeat, blockSizeElem, dstRowStride, offsetRowStride>(
            dstPtr, offsetPtr, srcAddr, validRow, validCol);
    } else {
        static_assert(sizeof(typename TileDataDst::DType) == 4 || sizeof(typename TileDataDst::DType) == 2 ||
                          sizeof(typename TileDataDst::DType) == 1,
            "Fix: TGATHERB has invalid data type.");
    }
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset>
PTO_INTERNAL void TGATHERB_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataOffset &offset) {
    static_assert(TileDataDst::isRowMajor, "Fix: TGATHERB has not supported Layout type.");
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataDst::DType);
    constexpr unsigned dstRowStride = TileDataDst::RowStride;
    constexpr unsigned offsetRowStride = TileDataOffset::RowStride;
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();

    TGatherB<TileDataDst, TileDataSrc, TileDataOffset, elementsPerRepeat, blockSizeElem, dstRowStride, offsetRowStride>(
        dst.data(), src.data(), offset.data(), validRow, validCol);
}
} // namespace pto

#endif