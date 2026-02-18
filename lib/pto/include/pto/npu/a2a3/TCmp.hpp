/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCMP_HPP
#define TCMP_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

namespace pto {

constexpr const uint64_t BITS_IN_BYTE = 8;

template <typename T>
PTO_INTERNAL bool CmpScalar(T a, T b, CmpMode cmpMode)
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

template <typename TileDataDst, typename TileDataSrc>
__tf__ PTO_INTERNAL void TCmpScalarNpu(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src0, typename TileDataSrc::TileDType __in__ src1, CmpMode cmpMode,
    unsigned validRow, unsigned validCol, unsigned validMaskCol)
{
    __ubuf__ uint8_t *dstPtr = (__ubuf__ uint8_t *)__cce_get_tile_ptr(dst);
    __ubuf__ typename TileDataSrc::DType *src0Ptr =
        (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src0);
    __ubuf__ typename TileDataSrc::DType *src1Ptr =
        (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src1);

    constexpr unsigned dstStride = TileDataDst::RowStride;
    constexpr unsigned srcStride = TileDataSrc::RowStride;

    for (unsigned r = 0; r < validRow; ++r) {
        for (unsigned byteIdx = 0; byteIdx < validMaskCol; ++byteIdx) {
            uint8_t packed = 0;
            for (unsigned bit = 0; bit < 8; ++bit) {
                const unsigned c = byteIdx * 8 + bit;
                if (c >= validCol) {
                    break;
                }
                const auto a = src0Ptr[r * srcStride + c];
                const auto b = src1Ptr[r * srcStride + c];
                const bool pred = CmpScalar(a, b, cmpMode);
                packed |= static_cast<uint8_t>((pred ? 1u : 0u) << bit);
            }
            dstPtr[r * dstStride + byteIdx] = packed;
        }
    }
}

	    template <typename TileDataDst, typename TileDataSrc>
	    PTO_INTERNAL void TCMP_IMPL(TileDataDst &dst, TileDataSrc &src0, TileDataSrc &src1, CmpMode cmpMode) {
	        static_assert(TileDataSrc::Loc == TileType::Vec, "TileType of src tiles must be TileType::Vec.");
	        static_assert(TileDataDst::Loc == TileType::Vec, "TileType of dst tiles must be TileType::Vec.");
	        static_assert(std::is_same_v<typename TileDataDst::DType, uint8_t>,
	            "TCMP: dst tile must be a packed u8 mask.");
	        static_assert(TileDataDst::isRowMajor && TileDataDst::SFractal == SLayout::NoneBox,
	            "TCMP: dst layout must be ND.");
	        static_assert(TileDataSrc::isRowMajor && TileDataSrc::SFractal == SLayout::NoneBox,
	            "TCMP: src layout must be ND.");
	        static_assert((TileDataSrc::Cols * sizeof(typename TileDataSrc::DType)) % REPEAT_BYTE == 0,
	            "TCMP: src cols * sizeof(dtype) must be a multiple of 256 bytes.");
	        static_assert(TileDataSrc::ValidCol <= TileDataSrc::Cols, "Number of valid columns must not be greater than number of tile columns.");
	        static_assert(TileDataSrc::ValidRow <= TileDataSrc::Rows, "Number of valid rows must not be greater than number of tile rows.");
	        
	        PTO_ASSERT(src0.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
	        PTO_ASSERT(CeilDivision(src0.GetValidCol(), 8) == dst.GetValidCol(),
	            "Number of dst valid columns must equal ceil(src valid col / 8) for packed masks.");

	        const unsigned validRow = src0.GetValidRow();
	        const unsigned validCol = src0.GetValidCol();
	        const unsigned validMaskCol = dst.GetValidCol();
	        TCmpScalarNpu<TileDataDst, TileDataSrc>(
	            dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol, validMaskCol);
    }
}
#endif
