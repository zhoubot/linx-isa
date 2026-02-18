/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSELS_HPP
#define TSELS_HPP

#include <pto/common/constants.hpp>

namespace pto {
    template <typename T, unsigned elementsPerRepeat, unsigned stride>
    PTO_INTERNAL void TSelsHead(
        __ubuf__ T *dstPtr,
        __ubuf__ T *src0Ptr,
        __ubuf__ T *src1Ptr,
        unsigned validRow,
        unsigned numRepeatPerLine,
        uint8_t opSelectionMode
    ) {
        unsigned numLoop = numRepeatPerLine / REPEAT_MAX;
        unsigned remainAfterLoop = numRepeatPerLine % REPEAT_MAX;
        for (uint64_t i = 0; i < validRow; i++) {
            if (numLoop) {
                for (int j = 0; j < numLoop; j++) {
                    vsel(
                        dstPtr + i * stride + j * elementsPerRepeat * REPEAT_MAX,
                        src0Ptr + i * stride + j * elementsPerRepeat * REPEAT_MAX,
                        src1Ptr + i * stride + j * elementsPerRepeat * REPEAT_MAX,
                        REPEAT_MAX,
                        1, 1, 1,
                        8, 8, 8,
                        opSelectionMode
                        );
                    }
            }
            if (remainAfterLoop) {
                vsel(
                    dstPtr + i * stride + numLoop * elementsPerRepeat * REPEAT_MAX,
                    src0Ptr + i * stride + numLoop * elementsPerRepeat * REPEAT_MAX,
                    src1Ptr + i * stride + numLoop * elementsPerRepeat * REPEAT_MAX,
                    remainAfterLoop,
                    1, 1, 1,
                    8, 8, 8,
                    opSelectionMode
                );
            }
        }
    }
    template <typename T, unsigned blockSizeElem, unsigned stride, bool strideOverFlag>
    PTO_INTERNAL void TSelsTailHead(
        __ubuf__ T *dstPtr,
        __ubuf__ T *src0Ptr,
        __ubuf__ T *src1Ptr,
        unsigned numLoop,
        uint8_t opSelectionMode
    ) {
        for (uint64_t i = 0; i < numLoop; i++) {
            if constexpr (strideOverFlag) {
                for (uint64_t j = 0; j < REPEAT_MAX; j++) {
                    vsel(
                        dstPtr + i * REPEAT_MAX * stride + j * stride,
                        src0Ptr + i * REPEAT_MAX * stride + j * stride,
                        src1Ptr + i * REPEAT_MAX * stride + j * stride,
                        1,
                        1, 1, 1,
                        1, 1, 1,
                        opSelectionMode
                    );
                }
            } else {
                vsel(
                    dstPtr + i * REPEAT_MAX * stride,
                    src0Ptr + i * REPEAT_MAX * stride,
                    src1Ptr + i * REPEAT_MAX * stride,
                    REPEAT_MAX,
                    1, 1, 1,
                    stride / blockSizeElem, stride / blockSizeElem, stride / blockSizeElem,
                    opSelectionMode
                );
            }
        }
    }
    template <typename T, unsigned blockSizeElem, unsigned stride>
    PTO_INTERNAL void TSelsTail(
        __ubuf__ T *dstPtr,
        __ubuf__ T *src0Ptr,
        __ubuf__ T *src1Ptr,
        unsigned validRow,
        unsigned numRemainPerLine,
        uint8_t opSelectionMode
    ) {
        unsigned numLoop = validRow / REPEAT_MAX;
        unsigned remainAfterLoop = validRow % REPEAT_MAX;
        bool constexpr strideOverFlag = (stride / blockSizeElem > REPEAT_STRIDE_MAX);
        SetContinuousMask(numRemainPerLine);
        if (numLoop > 0) {
            TSelsTailHead<T, blockSizeElem, stride, strideOverFlag>(dstPtr, src0Ptr, src1Ptr, numLoop, opSelectionMode);
        }
        if (remainAfterLoop > 0) {
            if constexpr (strideOverFlag) {
                for (uint64_t j = 0; j < remainAfterLoop; j++) {
                    vsel(
                        dstPtr + numLoop * REPEAT_MAX * stride + j * stride,
                        src0Ptr + numLoop * REPEAT_MAX * stride + j * stride,
                        src1Ptr + numLoop * REPEAT_MAX * stride + j * stride,
                        1,
                        1, 1, 1,
                        1, 1, 1,
                        opSelectionMode
                    );
                }
            } else {
                vsel(
                    dstPtr + numLoop * REPEAT_MAX * stride,
                    src0Ptr + numLoop * REPEAT_MAX * stride,
                    src1Ptr + numLoop * REPEAT_MAX * stride,
                    remainAfterLoop,
                    1, 1, 1,
                    stride / blockSizeElem, stride / blockSizeElem, stride / blockSizeElem,
                    opSelectionMode
                );
            }
        }
        set_vector_mask(-1, -1);
    }
    template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned stride>
    __tf__ AICORE void TSelsImpl(
        typename TileData::TileDType __out__ dst,
        typename TileData::TileDType __in__ src0,
        typename TileData::TileDType __in__ src1,
        uint8_t scalar,
        unsigned validRow,
        unsigned numRepeatPerLine,
        unsigned numRemainPerLine
    ) {
        using T = typename TileData::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
        __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
        __ubuf__ uint64_t *tmpPtr = (__ubuf__ uint64_t *)(TMP_UB_OFFSET); // 8KB, start from 184KB, UB:192KB=184+8KB
        constexpr uint8_t opSelectionMode = 0;
        uint64_t mask = 0;
        if (scalar == 1) {
            mask = 0xFFFFFFFFFFFFFFFF;
        }

        tmpPtr[0] = mask;
        tmpPtr[1] = mask;

        set_cmpmask(tmpPtr);
        pipe_barrier(PIPE_V);

        if (numRepeatPerLine > 0) {
            TSelsHead<T, elementsPerRepeat, stride>(dstPtr, src0Ptr, src1Ptr, validRow, numRepeatPerLine, opSelectionMode);
        }

        dstPtr += numRepeatPerLine * elementsPerRepeat;
        src0Ptr += numRepeatPerLine * elementsPerRepeat;
        src1Ptr += numRepeatPerLine * elementsPerRepeat;

        if (numRemainPerLine > 0) {
            TSelsTail<T, blockSizeElem, stride>(dstPtr, src0Ptr, src1Ptr, validRow, numRemainPerLine, opSelectionMode);
        }
    }

