/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef T_COL_REDUCE_OPS_HPP
#define T_COL_REDUCE_OPS_HPP

#include <pto/common/utils.hpp>
#include <pto/common/type.hpp>

namespace pto {
    template <typename T, typename InstrOp>
    struct TColReduceOp {
        template <int dupSrcStride>
        PTO_INTERNAL static void ColReduceInstrByMode(__ubuf__ T *dst, __ubuf__ T *src, int numRepeatPerLine,
                                                       int numRemainPerLine, int elementsPerLine, int validRow)
        {
            if (numRepeatPerLine > 0) {
                set_mask_count();
                set_vector_mask(0, elementsPerLine);
                for (int i = 1; i < validRow; i++) {
                    InstrOp::ReduceInstr(dst, dst, src + i * dupSrcStride, 0, 8, 8, 8);
                    pipe_barrier(PIPE_V);
                }
                set_mask_norm();
                set_vector_mask(-1, -1);
            }

            dst += elementsPerLine;
            src += elementsPerLine;

            if (numRemainPerLine > 0) {
                SetContinuousMask(numRemainPerLine);
                for (int i = 1; i < validRow; i++) {
                    InstrOp::ReduceInstr(dst, dst, src + i * dupSrcStride, 1, 8, 8, 8);
                    pipe_barrier(PIPE_V);
                }
                set_vector_mask(-1, -1);
            }
        }
    };
    
    template <typename InstrOp, typename T, typename TileDataOut, typename TileDataIn, unsigned srcstride>
    PTO_INTERNAL void ColReduceInstr(__ubuf__ T *dst, __ubuf__ T *src, int validRow, int validCol) {
        using ReduceOp = TColReduceOp<T, InstrOp>;
        constexpr int DTypeSize = sizeof(T);
        int lenBurst = (validCol * DTypeSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;
        
        copy_ubuf_to_ubuf(dst, src, 0, 1, lenBurst, 0, 0);
        pipe_barrier(PIPE_V);
        if (validRow == 1) {
            return;
        }

        constexpr int blockSizeElem = BLOCK_BYTE_SIZE / DTypeSize;
        constexpr int numBlockPerLine = (srcstride * DTypeSize + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;
        constexpr int dupSrcStride = numBlockPerLine * blockSizeElem;
        constexpr int elementsPerRepeat = REPEAT_BYTE / DTypeSize;
        int numRepeatPerLine = validCol / elementsPerRepeat;
        int numRemainPerLine = validCol % elementsPerRepeat;
        int elementsPerLine = numRepeatPerLine * elementsPerRepeat;
        
        ReduceOp::template ColReduceInstrByMode<dupSrcStride>(dst, src, numRepeatPerLine, numRemainPerLine,
            elementsPerLine, validRow);
        pipe_barrier(PIPE_V);
    }

    template <typename T, typename TileDataOut, typename TileDataIn>
    PTO_INTERNAL void TColReduceCheck(int SrcValidRow, int SrcValidCol, int DstValidCol) {
        static_assert(TileDataOut::Loc == pto::TileType::Vec && TileDataIn::Loc == pto::TileType::Vec,
                      "Fix: TCOLREDUCE only support Vec Tile");
        static_assert(TileDataIn::isRowMajor && TileDataIn::SFractal == SLayout::NoneBox,
                      "Fix: TCOLREDUCE input tile only support Nd fractal Tile");
        static_assert(TileDataOut::isRowMajor && TileDataOut::SFractal == SLayout::NoneBox,
                      "Fix: TCOLREDUCE output tile only support Nd fractal Tile");
        static_assert(std::is_same_v<T, half> || std::is_same_v<T, float> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>,
                      "Fix: TCOLREDUCE input data type is not supported by this instruction.");
        static_assert(std::is_same_v<typename TileDataOut::DType, T>,
                      "Fix: TCOLREDUCE input data type must be consistent with the output data type.");
        PTO_ASSERT(SrcValidCol == DstValidCol,
            "Fix: TCOLREDUCE input valid col must be consistent with the output valid row.");
        if (SrcValidRow == 0 || SrcValidCol == 0 || DstValidCol == 0) {
            return;
        }
    }
}
#endif