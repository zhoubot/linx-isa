/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TMOV_HPP
#define TMOV_HPP

#include <cassert>
#include <algorithm>
#include <pto/common/constants.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto
{
    template <typename DstTileData, typename SrcTileData>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src) {
        assert (src.GetValidRow() == dst.GetValidRow() && src.GetValidRow() == dst.GetValidRow());
        for(size_t c=0; c<src.GetValidCol(); c++) {
            size_t subTileSrcC = c / SrcTileData::InnerCols;
            size_t innerSrcC = c % SrcTileData::InnerCols;
            size_t subTileDstC = c / DstTileData::InnerCols;
            size_t innerDstC = c % DstTileData::InnerCols;

            for(size_t r=0; r<src.GetValidRow(); r++) {
                size_t srcTileIdx;
                size_t dstTileIdx;
                if constexpr (SrcTileData::SFractal == SLayout::NoneBox) {
                    srcTileIdx = GetTileElementOffsetPlain<SrcTileData>(r,c);
                } else {
                    size_t subTileR = r / SrcTileData::InnerRows;
                    size_t innerR = r % SrcTileData::InnerRows;
                    srcTileIdx = GetTileElementOffsetSubfractals<SrcTileData>(subTileR,innerR,subTileSrcC,innerSrcC);
                }

                if constexpr (DstTileData::SFractal == SLayout::NoneBox) {
                    dstTileIdx = GetTileElementOffsetPlain<DstTileData>(r,c);
                } else {
                    size_t subTileR = r / DstTileData::InnerRows;
                    size_t innerR = r % DstTileData::InnerRows;
                    dstTileIdx = GetTileElementOffsetSubfractals<DstTileData>(subTileR,innerR,subTileDstC,innerDstC);
                }
                dst.data()[dstTileIdx] = src.data()[srcTileIdx];
            }
        }
    }

    template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src) {
        TMOV_IMPL(dst, src);
        if constexpr (reluMode == ReluPreMode::NormalRelu) {
            const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
            const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
            for (std::size_t r = 0; r < rows; ++r) {
                for (std::size_t c = 0; c < cols; ++c) {
                    auto &v = dst.data()[GetTileElementOffset<DstTileData>(r, c)];
                    if (v < static_cast<typename DstTileData::DType>(0)) {
                        v = static_cast<typename DstTileData::DType>(0);
                    }
                }
            }
        }
    }

    template <typename DstTileData, typename SrcTileData, AccToVecMode mode,
              ReluPreMode reluMode = ReluPreMode::NoRelu>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src) {
        (void)mode;
        TMOV_IMPL<DstTileData, SrcTileData, reluMode>(dst, src);
    }

    template <typename DstTileData, typename SrcTileData, typename FpTileData,
              ReluPreMode reluMode = ReluPreMode::NoRelu>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src, FpTileData &fp) {
        (void)fp;
        TMOV_IMPL<DstTileData, SrcTileData, reluMode>(dst, src);
    }

    template <typename DstTileData, typename SrcTileData, typename FpTileData, AccToVecMode mode,
              ReluPreMode reluMode = ReluPreMode::NoRelu>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src, FpTileData &fp) {
        (void)mode;
        (void)fp;
        TMOV_IMPL<DstTileData, SrcTileData, reluMode>(dst, src);
    }

    template <typename DstTileData, typename SrcTileData, ReluPreMode reluMode = ReluPreMode::NoRelu>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src, uint64_t preQuantScalar) {
        (void)preQuantScalar;
        TMOV_IMPL<DstTileData, SrcTileData, reluMode>(dst, src);
    }

    template <typename DstTileData, typename SrcTileData, AccToVecMode mode,
              ReluPreMode reluMode = ReluPreMode::NoRelu>
    PTO_INTERNAL void TMOV_IMPL(DstTileData &dst, SrcTileData &src, uint64_t preQuantScalar) {
        (void)mode;
        (void)preQuantScalar;
        TMOV_IMPL<DstTileData, SrcTileData, reluMode>(dst, src);
    }
}
#endif  // TMOV_HPP
