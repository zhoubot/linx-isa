/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TEXPANDS_HPP
#define TEXPANDS_HPP

#include <pto/common/constants.hpp>
#include "TBinSOp.hpp"

namespace pto
{
    template<typename T>
    struct ExpandSOp {
        PTO_INTERNAL static void BinSInstr(__ubuf__ T* dst, __ubuf__ T* src0, T scalar, uint8_t repeats) {
            vector_dup(dst, scalar, repeats, 1, 1, 8, 8);
        }
        PTO_INTERNAL static void BinSInstr(__ubuf__ T* dst, __ubuf__ T* src0, T scalar, uint8_t repeats, uint8_t dstRepeatStride, uint8_t srcRepeatStride) {
            vector_dup(dst, scalar, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
        }
    };
    template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned stride>
    __tf__ PTO_INTERNAL void TExpandS(typename TileData::TileDType __out__ dst,
                                typename TileData::DType __in__ scalar,
                                unsigned validRow,
                                unsigned validCol) 
    {
        using T = typename TileData::DType;
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);

        TBinSInstr<ExpandSOp<typename TileData::DType>, TileData, TileData, elementsPerRepeat, blockSizeElem, stride,
            stride>(dstPtr, dstPtr, scalar, validRow, validCol);
    }
    template <typename TileData>
    PTO_INTERNAL void TEXPANDS_IMPL(TileData &dst, typename TileData::DType scalar)
    {
        static_assert(TileData::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileData::ValidCol <= TileData::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileData::ValidRow <= TileData::Rows, "Number of valid rows must not be greater than number of tile rows.");
        static_assert(TileData::isRowMajor, "TEXPANDS: not supported Layout type.");

        constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileData::DType);
        constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileData::DType);
        unsigned numRepeatPerLine = dst.GetValidCol() / elementsPerRepeat;
        unsigned numRemainPerLine = dst.GetValidCol() % elementsPerRepeat;
        constexpr unsigned stride = TileData::RowStride;
        unsigned validRow = dst.GetValidRow();
        unsigned validCol = dst.GetValidCol();
        TExpandS<TileData, elementsPerRepeat, blockSizeElem, stride>(dst.data(), scalar, validRow, validCol);
    }
}

#endif