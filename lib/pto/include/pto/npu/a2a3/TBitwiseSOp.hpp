/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef T_BITWISE_SCALAR_OP_HPP
#define T_BITWISE_SCALAR_OP_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

#include "TBinSOp.hpp"

namespace pto {

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TShiftCheck(const TileDataDst &dst, const TileDataSrc &src)
{
    using T = typename TileDataSrc::DType;
    static_assert(std::is_same_v<T, typename TileDataDst::DType>, "The data type of dst must be consistent with src.");
    static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value || std::is_same<T, int16_t>::value ||
                      std::is_same<T, uint32_t>::value || std::is_same<T, uint16_t>::value ||
                      std::is_same<T, unsigned int>::value,
        "Invalid data type");
    static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src and dst tiles must be TileType::Vec.");
    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();
    unsigned srcValidRow = src.GetValidRow();
    unsigned srcValidCol = src.GetValidCol();
    PTO_ASSERT((dstValidRow != 0 && dstValidCol != 0), "Number of cols of dst must be non-zero.");
    PTO_ASSERT((srcValidRow != 0 && srcValidCol != 0), "Number of cols of src must be non-zero.");
    PTO_ASSERT(dstValidCol == srcValidCol, "Number of cols of src and dst must be the same.");
    PTO_ASSERT(dstValidRow == srcValidRow, "Number of rows of src and dst must be the same.");
}

template <typename Op, typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TShiftS(typename TileDataDst::TileDType __out__ dstData, typename TileDataSrc::TileDType __in__ srcData,
    typename TileDataSrc::DType __in__ scalar, unsigned validRow, unsigned validCol)
{
    using T = typename TileDataSrc::DType;
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    constexpr unsigned elementsPerRepeat = pto::REPEAT_BYTE / sizeof(T);
    constexpr unsigned blockSizeElem = pto::BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned dstStride = TileDataDst::RowStride;
    constexpr unsigned srcStride = TileDataSrc::RowStride;
    TBinSInstr<Op, TileDataDst, TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(
        dst, src, scalar, validRow, validCol);
}

template <typename T>
struct ShrSOp {
    PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats)
    {
        vshr(dst, src0, src1, repeats, 1, 1, 8, 8, false);
    }
    PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats, uint8_t dstRepeatStride,
        uint8_t srcRepeatStride)
    {
        vshr(dst, src0, src1, repeats, 1, 1, dstRepeatStride, srcRepeatStride, false);
    }
};

template <typename T>
struct ShlSOp {
    PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats)
    {
        vshl(dst, src0, src1, repeats, 1, 1, 8, 8);
    }
    PTO_INTERNAL static void BinSInstr(__ubuf__ T *dst, __ubuf__ T *src0, T src1, uint8_t repeats, uint8_t dstRepeatStride,
        uint8_t srcRepeatStride)
    {
        vshl(dst, src0, src1, repeats, 1, 1, dstRepeatStride, srcRepeatStride);
    }
};

template <typename T, typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TSHLS32(typename TileDataDst::TileDType __out__ dstData, typename TileDataSrc::TileDType __in__ srcData,
    T scalar, unsigned validRow, unsigned validCol)
{
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            dst[r * TileDataDst::RowStride + c] = static_cast<T>(src[r * TileDataSrc::RowStride + c] << scalar);
        }
    }
}

template <typename T, typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TSHRS32(typename TileDataDst::TileDType __out__ dstData, typename TileDataSrc::TileDType __in__ srcData,
    T scalar, unsigned validRow, unsigned validCol)
{
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            dst[r * TileDataDst::RowStride + c] = static_cast<T>(src[r * TileDataSrc::RowStride + c] >> scalar);
        }
    }
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TSHLS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
{
    using T = typename TileDataSrc::DType;
    if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
        static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TSHLS b32 fallback only supports non-boxed layouts.");
        TSHLS32<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(), dst.GetValidCol());
        return;
    }
    TShiftCheck(dst, src);
    TShiftS<ShlSOp<typename TileDataSrc::DType>, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(),
        dst.GetValidCol());
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TSHRS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
{
    using T = typename TileDataSrc::DType;
    if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
        static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TSHRS b32 fallback only supports non-boxed layouts.");
        TSHRS32<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(), dst.GetValidCol());
        return;
    }
    TShiftCheck(dst, src);
    TShiftS<ShrSOp<typename TileDataSrc::DType>, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(),
        dst.GetValidCol());
}

