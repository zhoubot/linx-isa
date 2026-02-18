/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TSORT32_HPP
#define TSORT32_HPP

#include <algorithm>
#include <type_traits>
#include <vector>
#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto {
constexpr const int sortNum = 32;
constexpr const int floatStride = 1;
constexpr const int halfStride = 2;
constexpr const int halfOffset = 16;
constexpr const int totalByte = 8;

template<typename T>
struct ScoreIndexPair {
    T score;
    uint32_t index;
    
    // 用于降序排序的比较函数（稳定排序）
    bool operator<(const ScoreIndexPair& other) const {
        // 降序排序：分数大的在前
        if (score != other.score) {
            return score > other.score;  // 降序
        }
        // 分数相同时，按原始索引升序（i<j时优先存储i）
        return index < other.index;
    }
};

template<typename T, typename TileDataDst, typename TileDataSrc, typename TileDataIdx>
PTO_INTERNAL void TSort32(typename TileDataDst::TileDType dst, typename TileDataSrc::TileDType src,
                          typename TileDataIdx::TileDType idx, int validRow, int validCol)
{
    for (int i = 0; i < validRow; i++) {
        for (int j = 0; j < validCol; j += sortNum) {
            const size_t dstOffset = GetTileElementOffset<TileDataDst>(i, 2 * j);
            const size_t srcOffset = GetTileElementOffset<TileDataSrc>(i, j);
            const size_t idxOffset = GetTileElementOffset<TileDataIdx>(i, j);
            int validNum = std::min(sortNum, validCol - j);
            // 收集当前分段的分数-索引对
            std::vector<ScoreIndexPair<T>> segment(validNum);
            for (int k = 0; k < validNum; k++) {
                segment[k].score = src[srcOffset + k];
                segment[k].index = (uint32_t)(idx[idxOffset + k]);
            }

            // 对当前分段进行稳定排序（降序）
            // 使用稳定排序以保持相同分数的原始顺序
            std::stable_sort(segment.begin(), segment.end(),
                [](const ScoreIndexPair<T>& a, const ScoreIndexPair<T>& b) {
                    // 主要按分数降序排序
                    if (a.score != b.score) {
                        return a.score > b.score;  // 降序
                    }
                    // 分数相同时，按原始索引升序（i<j时优先存储i）
                    return a.index < b.index;
                }
            );

            int num = 0;
            int t = 0;
            while (num < validNum) {
                if constexpr (sizeof(T) == sizeof(half)) {
                    dst[dstOffset + t] = segment[num].score;
                    dst[dstOffset + t + 1] = 0;
                    dst[dstOffset + t + halfStride] = segment[num].index;
                    dst[dstOffset + t + halfStride + 1] = segment[num].index >> halfOffset;
                } else {
                    dst[dstOffset + t] = segment[num].score;
                    dst[dstOffset + t + 1] = segment[num].index;
                }
                num++;
                t += totalByte / sizeof(T);
            }
        }
    }
}

template<typename TileDataDst, typename TileDataSrc, typename TileDataIdx>
PTO_INTERNAL void TSORT32_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataIdx &idx)
{
    using T = typename TileDataSrc::DType;
    static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, half> ||
                  std::is_same_v<T, float>, "TSORT32: Invalid data type.");
    static_assert(std::is_same_v<typename TileDataDst::DType, T>,
                  "The Src data type must be consistent with the dst data type");
    static_assert(std::is_same_v<typename TileDataIdx::DType, uint32_t>, "The Idx data type must be uint32");
    static_assert(TileDataSrc::RowStride == TileDataIdx::RowStride,
                  "The Src stride must be consistent with the idx stride");
    static_assert(TileDataSrc::isRowMajor && TileDataIdx::isRowMajor && TileDataDst::isRowMajor,
                  "TSORT32: only RowMajor tiles are supported in CPU sim");
    static_assert(TileDataSrc::SFractal == SLayout::NoneBox && TileDataIdx::SFractal == SLayout::NoneBox &&
                      TileDataDst::SFractal == SLayout::NoneBox,
                  "TSORT32: only NoneBox tiles are supported in CPU sim");

    const int validRow = src.GetValidRow();
    const int validCol = src.GetValidCol();
    if (validRow == 0 || validCol == 0) {
        return;
    }
    if (validRow != idx.GetValidRow() || validCol != idx.GetValidCol()) {
        return;
    }
    TSort32<T, TileDataDst, TileDataSrc, TileDataIdx>(dst.data(), src.data(), idx.data(), validRow, validCol);
}

template<typename TileDataDst, typename TileDataSrc, typename TileDataIdx, typename TileDataTmp>
PTO_INTERNAL void TSORT32_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataIdx &idx, TileDataTmp &tmp)
{
    (void)tmp;
    TSORT32_IMPL(dst, src, idx);
}
}
#endif
