/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSCATTER_HPP
#define TSCATTER_HPP

#include <pto/common/constants.hpp>

namespace pto 
{
    template <typename TileDataD, typename TileDataS, typename TileDataI>
    __tf__ PTO_INTERNAL void TScatterImpl(
        typename TileDataD::TileDType __out__ dst,
        typename TileDataS::TileDType __in__ src,
        typename TileDataI::TileDType __in__ idx,
        unsigned validRow,
        unsigned validCol
    ) {
        using TD = typename TileDataD::DType;
        using TS = typename TileDataS::DType;
        using TI = typename TileDataI::DType;
        __ubuf__ TD *dstPtr = (__ubuf__ TD *)__cce_get_tile_ptr(dst);
        __ubuf__ TS *srcPtr = (__ubuf__ TS *)__cce_get_tile_ptr(src);
        __ubuf__ TI *indPtr = (__ubuf__ TI *)__cce_get_tile_ptr(idx);

        for (int i = 0; i < validRow; i++) {
            for (int j = 0; j < validCol; j++) {
                TI ix = *(indPtr + i * TileDataI::Cols + j);
                dstPtr[ix] = srcPtr[i * TileDataS::Cols + j];
            }
        }
    }

    template <typename TileDataD, typename TileDataS, typename TileDataI>
    PTO_INTERNAL void TSCATTER_IMPL(TileDataD &dst, TileDataS &src, TileDataI &idx)
    {
        using TD = typename TileDataD::DType;
        using TS = typename TileDataS::DType;
        using TI = typename TileDataI::DType;
        static_assert(std::is_same<TD, int32_t>::value ||
                      std::is_same<TD, int16_t>::value ||
                      std::is_same<TD, int8_t>::value ||
                      std::is_same<TD, uint32_t>::value ||
                      std::is_same<TD, uint16_t>::value ||
                      std::is_same<TD, uint8_t>::value ||
                      std::is_same<TD, half>::value ||
                      std::is_same<TD, float16_t>::value ||
                      std::is_same<TD, float32_t>::value ||
                      std::is_same<TD, bfloat16_t>::value,
                      "TSCATTER: Invalid data type.");
        static_assert(std::is_same<TD, TS>::value,
                      "TSCATTER: Data type of dst and src must be the same.");
        static_assert((sizeof(TD) == 4 && sizeof(TI) == 4) ||
                      (sizeof(TD) == 2 && sizeof(TI) == 2) ||
                      (sizeof(TD) == 1 && sizeof(TI) == 2),
                      "TSCATTER: Invalid data type of idx.");
        static_assert(std::is_same<TI, uint16_t>::value ||
                      std::is_same<TI, uint32_t>::value ||
                      std::is_same<TI, int16_t>::value ||
                      std::is_same<TI, int32_t>::value,
                      "TSCATTER: Invalid data type of idx.");
        static_assert(TileDataD::Loc == TileType::Vec &&
                      TileDataS::Loc == TileType::Vec &&
                      TileDataI::Loc == TileType::Vec,
                      "TSCATTER: TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataD::ValidCol <= TileDataD::Cols &&
                      TileDataS::ValidCol <= TileDataS::Cols &&
                      TileDataI::ValidCol <= TileDataI::Cols,
                      "TSCATTER: Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataD::ValidRow <= TileDataD::Rows &&
                      TileDataS::ValidRow <= TileDataS::Rows &&
                      TileDataI::ValidRow <= TileDataI::Rows,
                      "TSCATTER: Number of valid rows must not be greater than number of tile rows.");

        unsigned validRow = idx.GetValidRow();
        unsigned validCol = idx.GetValidCol();
        TScatterImpl<TileDataD, TileDataS, TileDataI>(dst.data(), src.data(), idx.data(), validRow, validCol);
    }
}

#endif