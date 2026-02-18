/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TFILLPAD_HPP
#define TFILLPAD_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "TLoad.hpp"

namespace pto {
template <typename TileData>
AICORE constexpr auto getCopyNullPtr() {
    using T = typename TileData::DType;
    if constexpr (sizeof(T) == 4) {
        return (__ubuf__ uint32_t *)0;
    } else if constexpr (sizeof(T) == 2) {
        return (__ubuf__ uint16_t *)0;
    } else if constexpr (sizeof(T) == 1) {
        return (__ubuf__ uint16_t *)0;
    } else {
        static_assert(sizeof(T) < 0, "Fix: TFILLPAD has unsupported DType for PadValue!");
    }
}

template <typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TFillPad_CopyData(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, uint64_t dstValidRow, uint64_t dstValidCol, uint64_t srcValidRow,
    uint64_t srcValidCol) {
    set_mask_count(); // counter mode
    using T = typename TileDataSrc::DType;
    auto srcPtr = getCopyNullPtr<TileDataSrc>();
    auto dstPtr = getCopyNullPtr<TileDataDst>();
    srcPtr = (decltype(srcPtr))__cce_get_tile_ptr(src);
    dstPtr = (decltype(dstPtr))__cce_get_tile_ptr(dst);
    constexpr uint64_t copySrcCols = (sizeof(T) == 1) ? TileDataSrc::Cols / 2 : TileDataSrc::Cols;
    constexpr uint64_t copyDstCols = (sizeof(T) == 1) ? TileDataDst::Cols / 2 : TileDataDst::Cols;

    set_vector_mask(0, copySrcCols);

    uint64_t srcCopyRow = srcValidRow;
    auto _srcPtr = srcPtr;
    auto _dstPtr = dstPtr;
    if constexpr (TileDataSrc::Rows > REPEAT_MAX) {
        while (srcCopyRow > REPEAT_MAX) {
            uint8_t repeat = REPEAT_MAX;
            uint16_t srcRepeatStride = TileDataSrc::Cols * sizeof(T) / 32;
            uint16_t dstRepeatStride = TileDataDst::Cols * sizeof(T) / 32;
            vcopy(_dstPtr, _srcPtr, repeat, 1, 1, dstRepeatStride, srcRepeatStride);
            srcCopyRow -= REPEAT_MAX;
            _srcPtr += REPEAT_MAX * copySrcCols;
            _dstPtr += REPEAT_MAX * copyDstCols;
        }
    }
    uint8_t repeat = srcCopyRow;
    uint16_t srcRepeatStride = TileDataSrc::Cols * sizeof(T) / 32;
    uint16_t dstRepeatStride = TileDataDst::Cols * sizeof(T) / 32;
    vcopy(_dstPtr, _srcPtr, repeat, 1, 1, dstRepeatStride, srcRepeatStride);
}

template <typename T>
PTO_INTERNAL uint64_t getPadMask(uint64_t validCol) {
    if constexpr (sizeof(T) == 4) {
        return 0;
    } else if constexpr (sizeof(T) == 2) {
        return 0;
    } else if constexpr (sizeof(T) == 1) {
        return 0;
    } else {
        static_assert(sizeof(T) < 0, "Fix: TFILLPAD has unsupported DType for PadValue!");
    }
}

// Helper: handle 32B-aligned padding for byte-sized elements (sizeof==1)
template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void Handle32BAlignedPad_Byte(decltype(getCopyNullPtr<TileDataDst>()) dstPtr, uint64_t srcValidRow,
    uint64_t srcValidCol, uint64_t /* srcValidCol32B */, decltype(GetPadValue<TileDataDst>()) padValue) {
    using T = typename TileDataSrc::DType;
    uint64_t pad_32B = 32 / sizeof(T) - srcValidCol;
    PtoSetWaitFlag<PIPE_V, PIPE_S>();
    using TP = decltype(padValue);
    for (uint64_t r = 0; r < srcValidRow; r++) {
        __ubuf__ TP *dstPadPtr = &((__ubuf__ TP *)dstPtr)[r * TileDataDst::Cols + srcValidCol];
        for (uint64_t p = 0; p < pad_32B; p++) {
            *(dstPadPtr++) = padValue;
        }
    }
    dsb(DSB_UB);
}

// Helper: handle 32B-aligned padding for non-byte elements (sizeof==2 or 4)
template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void Handle32BAlignedPad_Other(decltype(getCopyNullPtr<TileDataDst>()) dstPtr, uint64_t srcValidRow,
    uint64_t srcValidCol, uint64_t srcValidCol32B, decltype(GetPadValue<TileDataDst>()) padValue) {
    using T = typename TileDataSrc::DType;
    uint64_t elements_per_block = (sizeof(T) == 1) ? 16 : 32 / sizeof(T);
    uint64_t pad_32B = srcValidCol32B - srcValidCol;
    set_mask_norm();
    uint64_t mask = 0;
    uint16_t dstRepeatStride = TileDataDst::Cols * sizeof(T) / 32;
    if constexpr (sizeof(T) == 4)
        mask = 0xffULL;
    else
        mask = 0xffffULL;
    mask = mask >> (elements_per_block - pad_32B);
    mask = mask << (elements_per_block - pad_32B);
    set_vector_mask(0, mask);

    uint64_t fillRow = srcValidRow;
    auto _dstPtr = dstPtr + (srcValidCol32B - elements_per_block);
    if constexpr (TileDataSrc::Rows > REPEAT_MAX) {
        vector_dup(_dstPtr, padValue, REPEAT_MAX, 1, 1, dstRepeatStride, 0);
        _dstPtr += REPEAT_MAX * TileDataDst::Cols;
        fillRow -= REPEAT_MAX;
    }
    if (fillRow) {
        vector_dup(_dstPtr, padValue, fillRow, 1, 1, dstRepeatStride, 0);
    }
    pipe_barrier(PIPE_V);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void PadRightSingleRow(
    decltype(getCopyNullPtr<TileDataDst>()) dstPtr, uint64_t padOffset, uint64_t padCols, uint64_t dupPadValue) {
    set_mask_count(); // counter mode
    set_vector_mask(0, padCols);
    vector_dup(dstPtr + padOffset, dupPadValue, 1, 1, 1, 8, 0);
    pipe_barrier(PIPE_V);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void PadRightRemainingRows(
    decltype(getCopyNullPtr<TileDataDst>()) dstPtr, uint64_t padOffset, uint64_t copyDstCols, uint64_t srcValidRow) {
    using T = typename TileDataSrc::DType;
    uint16_t dstRepeatStride = TileDataDst::Cols * sizeof(T) / 32;
    auto _dstPtr = dstPtr + padOffset + copyDstCols;
    uint64_t fillRow = (srcValidRow > 0) ? srcValidRow - 1 : 0;

    if constexpr (TileDataSrc::Rows > REPEAT_MAX) {
        while (fillRow > REPEAT_MAX) {
            uint8_t repeat = REPEAT_MAX;
            vcopy(_dstPtr, dstPtr + padOffset, repeat, 1, 0, dstRepeatStride, 0);
            _dstPtr += REPEAT_MAX * copyDstCols;
            fillRow -= REPEAT_MAX;
        }
    }
    uint8_t repeat = static_cast<uint8_t>(fillRow);
    if (repeat) {
        vcopy(_dstPtr, dstPtr + padOffset, repeat, 1, 0, dstRepeatStride, 0);
    }
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void PadBottomRows(decltype(getCopyNullPtr<TileDataDst>()) dstPtr, uint64_t srcValidRow,
    uint64_t dstValidRow, uint64_t copyDstCols, uint64_t dupPadValue) {
    int padRows = static_cast<int>(dstValidRow) - static_cast<int>(srcValidRow);
    if (padRows <= 0)
        return;
    set_vector_mask(0, static_cast<uint64_t>(padRows) * copyDstCols);
    vector_dup(dstPtr + srcValidRow * copyDstCols, dupPadValue, 1, 1, 1, 8, 0);
}

template <typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TFillPad(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, uint64_t dstValidRow, uint64_t dstValidCol, uint64_t srcValidRow,
    uint64_t srcValidCol) {
    using T = typename TileDataSrc::DType;
    auto srcPtr = getCopyNullPtr<TileDataSrc>();
    auto dstPtr = getCopyNullPtr<TileDataDst>();
    srcPtr = (decltype(srcPtr))__cce_get_tile_ptr(src);
    dstPtr = (decltype(dstPtr))__cce_get_tile_ptr(dst);
    auto padValue = GetPadValue<TileDataDst>();

    constexpr const uint64_t copyDstCols = sizeof(T) == 1 ? TileDataDst::Cols / 2 : TileDataDst::Cols;
    uint64_t elements_per_block = (sizeof(T) == 1) ? 16 : 32 / sizeof(T);
    uint64_t srcValidCol32B = (sizeof(T) == 1) ?
                                  CeilDivision(CeilDivision(srcValidCol, 2), elements_per_block) * elements_per_block :
                                  CeilDivision(srcValidCol, elements_per_block) * elements_per_block;
    uint64_t padOffset = srcValidCol32B;
    uint64_t padCols = copyDstCols - srcValidCol32B;

    // handle 32B-aligned padding (was inlined previously)
    if constexpr (TileDataDst::PadVal != TileDataSrc::PadVal) {
        if constexpr (sizeof(T) == 1) {
            Handle32BAlignedPad_Byte<TileDataDst, TileDataSrc>(
                dstPtr, srcValidRow, srcValidCol, srcValidCol32B, padValue);
        } else {
            Handle32BAlignedPad_Other<TileDataDst, TileDataSrc>(
                dstPtr, srcValidRow, srcValidCol, srcValidCol32B, padValue);
        }
    }

    uint64_t dupPadValue = sizeof(T) == 1 ? ((uint64_t)padValue) << 8 | ((uint64_t)padValue) : padValue;

    // pad right for single row
    PadRightSingleRow<TileDataDst, TileDataSrc>(dstPtr, padOffset, padCols, dupPadValue);

    // pad right for remaining rows (if any)
    if constexpr (TileDataSrc::Rows > 1) {
        PadRightRemainingRows<TileDataDst, TileDataSrc>(dstPtr, padOffset, copyDstCols, srcValidRow);
    }

    // pad bottom rows
    PadBottomRows<TileDataDst, TileDataSrc>(dstPtr, srcValidRow, dstValidRow, copyDstCols, dupPadValue);

    set_mask_norm(); // restore to norm mode
    set_vector_mask(-1, -1);
} // end of tf

template <typename TileDataDst, typename TileDataSrc, bool inplace>
PTO_INTERNAL void TFILLPAD_GENERIC_IMPL(TileDataDst &dst, TileDataSrc &src) {
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataSrc::DType);
    constexpr unsigned dstStride = TileDataDst::RowStride;
    constexpr unsigned srcStride = TileDataSrc::RowStride;
    uint64_t validDstRow = dst.GetValidRow();
    uint64_t validDstCol = dst.GetValidCol();
    uint64_t validSrcRow = src.GetValidRow();
    uint64_t validSrcCol = src.GetValidCol();

    using T = typename TileDataSrc::DType;
    using U = typename TileDataDst::DType;
    static_assert(TileDataDst::PadVal != PadValue::Null, "Fix: TFillPad dst vecTile pad value must not be Null!");
    static_assert(sizeof(T) == sizeof(U), "Fix: TFillPad src and dst data type is different!");
    static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "Fix: TFillPad has invalid data type.");

    if (validDstRow == 0 || validDstCol == 0) {
        return;
    }
    if constexpr (!inplace) {
        TFillPad_CopyData<TileDataDst, TileDataSrc>(
            dst.data(), src.data(), validDstRow, validDstCol, validSrcRow, validSrcCol);
    }
    TFillPad<TileDataDst, TileDataSrc>(dst.data(), src.data(), validDstRow, validDstCol, validSrcRow, validSrcCol);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TFILLPAD_IMPL(TileDataDst &dst, TileDataSrc &src) {
    static_assert(TileDataDst::Cols == TileDataSrc::Cols && TileDataDst::Rows == TileDataSrc::Rows,
        "Fix: TFillPad Dst/Src vecTile Rows/Cols must be the same.");

    TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, false>(dst, src);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TFILLPAD_INPLACE_IMPL(TileDataDst &dst, TileDataSrc &src) {
    static_assert(TileDataDst::Cols == TileDataSrc::Cols && TileDataDst::Rows == TileDataSrc::Rows,
        "Fix: TFillPad Dst vecTile Rows/Cols must be greater or equal to src vecTile.");

    TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, true>(dst, src);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TFILLPAD_EXPAND_IMPL(TileDataDst &dst, TileDataSrc &src) {
    static_assert(TileDataDst::Cols >= TileDataSrc::Cols && TileDataDst::Rows >= TileDataSrc::Rows,
        "Fix: TFillPad Dst/Src vecTile Rows/Cols must be the same.");

    TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, false>(dst, src);
}

template <typename TileData>
__tf__ PTO_INTERNAL void TFillPad(typename TileData::TileDType __out__ dst, uint32_t dstValidRow, uint32_t dstValidCol)
{
    using U = typename TileData::DType;
    __cbuf__ U *dstPtr = (__cbuf__ U *)__cce_get_tile_ptr(dst);
    constexpr uint32_t elementsPerBlock = C0_SIZE_BYTE / sizeof(U);
    uint32_t alignedValidCol = CeilAlignment(dstValidCol, elementsPerBlock);

#if defined(__DAV_CUBE__)
    uint16_t blockLen = TileData::Rows - dstValidRow; // unit is 32B
    uint16_t repeat = alignedValidCol / elementsPerBlock;
    uint16_t repeatGap = dstValidRow;

    int64_t repeatConfig =
        (static_cast<uint64_t>(blockLen) << 16) |  // [30:16] is the block number of each repeat
        (static_cast<uint64_t>(repeatGap) << 32) | // [46:32] is the repeat gap between two consecutive repeats
        static_cast<uint64_t>(repeat);             // [14:0] is the repeat times
    if (blockLen != 0) {
        create_cbuf_matrix((__cbuf__ uint16_t *)(dstPtr + dstValidRow * elementsPerBlock), repeatConfig, 0);
    }
    if (alignedValidCol != TileData::Cols) { // if alignedValidCol is not equal to TileData::Cols, need to pad the left column
        blockLen = TileData::Rows;        // unit is 32B
        repeatConfig = (static_cast<uint64_t>(blockLen) << 16) | // [30:16] is the block number of each repeat
                       (static_cast<uint64_t>(0) << 32) | 1;     // [46:32] is the repeat gap
        create_cbuf_matrix((__cbuf__ uint16_t *)(dstPtr + TileData::Rows * alignedValidCol), repeatConfig, 0);
    }
#endif
}

	template <typename TileData, PadValue PadVal = PadValue::Zero>
	PTO_INTERNAL void TFILLPAD_IMPL(TileData &dst, TileData &src)
	{
	    if constexpr (TileData::Loc == TileType::Vec) {
	        static_assert(TileData::PadVal == PadVal,
	            "Fix: TFILLPAD VecTile PadValue mismatch between tile type and instruction template.");
	        TFILLPAD_GENERIC_IMPL<TileData, TileData, false>(dst, src);
	    } else {
	        static_assert(TileData::Loc == TileType::Mat, "Fix: TFILLPAD PadValue overload only supports Vec/Mat tiles.");
	        static_assert(!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor),
	            "Fix: TFillPad Dst matTile now only support NZ layout.");
	        static_assert(TileData::PadVal == PadValue::Zero || TileData::PadVal == PadValue::Null,
	            "Fix: TFillPad dst matTile pad value only support Zero or Null!");
	        using T = typename TileData::DType;
	        static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1,
	            "Fix: TFillPad type must be b4/b8/b16/b32.");

	        uint32_t validDstRow = dst.GetValidRow();
	        uint32_t validDstCol = dst.GetValidCol();
	        TFillPad<TileData>(dst.data(), validDstRow, validDstCol);
	    }
	}

} // namespace pto
#endif