    template <typename TileData>
    PTO_INTERNAL void TSELS_IMPL(
        TileData &dst,
        TileData &src0,
        TileData &src1,
        uint8_t selectMode
    ) {
        using T = typename TileData::DType;
        static_assert(
            std::is_same<T, half>::value ||
            std::is_same<T, float16_t>::value ||
            std::is_same<T, float>::value ||
            std::is_same<T, float32_t>::value,
            "TSELS: Invalid data type");
        static_assert(TileData::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileData::isRowMajor, "TSELS: not supported Layout type");
        static_assert(TileData::ValidCol <= TileData::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileData::ValidRow <= TileData::Rows, "Number of valid rows must not be greater than number of tile rows.");
        
        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
        constexpr unsigned stride = TileData::RowStride;
        unsigned numRepeatPerLine = dst.GetValidCol() / elementsPerRepeat;
        unsigned numRemainPerLine = dst.GetValidCol() % elementsPerRepeat;
        unsigned validRow = dst.GetValidRow();

        PTO_ASSERT(src0.GetValidCol() == src1.GetValidCol(), "Number of columns of src0, src1 must be the same.");
        PTO_ASSERT(src1.GetValidCol() == dst.GetValidCol(), "Number of columns of src1 and dst must be the same.");
        PTO_ASSERT(src0.GetValidRow() == src1.GetValidRow(), "Number of rows of src0, src1 must be the same.");
        PTO_ASSERT(src1.GetValidRow() == dst.GetValidRow(), "Number of rows of src1 and dst must be the same.");

        TSelsImpl<TileData, elementsPerRepeat, blockSizeElem, stride>(
            dst.data(), src0.data(), src1.data(), selectMode, validRow, numRepeatPerLine, numRemainPerLine);
    }
}
#endif