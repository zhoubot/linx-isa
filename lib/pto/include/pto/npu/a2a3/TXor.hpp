/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TXOR_HPP
#define TXOR_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "pto/npu/a2a3/TBinOp.hpp"

namespace pto {
    template <typename T, typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
    __tf__ PTO_INTERNAL void TXorScalar(typename TileDataDst::TileDType __out__ dst, typename TileDataSrc0::TileDType __in__ src0,
        typename TileDataSrc1::TileDType __in__ src1, unsigned validRows, unsigned validCols) {
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
        __ubuf__ T *src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
        for (unsigned r = 0; r < validRows; ++r) {
            for (unsigned c = 0; c < validCols; ++c) {
                dstPtr[r * TileDataDst::RowStride + c] =
                    src0Ptr[r * TileDataSrc0::RowStride + c] ^ src1Ptr[r * TileDataSrc1::RowStride + c];
            }
        }
    }

    template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1, typename TileDataTmp>
    PTO_INTERNAL void TXorCheck(const TileDataDst &dst, const TileDataSrc0 &src0, const TileDataSrc1 &src1, const TileDataTmp &tmp) {
        using T = typename TileDataDst::DType;
        static_assert(std::is_same<T, typename TileDataSrc0::DType>::value &&
            std::is_same<T, typename TileDataSrc1::DType>::value &&
            std::is_same<T, typename TileDataTmp::DType>::value,
            "Fix: TXOR the data type of dst must be consistent with of src0 and src1.");
        static_assert(std::is_same<T, uint16_t>::value || std::is_same<T, int16_t>::value ||
                          std::is_same<T, uint32_t>::value || std::is_same<T, int32_t>::value ||
                          std::is_same<T, unsigned int>::value || std::is_same<T, int>::value,
            "Fix: TXOR has invalid data type.");
        static_assert(TileDataDst::isRowMajor && TileDataSrc0::isRowMajor && TileDataSrc1::isRowMajor && TileDataTmp::isRowMajor,
            "Fix: TXOR only support row major layout.");
        unsigned validRows = dst.GetValidRow();
        unsigned validCols = dst.GetValidCol();
        PTO_ASSERT(src0.GetValidRow() == validRows && src0.GetValidCol() == validCols,
            "Fix: TXOR input tile src0 valid shape mismatch with output tile dst shape.");
        PTO_ASSERT(src1.GetValidRow() == validRows && src1.GetValidCol() == validCols,
            "Fix: TXOR input tile src1 valid shape mismatch with output tile dst shape.");
        PTO_ASSERT(tmp.GetValidRow() == validRows && tmp.GetValidCol() == validCols,
            "Fix: TXOR input tile tmp valid shape mismatch with output tile dst shape.");
    }

    template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1, typename TileDataTmp>
    PTO_INTERNAL void TXOR_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1, TileDataTmp &tmp) {
        TXorCheck(dst, src0, src1, tmp);
        using T = typename TileDataDst::DType;
        if constexpr (sizeof(T) == 4) {
            static_assert(TileDataDst::SFractal == SLayout::NoneBox && TileDataSrc0::SFractal == SLayout::NoneBox &&
                              TileDataSrc1::SFractal == SLayout::NoneBox,
                "Fix: TXOR b32 fallback only supports non-boxed layouts.");
            TXorScalar<T, TileDataDst, TileDataSrc0, TileDataSrc1>(
                dst.data(), src0.data(), src1.data(), dst.GetValidRow(), dst.GetValidCol());
            return;
        }
        TOR_IMPL(dst, src0, src1);
        pipe_barrier(PIPE_V);
        TAND_IMPL(tmp, src0, src1);
        pipe_barrier(PIPE_V);
        TNOT_IMPL(tmp, tmp);
        pipe_barrier(PIPE_V);
        TAND_IMPL(dst, dst, tmp);
    }
}

#endif
