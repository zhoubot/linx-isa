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
#include <limits>

#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"

namespace pto{

    template <typename TileDataDst, typename TileDataSrc>
    void TFillPad(typename TileDataDst::TileDType dst,
                                typename TileDataSrc::TileDType src,
                                unsigned validDstRow, 
                                unsigned validDstCol, 
                                unsigned validSrcRow, 
                                unsigned validSrcCol) {
        typename TileDataDst::DType padVal = 0;

        constexpr auto PadVal_ = TileDataDst::PadVal;
        if constexpr (std::numeric_limits<typename TileDataDst::DType>::has_infinity)
        {
            if constexpr(PadVal_ == PadValue::Max)
                padVal = std::numeric_limits<typename TileDataDst::DType>::infinity();
            else if constexpr (PadVal_ == PadValue::Min)
                padVal = -std::numeric_limits<typename TileDataDst::DType>::infinity();
        }
        else 
        {
            if constexpr (PadVal_ == PadValue::Max)
                padVal = std::numeric_limits<typename TileDataDst::DType>::max();
            else if constexpr (PadVal_ == PadValue::Min)
                padVal = std::numeric_limits<typename TileDataDst::DType>::min();
        }

        cpu::parallel_for_1d(0, TileDataDst::Rows, static_cast<std::size_t>(TileDataDst::Rows) * TileDataDst::Cols,
            [&](std::size_t i) {
                PTO_CPU_VECTORIZE_LOOP
                for (std::size_t j = 0; j < TileDataDst::Cols; ++j) {
                    if (i < validSrcRow && j < validSrcCol) {
                        dst[GetTileElementOffset<TileDataDst>(i, j)] = src[GetTileElementOffset<TileDataSrc>(i, j)];
                    } else {
                        dst[GetTileElementOffset<TileDataDst>(i, j)] = padVal;
                    }
                }
            });
    }

    template <typename TileDataDst, typename TileDataSrc, bool inplace>
    PTO_INTERNAL void TFILLPAD_GENERIC_IMPL(TileDataDst &dst, TileDataSrc &src) {
        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataSrc::DType);
        constexpr unsigned srcStride = TileDataSrc::RowStride;
        constexpr unsigned dstStride = TileDataDst::RowStride;
        (void)blockSizeElem;
        (void)srcStride;
        (void)dstStride;
        unsigned validSrcRow = src.GetValidRow();
        unsigned validSrcCol = src.GetValidCol();
        unsigned validDstRow = dst.GetValidRow();
        unsigned validDstCol = dst.GetValidCol();

        using T = typename TileDataSrc::DType;
        using U = typename TileDataDst::DType;
        static_assert(TileDataDst::PadVal != PadValue::Null, "TFillPad, dst vecTile pad value can't be Null!");
        static_assert(sizeof(T) == sizeof(U), "TFillPad, src and dst data type shouuld be the same!");
        static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "TFillPad: Invalid data type!");
        
        if(validDstRow == 0 || validDstCol == 0) {
            return;
        }
        if constexpr (!inplace) 
        {
            TFillPad<TileDataDst, TileDataSrc>(dst.data(), src.data(), validDstRow, validDstCol, validSrcRow, validSrcCol);
        }
        TFillPad<TileDataDst, TileDataSrc>(dst.data(), src.data(), validDstRow, validDstCol, validSrcRow, validSrcCol);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TFILLPAD_IMPL(TileDataDst &dst, TileDataSrc &src) {
        static_assert(TileDataDst::Cols == TileDataSrc::Cols && TileDataDst::Rows == TileDataSrc::Rows,
            "TFillPad: dst and src should have the same rows/cols!");
        TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, false>(dst, src);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TFILLPAD_INPLACE_IMPL(TileDataDst &dst, TileDataSrc &src) {
        static_assert(TileDataDst::Cols == TileDataSrc::Cols && TileDataDst::Rows == TileDataSrc::Rows,
            "TFillPad: dst and src should have the same rows/cols!");
        TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, true>(dst, src);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TFILLPAD_EXPAND_IMPL(TileDataDst &dst, TileDataSrc &src) {
        static_assert(TileDataDst::Cols >= TileDataSrc::Cols && TileDataDst::Rows >= TileDataSrc::Rows,
            "TFillPad: dst rows/cols must cover src rows/cols!");
        TFILLPAD_GENERIC_IMPL<TileDataDst, TileDataSrc, false>(dst, src);
    }

    template <typename TileData, PadValue PadVal = PadValue::Zero>
    PTO_INTERNAL void TFILLPAD_IMPL(TileData &dst, TileData &src)
    {
        (void)PadVal;
        TFILLPAD_GENERIC_IMPL<TileData, TileData, false>(dst, src);
    }
}
#endif
