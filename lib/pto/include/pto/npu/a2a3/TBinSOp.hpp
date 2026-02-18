/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TBINS_HPP
#define TBINS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
namespace pto
{
    #define SMALL_RPT (4)
    template <typename Op, typename T>
    PTO_INTERNAL void BinS1LCountMode(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        set_mask_count();
        SetVectorCount(validRow * validCol);
        Op::BinSInstr(dst, src0, src1, 0);
        set_mask_norm();
        SetFullVecMaskByDType<T>();
    }
    template <typename Op, typename T, unsigned dstStride, unsigned srcStride>
    PTO_INTERNAL void BinS2LCountMode(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        set_mask_count();
        SetVectorCount(validCol);
        for (unsigned i = 0; i < validRow; i++) {
            unsigned dstOffset = i * dstStride;
            unsigned srcOffset = i * srcStride;
            Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, 0);
        }
        set_mask_norm();
        SetFullVecMaskByDType<T>();
    }
    template <typename Op, typename T, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned Cols>
    PTO_INTERNAL void BinS1LNormMode(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        unsigned numElements = validRow * validCol;
        unsigned headRepeats = numElements / elementsPerRepeat;
        unsigned tailElements = numElements % elementsPerRepeat;
        Op::BinSInstr(dst, src0, src1, headRepeats);
        if (tailElements) [[unlikely]] {
            unsigned offset = headRepeats * elementsPerRepeat;
            SetContMaskByDType<T>(tailElements);
            Op::BinSInstr(dst + offset, src0 + offset, src1, 1);
            SetFullVecMaskByDType<T>();
        }
    }
    template <typename Op, typename T, unsigned elementsPerRepeat, unsigned dstStride, unsigned srcStride>
    PTO_INTERNAL void BinS2LNormModeColVLAlign(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        unsigned headRepeats = validCol / elementsPerRepeat;
        for (uint32_t i = 0; i < validRow; i++) {
            unsigned dstOffset = i * dstStride;
            unsigned srcOffset = i * srcStride;
            Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, headRepeats);
        }
    }
    template <typename Op, typename T, unsigned Rows, unsigned elementsPerRepeat, unsigned blockSizeElem,
              unsigned dstStride, unsigned srcStride>
    PTO_INTERNAL void BinS2LNormModeHead(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned numRepeatPerLine) {
        if (numRepeatPerLine > 0) {
            unsigned numLoop = numRepeatPerLine / REPEAT_MAX;
            unsigned remainAfterLoop = numRepeatPerLine % REPEAT_MAX;
            for (int i = 0; i < validRow; i++) {
                if (numLoop) [[unlikely]] {
                    for (int j = 0; j < numLoop; j++) {
                        unsigned dstOffset = i * dstStride + j * elementsPerRepeat * REPEAT_MAX;
                        unsigned srcOffset = i * srcStride + j * elementsPerRepeat * REPEAT_MAX;
                        Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, REPEAT_MAX);
                    }
                }
                if (remainAfterLoop) {
                    unsigned dstOffset = i * dstStride + numLoop * elementsPerRepeat * REPEAT_MAX;
                    unsigned srcOffset = i * srcStride + numLoop * elementsPerRepeat * REPEAT_MAX;
                    Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, remainAfterLoop);
                }   
            }
        }
    }
    
    template <typename Op, typename T, unsigned Rows, unsigned elementsPerRepeat, unsigned blockSizeElem,
              unsigned dstStride, unsigned srcStride>
    PTO_INTERNAL void BinS2LNormModeTail(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned numRemainPerLine) {
        unsigned numLoop = 0;
        unsigned remainAfterLoop = validRow;
        const bool strideOverFlag = (dstStride / blockSizeElem > REPEAT_STRIDE_MAX) ||
                                    (srcStride / blockSizeElem > REPEAT_STRIDE_MAX);
        SetContMaskByDType<T>(numRemainPerLine);
        if constexpr (Rows > pto::REPEAT_MAX) {
            numLoop = validRow / REPEAT_MAX;
            for (int i = 0; i < numLoop; i++) {
                if constexpr (strideOverFlag) {
                    for (uint64_t j = 0; j < REPEAT_MAX; j++) {
                        unsigned dstOffset = i * REPEAT_MAX * dstStride + j * dstStride;
                        unsigned srcOffset = i * REPEAT_MAX * srcStride + j * srcStride;
                        Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, 1, 1, 1);
                    }
                } else {
                    unsigned dstOffset = i * REPEAT_MAX * dstStride;
                    unsigned srcOffset = i * REPEAT_MAX * srcStride;
                    uint8_t dstRepeatStride = dstStride / blockSizeElem;
                    uint8_t srcRepeatStride = srcStride / blockSizeElem;
                    Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, REPEAT_MAX, dstRepeatStride,
                                    srcRepeatStride);
                }
            }
            remainAfterLoop = validRow % REPEAT_MAX;
        }
        
        if (remainAfterLoop) {
            if constexpr (strideOverFlag) {
                for (unsigned j = 0; j < remainAfterLoop; j++) {
                    unsigned dstOffset = numLoop * REPEAT_MAX * dstStride + j * dstStride;
                    unsigned srcOffset = numLoop * REPEAT_MAX * srcStride + j * srcStride;
                    Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, 1, 1, 1);
                }
            } else {
                unsigned dstOffset = numLoop * REPEAT_MAX * dstStride;
                unsigned srcOffset = numLoop * REPEAT_MAX * srcStride;
                uint8_t dstRepeatStride = dstStride / blockSizeElem;
                uint8_t srcRepeatStride = srcStride / blockSizeElem;
                Op::BinSInstr(dst + dstOffset, src0 + srcOffset, src1, remainAfterLoop, dstRepeatStride,
                                srcRepeatStride);
            }
        }
        SetFullVecMaskByDType<T>();
    }

    template <typename Op, typename T, unsigned Rows, unsigned elementsPerRepeat, unsigned blockSizeElem,
              unsigned dstStride, unsigned srcStride>
    PTO_INTERNAL void BinS2LNormModeRowRpt(__ubuf__ T* dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        constexpr unsigned dstRepeatStride = dstStride / blockSizeElem;
        constexpr unsigned srcRepeatStride = srcStride / blockSizeElem;
        constexpr bool condRowRpt = ((Rows <= pto::REPEAT_MAX) && dstRepeatStride <= (REPEAT_STRIDE_MAX) &&
                                    srcRepeatStride <= (REPEAT_STRIDE_MAX));
        if constexpr (condRowRpt) {
            unsigned numLoop = validCol / elementsPerRepeat;
            unsigned tailElements = validCol % elementsPerRepeat;
            for (unsigned i = 0; i < numLoop; i++) {
                unsigned offset = i * elementsPerRepeat;
                Op::BinSInstr(dst + offset, src0 + offset, src1, validRow, dstRepeatStride, srcRepeatStride);
            }

            if (tailElements) {
                unsigned offset = numLoop * elementsPerRepeat;
                SetContMaskByDType<T>(tailElements);
                Op::BinSInstr(dst + offset, src0 + offset, src1, validRow, dstRepeatStride, srcRepeatStride);
                SetFullVecMaskByDType<T>();
            }
        } else {
            unsigned numRemainPerLine = validCol;
            if constexpr (Rows > elementsPerRepeat) {
                unsigned numRepeatPerLine = validCol / elementsPerRepeat;
                numRemainPerLine = validCol % elementsPerRepeat;
                BinS2LNormModeHead<Op, T, Rows, elementsPerRepeat, blockSizeElem, dstStride, srcStride>
                    (dst, src0, src1, validRow, numRepeatPerLine);
                unsigned offset = numRepeatPerLine * elementsPerRepeat;
                dst += offset; 
                src0 += offset; 
            }
            if (numRemainPerLine) {
                BinS2LNormModeTail<Op, T, Rows, elementsPerRepeat, blockSizeElem, dstStride, srcStride>
                    (dst, src0, src1, validRow, numRemainPerLine);
            }
        }
    }
    template <typename Op, typename TileDataDst, typename TileDataSrc, unsigned elementsPerRepeat,
              unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
    PTO_INTERNAL void TBinSInstr(__ubuf__ typename TileDataDst::DType __out__ *dst,
        __ubuf__ typename TileDataSrc::DType __in__ *src0, typename TileDataSrc::DType __in__ src1, unsigned validRow,
        unsigned validCol)
    {
        using T = typename TileDataDst::DType;
        constexpr bool tileDataContinue = ((TileDataDst::Cols == TileDataDst::ValidCol) &&
            (TileDataSrc::Cols == TileDataSrc::ValidCol)) || ((TileDataDst::Rows == 1) && (TileDataSrc::Rows == 1));
        if constexpr (tileDataContinue) {
            constexpr unsigned totalRepeats = (TileDataDst::Rows * TileDataDst::Cols + elementsPerRepeat - 1) /
                                              elementsPerRepeat;
            constexpr bool nonVLAligned = (((TileDataDst::Cols % elementsPerRepeat) != 0) &&
                                          (TileDataDst::Cols > elementsPerRepeat));
            constexpr bool enbleCountMode = nonVLAligned || (totalRepeats > pto::REPEAT_MAX);
            if constexpr (enbleCountMode) {
                BinS1LCountMode<Op, T>(dst, src0, src1, validRow, validCol);
            } else {
                BinS1LNormMode<Op, T, elementsPerRepeat, blockSizeElem, TileDataDst::Cols>
                    (dst, src0, src1, validRow, validCol);
            }
        } else {
            if (tileDataContinue) [[likely]] {
                unsigned totalRepeats = (validRow * validCol + elementsPerRepeat - 1) / elementsPerRepeat;
                bool nonVLAligned = ((validCol > elementsPerRepeat) && ((validCol % elementsPerRepeat) != 0));
                bool enbleCountMode = nonVLAligned || (totalRepeats > pto::REPEAT_MAX);
                if (enbleCountMode) [[unlikely]] {
                    BinS1LCountMode<Op, T>(dst, src0, src1, validRow, validCol);
                } else {
                    BinS1LNormMode<Op, T, elementsPerRepeat, blockSizeElem, TileDataDst::Cols>
                        (dst, src0, src1, validRow, validCol);
                }
            } else {
                constexpr unsigned normColRepeat = TileDataDst::Cols / elementsPerRepeat;
                constexpr bool countMode = (normColRepeat > 1) && ((TileDataDst::Rows * normColRepeat) < SMALL_RPT) &&
                                           ((TileDataSrc::Rows * normColRepeat) < SMALL_RPT);
                constexpr bool isColRpt = (TileDataDst::Rows < (normColRepeat + 1)) &&
                                          (TileDataSrc::Rows < (normColRepeat + 1));
                if constexpr (countMode) {
                    BinS2LCountMode<Op, T, dstStride, srcStride>(dst, src0, src1, validRow, validCol);
                } else if constexpr (isColRpt) {
                    unsigned tailElements = validCol % elementsPerRepeat;
                    if (tailElements) {
                        BinS2LCountMode<Op, T, dstStride, srcStride>(dst, src0, src1, validRow, validCol);    
                    } else {
                        BinS2LNormModeColVLAlign<Op, T, elementsPerRepeat, dstStride, srcStride>(dst, src0, src1,
                            validRow, validCol);
                    }
                } else {
                    BinS2LNormModeRowRpt<Op, T, TileDataDst::Rows, elementsPerRepeat, blockSizeElem, dstStride,
                        srcStride>(dst, src0, src1, validRow, validCol);
                }
            }
        }
    }
} //namespace pto
#endif
