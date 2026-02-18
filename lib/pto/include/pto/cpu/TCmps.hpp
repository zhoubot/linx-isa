/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCMPS_HPP
#define TCMPS_HPP
#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"
#include <cmath>
#include <vector>

namespace pto {

const int32_t CMP_BITS_PER_INDEX = 32;

template <typename T>
AICORE uint8_t CmpCall(T a, T b, CmpMode cmpMode)
{
    uint8_t res = 0;
    const double diff = static_cast<double>(a) - static_cast<double>(b);
    switch (static_cast<CmpMode>(cmpMode)) {
        case CmpMode::EQ:
            res = (std::fabs(diff) < 1e-9);
            break;
        case CmpMode::NE:
            res = (std::fabs(diff) > 1e-9);
            break;
        case CmpMode::LT:
            res = (a < b);
            break;
        case CmpMode::GT:
            res = (a > b);
            break;
        case CmpMode::GE:
            res = (a >= b);
            break;
        case CmpMode::LE:
            res = (a <= b);
            break;
        default:
            res = (std::fabs(diff) < 1e-9);
            break;
    }
    return res;
}


template <typename TileDataDst, typename TileDataSrc, typename T>
AICORE void TCmps(
    typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src0, 
    T src1, 
    CmpMode mode, 
    unsigned srcValidRow,  
    unsigned srcValidCol,  
    unsigned dstValidRow,  
    unsigned dstValidCol,  
    unsigned dstStride,  
    unsigned srcStride
) 
{
    size_t H = TileDataSrc::Rows;
    size_t W = TileDataSrc::Cols;
    std::vector<uint8_t> golden(H * W, 0);

    for (size_t i = 0; i < srcValidRow; i++)
    {
        for (size_t j = 0; j < srcValidCol; j++)
        {
            T a = src0[i * srcStride + j];
            golden[i * W + j] = CmpCall<T>(a, src1, mode);
        }
    }

    std::vector<uint8_t> out_uint8;
    size_t bits_per_row = W / 8;

    for (size_t h = 0; h < H; ++h) {
        for (size_t i = 0; i < bits_per_row; ++i) {
            uint8_t packed_byte = 0;
            for (size_t bit = 0; bit < 8; ++bit) {
                // Get the bit from the golden array and shift it into position
                uint8_t bit_val = golden[h * W + (i * 8 + bit)];
                packed_byte |= (bit_val << bit);
            }
            out_uint8.push_back(packed_byte);
        }
    }

    int c = 0;
    for (size_t i = 0; i < dstValidRow && c < out_uint8.size(); i++)
    {
        for (size_t j = 0; j < dstValidCol && c < out_uint8.size(); j++)
        {
            dst[i * W + j] = out_uint8[c++];
            uint8_t b = dst[i * W + j];
        }
    }
}

template <typename TileDataDst, typename TileDataSrc, typename T>
PTO_INTERNAL void TCMPS_IMPL(TileDataDst &dst, TileDataSrc &src0, T src1, CmpMode cmpMode) {

    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();

    unsigned srcValidRow = src0.GetValidRow();
    unsigned srcValidCol = src0.GetValidCol();

    unsigned dstStride = TileDataDst::RowStride;
    unsigned srcStride = TileDataSrc::RowStride;

    TCmps<TileDataDst, TileDataSrc, T>(dst.data(), src0.data(), src1, cmpMode, srcValidRow, srcValidCol, dstValidRow, dstValidCol, dstStride, srcStride);
}
}
#endif
