/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TDIVS_HPP
#define TDIVS_HPP

#include <pto/common/constants.hpp>
#include "pto/npu/a2a3/TBinSOp.hpp"

namespace pto
{
    template <typename T>
    struct SDivOp {
        PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats) {
            if constexpr (std::is_same<T, int32_t>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, 8, 0);
                pipe_barrier(PIPE_V);
                vconv_s322f32(reinterpret_cast<__ubuf__ float *>(dst), dst, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vconv_s322f32(reinterpret_cast<__ubuf__ float *>(src0), src0, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vdiv(reinterpret_cast<__ubuf__ float *>(dst), reinterpret_cast<__ubuf__ float *>(dst), reinterpret_cast<__ubuf__ float *>(src0), repeats, 1, 1, 1, 8, 8, 8);
                pipe_barrier(PIPE_V);
                vconv_f322s32z(dst, reinterpret_cast<__ubuf__ float *>(dst), repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, int16_t>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, 8, 0);
                pipe_barrier(PIPE_V);
                vconv_s162f16(reinterpret_cast<__ubuf__ half *>(dst), dst, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vconv_s162f16(reinterpret_cast<__ubuf__ half *>(src0), src0, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vdiv(reinterpret_cast<__ubuf__ half *>(dst), reinterpret_cast<__ubuf__ half *>(dst), reinterpret_cast<__ubuf__ half *>(src0), repeats, 1, 1, 1, 8, 8, 8);
                pipe_barrier(PIPE_V);
                vconv_f162s16z(dst, reinterpret_cast<__ubuf__ half *>(dst), repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, float>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, 8, 0);
                pipe_barrier(PIPE_V);
                vdiv(dst, dst, src0, repeats, 1, 1, 1, 8, 8, 8);
            }
            else if constexpr (std::is_same<T, half>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, 8, 0);
                pipe_barrier(PIPE_V);
                vdiv(dst, dst, src0, repeats, 1, 1, 1, 8, 8, 8);
            }
        }
        PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats, uint8_t dstRepeatStride, uint8_t srcRepeatStride) {
            if constexpr (std::is_same<T, int32_t>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, dstRepeatStride, 0);
                pipe_barrier(PIPE_V);
                vconv_s322f32(reinterpret_cast<__ubuf__ float *>(dst), dst, repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
                vconv_s322f32(reinterpret_cast<__ubuf__ float *>(src0), src0, repeats, 1, 1, srcRepeatStride, srcRepeatStride);
                pipe_barrier(PIPE_V);
                vdiv(reinterpret_cast<__ubuf__ float *>(dst), reinterpret_cast<__ubuf__ float *>(dst), reinterpret_cast<__ubuf__ float *>(src0), repeats, 1, 1, 1, dstRepeatStride, dstRepeatStride, srcRepeatStride);
                pipe_barrier(PIPE_V);
                vconv_f322s32z(dst, reinterpret_cast<__ubuf__ float *>(dst), repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, int16_t>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, dstRepeatStride, 0);
                pipe_barrier(PIPE_V);
                vconv_s162f16(reinterpret_cast<__ubuf__ half *>(dst), dst, repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
                vconv_s162f16(reinterpret_cast<__ubuf__ half *>(src0), src0, repeats, 1, 1, srcRepeatStride, srcRepeatStride);
                pipe_barrier(PIPE_V);
                vdiv(reinterpret_cast<__ubuf__ half *>(dst), reinterpret_cast<__ubuf__ half *>(dst), reinterpret_cast<__ubuf__ half *>(src0), repeats, 1, 1, 1, dstRepeatStride, dstRepeatStride, srcRepeatStride);
                pipe_barrier(PIPE_V);
                vconv_f162s16z(dst, reinterpret_cast<__ubuf__ half *>(dst), repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, float>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, dstRepeatStride, 0);
                pipe_barrier(PIPE_V);
                vdiv(dst, dst, src0, repeats, 1, 1, 1, dstRepeatStride, dstRepeatStride, srcRepeatStride);
            }
            else if constexpr (std::is_same<T, half>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, dstRepeatStride, 0);
                pipe_barrier(PIPE_V);
                vdiv(dst, dst, src0, repeats, 1, 1, 1, dstRepeatStride, dstRepeatStride, srcRepeatStride);
            }
        }
    };
    
