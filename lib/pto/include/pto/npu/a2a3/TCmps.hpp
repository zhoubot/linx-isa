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

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

namespace pto {

constexpr const uint64_t NUM_BITS_IN_BYTE = 8;

template <typename T>
PTO_INTERNAL bool CmpScalarS(T a, T b, CmpMode cmpMode)
{
    if constexpr (std::is_same_v<T, half> || std::is_same_v<T, float16_t>) {
        const float af = static_cast<float>(a);
        const float bf = static_cast<float>(b);
        switch (cmpMode) {
            case CmpMode::EQ: return af == bf;
            case CmpMode::NE: return af != bf;
            case CmpMode::LT: return af < bf;
            case CmpMode::GT: return af > bf;
            case CmpMode::GE: return af >= bf;
            case CmpMode::LE: return af <= bf;
            default: return af == bf;
        }
    } else if constexpr (std::is_floating_point_v<T>) {
        switch (cmpMode) {
            case CmpMode::EQ: return a == b;
            case CmpMode::NE: return a != b;
            case CmpMode::LT: return a < b;
            case CmpMode::GT: return a > b;
            case CmpMode::GE: return a >= b;
            case CmpMode::LE: return a <= b;
            default: return a == b;
        }
    } else {
        switch (cmpMode) {
            case CmpMode::EQ: return a == b;
            case CmpMode::NE: return a != b;
            case CmpMode::LT: return a < b;
            case CmpMode::GT: return a > b;
            case CmpMode::GE: return a >= b;
            case CmpMode::LE: return a <= b;
            default: return a == b;
        }
    }
}

template <typename TileDataDst, typename TileDataSrc0, typename ScalarT>
__tf__ PTO_INTERNAL void TCmpsScalarNpu(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc0::TileDType __in__ src0, ScalarT scalar, CmpMode cmpMode, unsigned validRow,
    unsigned validCol, unsigned validMaskCol)
{
    __ubuf__ uint8_t *dstPtr = (__ubuf__ uint8_t *)__cce_get_tile_ptr(dst);
    __ubuf__ ScalarT *srcPtr = (__ubuf__ ScalarT *)__cce_get_tile_ptr(src0);

    constexpr unsigned dstStride = TileDataDst::RowStride;
    constexpr unsigned srcStride = TileDataSrc0::RowStride;

    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned byteIdx = 0; byteIdx < validMaskCol; ++byteIdx) {
            uint8_t packed = 0;
            for (unsigned bit = 0; bit < 8; ++bit) {
                const unsigned c = byteIdx * 8 + bit;
                if (c >= validCol) {
                    break;
                }
                const auto a = srcPtr[r * srcStride + c];
                const bool pred = CmpScalarS(a, scalar, cmpMode);
                packed |= static_cast<uint8_t>((pred ? 1u : 0u) << bit);
            }
            dstPtr[r * dstStride + byteIdx] = packed;
        }
    }
}

	    template <typename TileDataDst, typename TileDataSrc0, typename T>
	    PTO_INTERNAL void TCMPS_IMPL(TileDataDst &dst, TileDataSrc0 &src0, T src1, CmpMode cmpMode) {
	        static_assert(std::is_same<typename TileDataSrc0::DType, int32_t>::value ||
	                std::is_same<typename TileDataSrc0::DType, float>::value ||
	                std::is_same<typename TileDataSrc0::DType, half>::value,
	                "TCMPS: Invalid data type.");
	        static_assert(std::is_same_v<typename TileDataDst::DType, uint8_t>,
	            "TCMPS: dst tile must be a packed u8 mask.");
	        static_assert(TileDataDst::isRowMajor, "TCMPS: not supported Layout type");

        static_assert(TileDataDst::Loc == TileType::Vec, "TileType of dst tile must be TileType::Vec.");
        static_assert(TileDataDst::ValidCol <= TileDataDst::Cols, "Number of valid columns for dst must not be greater than number of tile columns.");
        static_assert(TileDataDst::ValidRow <= TileDataDst::Rows, "Number of valid rows for dst must not be greater than number of tile rows.");

	        static_assert(TileDataSrc0::Loc == TileType::Vec, "TileType of src tile must be TileType::Vec.");
	        static_assert(TileDataSrc0::ValidCol <= TileDataSrc0::Cols, "Number of valid columns for scr must not be greater than number of tile columns.");
	        static_assert(TileDataSrc0::ValidRow <= TileDataSrc0::Rows, "Number of valid rows for src must not be greater than number of tile rows.");
	        PTO_ASSERT(src0.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
	        PTO_ASSERT(CeilDivision(src0.GetValidCol(), 8) == dst.GetValidCol(),
	            "Number of dst valid columns must equal ceil(src valid col / 8) for packed masks.");

	        const unsigned validRow = src0.GetValidRow();
	        const unsigned validCol = src0.GetValidCol();
	        const unsigned validMaskCol = dst.GetValidCol();

	        using ScalarT = typename TileDataSrc0::DType;
	        const ScalarT scalar = static_cast<ScalarT>(src1);

	        TCmpsScalarNpu<TileDataDst, TileDataSrc0, ScalarT>(
	            dst.data(), src0.data(), scalar, cmpMode, validRow, validCol, validMaskCol);
	    }
}
#endif