template <typename T, typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TANDS32(typename TileDataDst::TileDType __out__ dstData, typename TileDataSrc::TileDType __in__ srcData,
    T scalar, unsigned validRow, unsigned validCol)
{
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            dst[r * TileDataDst::RowStride + c] = src[r * TileDataSrc::RowStride + c] & scalar;
        }
    }
}

template <typename T, typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TORS32(typename TileDataDst::TileDType __out__ dstData, typename TileDataSrc::TileDType __in__ srcData, T scalar,
    unsigned validRow, unsigned validCol)
{
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            dst[r * TileDataDst::RowStride + c] = src[r * TileDataSrc::RowStride + c] | scalar;
        }
    }
}

template <typename T, typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TXORS32(typename TileDataDst::TileDType __out__ dstData, typename TileDataSrc::TileDType __in__ srcData, T scalar,
    unsigned validRow, unsigned validCol)
{
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned c = 0; c < validCol; ++c) {
            dst[r * TileDataDst::RowStride + c] = src[r * TileDataSrc::RowStride + c] ^ scalar;
        }
    }
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TANDS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
{
    using T = typename TileDataSrc::DType;
    if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
        static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TANDS b32 fallback only supports non-boxed layouts.");
        TANDS32<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(), dst.GetValidCol());
        return;
    }
    TEXPANDS_IMPL(dst, scalar);
    TAND_IMPL(dst, src, dst);
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TORS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar)
{
    using T = typename TileDataSrc::DType;
    if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
        static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TORS b32 fallback only supports non-boxed layouts.");
        TORS32<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(), dst.GetValidCol());
        return;
    }
    TEXPANDS_IMPL(dst, scalar);
    TOR_IMPL(dst, src, dst);
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataTmp>
PTO_INTERNAL void TXORS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar, TileDataTmp &tmp)
{
    using T = typename TileDataSrc::DType;
    if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
        (void)tmp;
        static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc::SFractal == SLayout::NoneBox,
            "Fix: TXORS b32 fallback only supports non-boxed layouts.");
        TXORS32<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), scalar, dst.GetValidRow(), dst.GetValidCol());
        return;
    }
    TORS_IMPL(dst, src, scalar);
    pipe_barrier(PIPE_V);
    TANDS_IMPL(tmp, src, scalar);
    pipe_barrier(PIPE_V);
    TNOT_IMPL(tmp, tmp);
    pipe_barrier(PIPE_V);
    TAND_IMPL(dst, dst, tmp);
}

template <typename TileData>
__tf__ PTO_INTERNAL typename TileData::DType ReadScalar0(typename TileData::TileDType __in__ src)
{
    __ubuf__ typename TileData::DType *ptr = (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(src);
    return ptr[0];
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TSHL_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1)
{
    using T = typename TileDataSrc0::DType;
    static_assert(std::is_same_v<T, typename TileDataDst::DType>, "TSHL: dst/src0 dtype mismatch");
    static_assert(std::is_same_v<T, typename TileDataSrc1::DType>, "TSHL: shift dtype mismatch");
    static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc0::Loc == TileType::Vec && TileDataSrc1::Loc == TileType::Vec,
        "TSHL: only supports Vec tiles");

    const T sh = ReadScalar0<TileDataSrc1>(src1.data());
    TSHLS_IMPL(dst, src0, sh);
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TSHR_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1)
{
    using T = typename TileDataSrc0::DType;
    static_assert(std::is_same_v<T, typename TileDataDst::DType>, "TSHR: dst/src0 dtype mismatch");
    static_assert(std::is_same_v<T, typename TileDataSrc1::DType>, "TSHR: shift dtype mismatch");
    static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc0::Loc == TileType::Vec && TileDataSrc1::Loc == TileType::Vec,
        "TSHR: only supports Vec tiles");

    const T sh = ReadScalar0<TileDataSrc1>(src1.data());
    TSHRS_IMPL(dst, src0, sh);
}

} // namespace pto

#endif