    template <typename T> 
    struct DivSOp {
        PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats) {
            float divider = static_cast<float>(src1);
            if (divider != 0.0f)
            {
                divider = 1.0f / divider;
            }
            else
            {
                divider = 1.0 / 0.0;
            }
            if constexpr (std::is_same<T, int32_t>::value)
            {
                vconv_s322f32(reinterpret_cast<__ubuf__ float *>(dst), src0, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vmuls(reinterpret_cast<__ubuf__ float *>(dst), reinterpret_cast<__ubuf__ float *>(dst), divider, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vconv_f322s32z(dst, reinterpret_cast<__ubuf__ float *>(dst), repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, int16_t>::value)
            {
                vconv_s162f16(reinterpret_cast<__ubuf__ half *>(dst), src0, repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vmuls(reinterpret_cast<__ubuf__ half *>(dst), reinterpret_cast<__ubuf__ half *>(dst), static_cast<half>(divider), repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
                vconv_f162s16z(dst, reinterpret_cast<__ubuf__ half *>(dst), repeats, 1, 1, 8, 8);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, half>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, 8, 0);
                pipe_barrier(PIPE_V);
                vdiv(dst, src0, dst, repeats, 1, 1, 1, 8, 8, 8);
            }
            else
            {
                vmuls(dst, src0, divider, repeats, 1, 1, 8, 8);
            }
        }
        PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats, uint8_t dstRepeatStride, uint8_t srcRepeatStride) {
            float divider = static_cast<float>(src1);
            if (divider != 0.0f)
            {
                divider = 1.0f / divider;
            }
            else
            {
                divider = 1.0 / 0.0;
            }
            if constexpr (std::is_same<T, int32_t>::value)
            {
                vconv_s322f32(reinterpret_cast<__ubuf__ float *>(dst), src0, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
                pipe_barrier(PIPE_V);
                vmuls(reinterpret_cast<__ubuf__ float *>(dst), reinterpret_cast<__ubuf__ float *>(dst), divider, repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
                vconv_f322s32z(dst, reinterpret_cast<__ubuf__ float *>(dst), repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, int16_t>::value)
            {
                vconv_s162f16(reinterpret_cast<__ubuf__ half *>(dst), src0, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
                pipe_barrier(PIPE_V);
                vmuls(reinterpret_cast<__ubuf__ half *>(dst), reinterpret_cast<__ubuf__ half *>(dst), static_cast<half>(divider), repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
                vconv_f162s16z(dst, reinterpret_cast<__ubuf__ half *>(dst), repeats, 1, 1, dstRepeatStride, dstRepeatStride);
                pipe_barrier(PIPE_V);
            }
            else if constexpr (std::is_same<T, half>::value)
            {
                vector_dup(dst, src1, repeats, 1, 1, dstRepeatStride, 0);
                pipe_barrier(PIPE_V);
                vdiv(dst, src0, dst, repeats, 1, 1, 1, dstRepeatStride, srcRepeatStride, dstRepeatStride);
            }
            else
            {
                vmuls(dst, src0, divider, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
            }
        }
    };
    template <typename T, unsigned Cols>
    PTO_INTERNAL void TDivs_naive(__ubuf__ T *dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        PtoSetWaitFlag<PIPE_V, PIPE_S>();
        for (int row = 0; row < validRow; row++) {
            for (int col = 0; col < validCol; col++) {
                int idx = row * Cols + col;
                dst[idx] = src0[idx] / src1;
            }
        }
        PtoSetWaitFlag<PIPE_S, PIPE_V>();
    }

    template <typename T, unsigned Cols>
    PTO_INTERNAL void TSDiv_naive(__ubuf__ T *dst, __ubuf__ T* src0, T src1, unsigned validRow, unsigned validCol) {
        PtoSetWaitFlag<PIPE_V, PIPE_S>();
        for (int row = 0; row < validRow; row++) {
            for (int col = 0; col < validCol; col++) {
                int idx = row * Cols + col;
                dst[idx] = src1 / src0[idx];
            }
        }
        PtoSetWaitFlag<PIPE_S, PIPE_V>();
    }

    template <typename T, typename TileDataDst, typename TileDataSrc>
    __tf__ PTO_INTERNAL void TDivS(typename TileDataDst::TileDType __out__ dstData,
                                   typename TileDataSrc::TileDType __in__ srcData,
                                   T __in__ scalar, unsigned validRow, unsigned validCol) {
        __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
        __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
        constexpr unsigned elementsPerRepeat = pto::REPEAT_BYTE / sizeof(T);
        constexpr unsigned blockSizeElem = pto::BLOCK_BYTE_SIZE / sizeof(T);
        constexpr unsigned dstStride = TileDataDst::RowStride;
        constexpr unsigned srcStride = TileDataSrc::RowStride;
        TBinSInstr<DivSOp<T>, TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>
            (dst, src, scalar, validRow, validCol);
    }

    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TDIVS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
    {
        using T = typename TileDataSrc::DType;
        static_assert(std::is_same_v<T, typename TileDataDst::DType>,
            "TDIVS: The data type of dst must be consistent with src.");
        static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value ||
                      std::is_same<T, int16_t>::value || std::is_same<T, half>::value ||
                      std::is_same<T, float16_t>::value || std::is_same<T, float>::value ||
                      std::is_same<T, float32_t>::value, "TDIVS: Invalid data type");

        static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataDst::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataSrc::ValidCol <= TileDataSrc::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataSrc::ValidRow <= TileDataSrc::Rows, "Number of valid rows must not be greater than number of tile rows.");
        static_assert(TileDataDst::ValidCol <= TileDataDst::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataDst::ValidRow <= TileDataDst::Rows, "Number of valid rows must not be greater than number of tile rows.");


        PTO_ASSERT(src.GetValidCol() == dst.GetValidCol(), "Number of cols of src and dst must be the same.");
        PTO_ASSERT(src.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");

        unsigned dstValidRow = dst.GetValidRow();
        unsigned dstValidCol = dst.GetValidCol();
        if ((dstValidRow != 0 && dstValidCol != 0) &&
            (dstValidRow == src.GetValidRow() && dstValidCol == src.GetValidCol())) {
            TDivS<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dstValidRow, dstValidCol);
        } else {
            PTO_ASSERT(false, "TDIVS: dstTile validRow/validCol must be consistent with of src.");
        }
    }

    template <typename T, typename TileDataDst, typename TileDataSrc>
    __tf__ PTO_INTERNAL void TSDiv(typename TileDataDst::TileDType __out__ dstData,
        typename TileDataSrc::TileDType __in__ srcData, T __in__ scalar, unsigned validRow, unsigned validCol) {
        __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
        __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
        constexpr unsigned elementsPerRepeat = pto::REPEAT_BYTE / sizeof(T);
        constexpr unsigned blockSizeElem = pto::BLOCK_BYTE_SIZE / sizeof(T);
        constexpr unsigned dstStride = TileDataDst::RowStride;
        constexpr unsigned srcStride = TileDataSrc::RowStride;
        TBinSInstr<SDivOp<T>, TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>
            (dst, src, scalar, validRow, validCol);
    }
    template <typename TileDataDst, typename TileDataSrc>
    PTO_INTERNAL void TDIVS_IMPL(TileDataDst &dst, typename TileDataDst::DType scalar, TileDataSrc &src) {
        using T = typename TileDataSrc::DType;
        static_assert(std::is_same_v<T, typename TileDataDst::DType>,
            "TDIVS: The data type of dst must be consistent with src.");
        static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value ||
            std::is_same<T, int16_t>::value || std::is_same<T, half>::value || std::is_same<T, float16_t>::value ||
            std::is_same<T, float>::value || std::is_same<T, float32_t>::value, "TDIVS: Invalid data type");

        static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataDst::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
        static_assert(TileDataSrc::ValidCol <= TileDataSrc::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataSrc::ValidRow <= TileDataSrc::Rows, "Number of valid rows must not be greater than number of tile rows.");
        static_assert(TileDataDst::ValidCol <= TileDataDst::Cols, "Number of valid columns must not be greater than number of tile columns.");
        static_assert(TileDataDst::ValidRow <= TileDataDst::Rows, "Number of valid rows must not be greater than number of tile rows.");

        PTO_ASSERT(src.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
        PTO_ASSERT(src.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");

        unsigned dstValidRow = dst.GetValidRow();
        unsigned dstValidCol = dst.GetValidCol();
        if ((dstValidRow != 0 && dstValidCol != 0) &&
            (dstValidRow == src.GetValidRow() && dstValidCol == src.GetValidCol())) {
            TSDiv<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dstValidRow, dstValidCol);
        } else {
            PTO_ASSERT(false, "TDIVS: dstTile validRow/validCol must be consistent with of src.");
        }
    }
}

#endif