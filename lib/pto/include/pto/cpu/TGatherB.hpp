/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TGATHERB_HPP
#define TGATHERB_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <pto/common/pto_tile.hpp>

namespace pto {

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset>
PTO_INTERNAL void TGatherB(typename TileDataDst::TileDType dst, typename TileDataSrc::TileDType src,
    typename TileDataOffset::TileDType offset, unsigned validRow, unsigned validCol)
{
    const auto *srcBytes = reinterpret_cast<const std::uint8_t *>(src);
    auto *dstBytes = reinterpret_cast<std::uint8_t *>(dst);
    const std::size_t srcBytesN = static_cast<std::size_t>(TileDataSrc::Rows) *
                                  static_cast<std::size_t>(TileDataSrc::Cols) * sizeof(typename TileDataSrc::DType);
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataDst::DType);
    for (unsigned r = 0; r < validRow; r++) {
        for (unsigned c = 0; c < validCol / blockSizeElem; c++) {
            const std::size_t idx = static_cast<std::size_t>(r) * static_cast<std::size_t>(TileDataOffset::Cols) + c;
            const std::uint32_t off = static_cast<std::uint32_t>(offset[idx]);
            const std::size_t dstOff =
                (static_cast<std::size_t>(r) * static_cast<std::size_t>(TileDataDst::Cols) + c * blockSizeElem) *
                sizeof(typename TileDataDst::DType);

            typename TileDataDst::DType v[blockSizeElem];
            if (off + BLOCK_BYTE_SIZE <= srcBytesN) {
                for (unsigned i = 0; i < BLOCK_BYTE_SIZE; i++) {
                    dstBytes[dstOff + i] = srcBytes[off + i];
                }
            }
        }
    }
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataOffset>
PTO_INTERNAL void TGATHERB_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataOffset &offset)
{
    static_assert(TileDataDst::isRowMajor, "TGATHERB: not supported Layout type.");
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();
    assert(validCol * sizeof(typename TileDataDst::TileDType) % 32 == 0);
    TGatherB<TileDataDst, TileDataSrc, TileDataOffset>(dst.data(), src.data(), offset.data(), validRow, validCol);
}
} // namespace pto

#endif // TGATHERB_HPP
