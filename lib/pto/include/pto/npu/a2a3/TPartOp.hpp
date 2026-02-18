/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPARTOP_HPP
#define TPARTOP_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>

namespace pto {
template <typename T, int dstCols, int srcCols, unsigned dstStride, unsigned srcStride>
PTO_INTERNAL
void TPartCopyInstr(__ubuf__ T *dstPtr, __ubuf__ T *srcPtr, uint64_t validRow, uint64_t validCol, uint64_t startRow)
{
    validRow -= startRow;
    srcPtr += startRow * srcStride;
    dstPtr += startRow * dstStride;

    if constexpr (dstCols == srcCols) {
        set_mask_count();  // counter mode
        SetVectorCount(dstCols * validRow);
        uint64_t blockLen = (dstCols * validRow * sizeof(T) + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;
        copy_ubuf_to_ubuf(dstPtr, srcPtr, 0, 1, blockLen, 1, 1);
    } else {
        set_mask_count();  // counter mode
        SetVectorCount(validCol);
        uint64_t blockLen = (validCol * sizeof(T) + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;
        for (uint64_t i = 0; i < validRow; i++) {
            copy_ubuf_to_ubuf(dstPtr + i * dstStride, srcPtr + i * srcStride, 0, 1, blockLen, 1, 1);
        }
    }
    set_mask_norm();
    SetFullVecMaskByDType<T>();
}

template <typename Op, typename T, unsigned dstStride, unsigned src0Stride, unsigned src1Stride>
PTO_INTERNAL
void PartCountMode(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRow, unsigned validCol)
{
    set_mask_count();
    SetVectorCount(validCol);
    for (unsigned i = 0; i < validRow; i++) {
        Op::PartInstr(dstPtr + i * dstStride, src0Ptr + i * src0Stride, src1Ptr + i * src1Stride, 0);
    }
    set_mask_norm();
    SetFullVecMaskByDType<T>();
}

template <typename Op, typename T, int dstRow, unsigned elementsPerRepeat, unsigned blockSizeElem,
    unsigned dstStride, unsigned src0Stride, unsigned src1Stride>
PTO_INTERNAL
void PartNormModeTail(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRow, unsigned validCol)
{
    unsigned numRepeatPerCol = validRow / REPEAT_MAX;
    unsigned numRemainPerCol = validRow % REPEAT_MAX;
    unsigned dstRepeatStride = dstStride / blockSizeElem;
    unsigned src0RepeatStride = src0Stride / blockSizeElem;
    unsigned src1RepeatStride = src1Stride / blockSizeElem;
    unsigned dstRepeatMaxStride = REPEAT_MAX * dstStride;
    unsigned src0RepeatMaxStride = REPEAT_MAX * src0Stride;
    unsigned src1RepeatMaxStride = REPEAT_MAX * src1Stride;
    unsigned numRemainPerLine = validCol % elementsPerRepeat;
    SetContMaskByDType<T>(numRemainPerLine);
    if constexpr (dstRow >= REPEAT_MAX) {
        for (unsigned j = 0; j < numRepeatPerCol; j++) {
            Op::PartInstr(dstPtr + j * dstRepeatMaxStride,
                 src0Ptr + j * src0RepeatMaxStride,
                 src1Ptr + j * src1RepeatMaxStride,
                 REPEAT_MAX, dstRepeatStride, src0RepeatStride, src1RepeatStride);
        }
    }
    if (numRemainPerCol) {
        Op::PartInstr(dstPtr + numRepeatPerCol * dstRepeatMaxStride,
             src0Ptr + numRepeatPerCol * src0RepeatMaxStride,
             src1Ptr + numRepeatPerCol * src1RepeatMaxStride,
             numRemainPerCol, dstRepeatStride, src0RepeatStride, src1RepeatStride);
    }
    SetFullVecMaskByDType<T>();
}

template <typename Op, typename T, int dstRow, int dstCol, unsigned elementsPerRepeat, unsigned blockSizeElem,
    unsigned dstStride, unsigned src0Stride, unsigned src1Stride>
PTO_INTERNAL
void PartNormMode(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRow, unsigned validCol)
{
    unsigned numRepeatPerLine = validCol / elementsPerRepeat;
    unsigned numRemainPerLine = validCol % elementsPerRepeat;
    if constexpr (dstCol >= static_cast<int>(elementsPerRepeat)) {
        unsigned numRepeatPerCol = validRow / REPEAT_MAX;
        unsigned numRemainPerCol = validRow % REPEAT_MAX;
        unsigned dstRepeatStride = dstStride / blockSizeElem;
        unsigned src0RepeatStride = src0Stride / blockSizeElem;
        unsigned src1RepeatStride = src1Stride / blockSizeElem;
        unsigned dstRepeatMaxStride = REPEAT_MAX * dstStride;
        unsigned src0RepeatMaxStride = REPEAT_MAX * src0Stride;
        unsigned src1RepeatMaxStride = REPEAT_MAX * src1Stride;
        for (unsigned i = 0; i < numRepeatPerLine; i++) {
            if constexpr (dstRow >= REPEAT_MAX) {
                for (unsigned j = 0; j < numRepeatPerCol; j++) {
                    Op::PartInstr(dstPtr + j * dstRepeatMaxStride + i * elementsPerRepeat,
                         src0Ptr + j * src0RepeatMaxStride + i * elementsPerRepeat,
                         src1Ptr + j * src1RepeatMaxStride + i * elementsPerRepeat,
                         REPEAT_MAX, dstRepeatStride, src0RepeatStride, src1RepeatStride);
                }
            }
            if (numRemainPerCol) {
                Op::PartInstr(dstPtr + numRepeatPerCol * dstRepeatMaxStride + i * elementsPerRepeat,
                     src0Ptr + numRepeatPerCol * src0RepeatMaxStride + i * elementsPerRepeat,
                     src1Ptr + numRepeatPerCol * src1RepeatMaxStride + i * elementsPerRepeat,
                     numRemainPerCol, dstRepeatStride, src0RepeatStride, src1RepeatStride);
            }
        }
        unsigned offset = numRepeatPerLine * elementsPerRepeat;
        dstPtr += offset;
        src0Ptr += offset;
        src1Ptr += offset;
    }
    if (numRemainPerLine) {
        PartNormModeTail<Op, T, dstRow, elementsPerRepeat, blockSizeElem, dstStride, src0Stride, src1Stride>(
                dstPtr, src0Ptr, src1Ptr, validRow, validCol);
    }
}

template <typename Op, typename T, int dstRow, int dstCol, unsigned elementsPerRepeat, unsigned blockSizeElem,
    unsigned dstStride, unsigned src0Stride, unsigned src1Stride>
PTO_INTERNAL
void TPartOps(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned validRow, unsigned validCol) {
    bool constexpr strideOverFlag = ((src0Stride / blockSizeElem > REPEAT_STRIDE_MAX) ||
                                     (src1Stride / blockSizeElem > REPEAT_STRIDE_MAX) ||
                                     (dstStride / blockSizeElem > REPEAT_STRIDE_MAX));
    if constexpr (strideOverFlag) {
        PartCountMode<Op, T, dstStride, src0Stride, src1Stride>(dstPtr, src0Ptr, src1Ptr, validRow, validCol);
    } else {
        if (validRow < CeilDivision(validCol, elementsPerRepeat) * CeilDivision(validRow, REPEAT_MAX)) {
            PartCountMode<Op, T, dstStride, src0Stride, src1Stride>(dstPtr, src0Ptr, src1Ptr, validRow, validCol);
        } else {
            PartNormMode<Op, T, dstRow, dstCol, elementsPerRepeat, blockSizeElem, dstStride, src0Stride, src1Stride>(
                dstPtr, src0Ptr, src1Ptr, validRow, validCol);
        }
    }
}

template <typename Op, typename T, int dstCol, int src0Col, int dstRow, unsigned elementsPerRepeat,
          unsigned blockSizeElem, unsigned dstRowStride, unsigned src0RowStride, unsigned src1RowStride>
PTO_INTERNAL
void TPartInstr(__ubuf__ T *dstPtr, __ubuf__ T *src0Ptr, __ubuf__ T *src1Ptr, unsigned src0ValidRow,
    unsigned src0ValidCol, unsigned src1ValidRow, unsigned src1ValidCol, unsigned dstValidRow, unsigned dstValidCol)
{
    bool condSrc1EqDst = (src1ValidRow == dstValidRow && src1ValidCol == dstValidCol);
    bool condSrc1RowLtDst = (src1ValidRow < dstValidRow && src1ValidCol == dstValidCol);
    bool condSrc1ColLtDst = (src1ValidRow <= dstValidRow && src1ValidCol < dstValidCol);

    if (condSrc1RowLtDst) {  // src1Row < dstRow
        if (src1ValidRow != 0) {
            TPartOps<Op, T, dstRow, dstCol, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride, src1RowStride>(
                dstPtr, src0Ptr, src1Ptr, src1ValidRow, src1ValidCol);
        }
        TPartCopyInstr<T, dstCol, src0Col, dstRowStride, src0RowStride>(
            dstPtr, src0Ptr, src0ValidRow, dstValidCol, src1ValidRow);
    } else if (condSrc1ColLtDst) {  // src1Col < dstCol
        TPartCopyInstr<T, dstCol, src0Col, dstRowStride, src0RowStride>(
            dstPtr, src0Ptr, src0ValidRow, dstValidCol, 0);
        if (src1ValidCol != 0) {
            pipe_barrier(PIPE_V);
            TPartOps<Op, T, dstRow, dstCol, elementsPerRepeat, blockSizeElem, dstRowStride, src1RowStride, dstRowStride>(
                dstPtr, src1Ptr, dstPtr, src1ValidRow, src1ValidCol);
        }
    } else if (condSrc1EqDst) {  // src0 == src1 == dst
        TPartOps<Op, T, dstRow, dstCol, elementsPerRepeat, blockSizeElem, dstRowStride, src0RowStride, src1RowStride>(
            dstPtr, src0Ptr, src1Ptr, dstValidRow, dstValidCol);
    } else {
        // unsupport other conditions
        PTO_ASSERT(condSrc1EqDst || condSrc1RowLtDst || condSrc1ColLtDst,
            "TPARTOPS: At most one entry in the valid-rows and valid-cols of src0 and src1 is smaller than dst.");
    }
}
}  // namespace pto
#endif