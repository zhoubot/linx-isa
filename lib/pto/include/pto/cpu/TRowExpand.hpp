/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_CPU_TROWEXPAND_HPP
#define PTO_CPU_TROWEXPAND_HPP

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"

namespace pto {

template <typename TileDst, typename TileSrc>
PTO_INTERNAL void TROWEXPAND_IMPL(TileDst &dst, TileSrc &src)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        const auto v = src.data()[GetTileElementOffset<TileSrc>(r, 0)];
        for (std::size_t c = 0; c < cols; ++c) {
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = static_cast<typename TileDst::DType>(v);
        }
    });
}

namespace {
template <typename TileVec>
PTO_INTERNAL typename TileVec::DType load_row_scalar(TileVec &src1, std::size_t rowIndex)
{
    const std::size_t vr = static_cast<std::size_t>(src1.GetValidRow());
    const std::size_t vc = static_cast<std::size_t>(src1.GetValidCol());
    if (vr == 1 && rowIndex < vc) {
        return static_cast<typename TileVec::DType>(src1.data()[GetTileElementOffset<TileVec>(0, rowIndex)]);
    }
    if (vc == 1 && rowIndex < vr) {
        return static_cast<typename TileVec::DType>(src1.data()[GetTileElementOffset<TileVec>(rowIndex, 0)]);
    }
    return static_cast<typename TileVec::DType>(src1.data()[rowIndex % static_cast<std::size_t>(TileVec::Numel)]);
}

template <typename TileVec>
PTO_INTERNAL typename TileVec::DType load_row_broadcast(TileVec &src1, std::size_t rowIndex, std::size_t colIndex)
{
    const std::size_t vr = static_cast<std::size_t>(src1.GetValidRow());
    const std::size_t vc = static_cast<std::size_t>(src1.GetValidCol());
    if (vr == 1 && rowIndex < vc) {
        // Row vector: src1[0, rowIndex] provides the per-row scalar.
        return static_cast<typename TileVec::DType>(src1.data()[GetTileElementOffset<TileVec>(0, rowIndex)]);
    }
    if (vc == 1 && rowIndex < vr) {
        // Column vector: src1[rowIndex, 0] provides the per-row scalar.
        return static_cast<typename TileVec::DType>(src1.data()[GetTileElementOffset<TileVec>(rowIndex, 0)]);
    }
    if (rowIndex < vr && vc > 0) {
        // NPU RowExpandBinOps accept per-row vectors (e.g. f32 uses 8 elements per row / 32B block).
        // Broadcast by repeating the row's vector across destination columns.
        return static_cast<typename TileVec::DType>(
            src1.data()[GetTileElementOffset<TileVec>(rowIndex, colIndex % vc)]
        );
    }
    return static_cast<typename TileVec::DType>(
        src1.data()[(rowIndex + colIndex) % static_cast<std::size_t>(TileVec::Numel)]
    );
}
} // namespace

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDDIV_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const auto s = load_row_broadcast(src1, r, c);
            const auto v0 = static_cast<typename TileDst::DType>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = static_cast<typename TileDst::DType>(v0 / s);
        }
    });
}

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDMUL_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const auto s = load_row_broadcast(src1, r, c);
            const auto v0 = static_cast<typename TileDst::DType>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = static_cast<typename TileDst::DType>(v0 * s);
        }
    });
}

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDSUB_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const auto s = load_row_broadcast(src1, r, c);
            const auto v0 = static_cast<typename TileDst::DType>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = static_cast<typename TileDst::DType>(v0 - s);
        }
    });
}

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDADD_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const auto s = load_row_broadcast(src1, r, c);
            const auto v0 = static_cast<typename TileDst::DType>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = static_cast<typename TileDst::DType>(v0 + s);
        }
    });
}

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDMAX_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const auto s = load_row_broadcast(src1, r, c);
            const auto v0 = static_cast<typename TileDst::DType>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = std::max(v0, static_cast<typename TileDst::DType>(s));
        }
    });
}

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDMIN_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const auto s = load_row_broadcast(src1, r, c);
            const auto v0 = static_cast<typename TileDst::DType>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = std::min(v0, static_cast<typename TileDst::DType>(s));
        }
    });
}

template <typename TileDst, typename TileSrc1>
PTO_INTERNAL void TROWEXPANDEXPDIF_IMPL(TileDst &dst, TileDst &src0, TileSrc1 &src1)
{
    using T = typename TileDst::DType;
    static_assert(std::is_floating_point_v<T> || std::is_same_v<T, half>, "TROWEXPANDEXPDIF: expected floating dtype");

    const std::size_t rows = static_cast<std::size_t>(dst.GetValidRow());
    const std::size_t cols = static_cast<std::size_t>(dst.GetValidCol());
    if (rows == 0 || cols == 0) {
        return;
    }

    cpu::parallel_for_rows(rows, cols, [&](std::size_t r) {
        for (std::size_t c = 0; c < cols; ++c) {
            const float s = static_cast<float>(load_row_broadcast(src1, r, c));
            const float v0 = static_cast<float>(src0.data()[GetTileElementOffset<TileDst>(r, c)]);
            dst.data()[GetTileElementOffset<TileDst>(r, c)] = static_cast<T>(std::exp(v0 - s));
        }
    });
}

} // namespace pto

#endif
