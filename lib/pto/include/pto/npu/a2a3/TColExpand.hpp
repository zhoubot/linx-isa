/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLEXPAND_HPP
#define TCOLEXPAND_HPP

#include <pto/common/utils.hpp>
#include <pto/common/type.hpp>

namespace pto {
    template <typename T, typename TileDataDst, typename TileDataSrc, unsigned dstStride>
    __tf__ PTO_INTERNAL void TColExpand(typename TileDataDst::TileDType __out__ dst,
                                     typename TileDataSrc::TileDType __in__ src,
                                     int validRow, int validCol)
    {
        __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
        __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

        int lenBurst = (validCol * sizeof(T) + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;

        for (int i = 0; i < validRow; i++) {
            copy_ubuf_to_ubuf(dstPtr + i * dstStride, srcPtr, 0, 1, lenBurst, 0, 0);
        }
    }

    template <typename T, typename TileDataOut, typename TileDataIn>
    PTO_INTERNAL void TColExpandCheck(int SrcValidRow, int SrcValidCol, int DstValidCol) {
        static_assert(TileDataOut::Loc == pto::TileType::Vec && TileDataIn::Loc == pto::TileType::Vec,
                      "Fix: TCOLEXPAND only support Vec Tile");
        static_assert(TileDataIn::isRowMajor && TileDataIn::SFractal == SLayout::NoneBox,
                      "Fix: TCOLEXPAND input tile only support Nd fractal Tile");
        static_assert(TileDataOut::isRowMajor && TileDataOut::SFractal == SLayout::NoneBox,
                      "Fix: TCOLEXPAND output tile only support Nd fractal Tile");
        static_assert(std::is_same_v<T, half> || std::is_same_v<T, float> ||
                      std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t> ||
                      std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>,
                      "Fix: TCOLEXPAND input data type is not supported by this instruction.");
        static_assert(std::is_same_v<typename TileDataOut::DType, T>,
                      "Fix: TCOLEXPAND input data type must be consistent with the output data type.");
        PTO_ASSERT(SrcValidCol == DstValidCol,
                   "Fix: TCOLEXPAND input valid col must be consistent with the output valid row.");
    }

    template <typename TileDataOut, typename TileDataIn>
    PTO_INTERNAL void TCOLEXPAND_IMPL(TileDataOut &dst, TileDataIn &src) {
        using T = typename TileDataIn::DType;
        int validRow = dst.GetValidRow();
        int validCol = dst.GetValidCol();
        TColExpandCheck<T, TileDataOut, TileDataIn>(validRow, validCol, dst.GetValidCol());
        if (validRow == 0 || validCol == 0 || src.GetValidRow() == 0 || src.GetValidCol() == 0) {
            return;
        }
        constexpr int dstStride = TileDataOut::RowStride;
        TColExpand<T, TileDataOut, TileDataIn, dstStride>(dst.data(), src.data(), validRow, validCol);
    }
}
#endif