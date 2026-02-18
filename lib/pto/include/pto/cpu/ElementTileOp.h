/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef ELEMENT_TILE_OP_HPP
#define ELEMENT_TILE_OP_HPP

#include "pto/cpu/ElementOp.h"
#include "pto/cpu/parallel.hpp"

namespace pto {
    template<typename tile_shape, ElementOp op>
    void BinaryElementTileOp_Impl(typename tile_shape::TileDType dst, typename tile_shape::TileDType src0,
                              typename tile_shape::TileDType src1, unsigned validRow, unsigned validCol,
                              size_t extra = 0)
    {
        using DType = typename tile_shape::DType;
        if constexpr (tile_shape::SFractal == SLayout::NoneBox) {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    const std::size_t base = r * tile_shape::Cols;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = base + c;
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], extra);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    const std::size_t base = c * tile_shape::Rows;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = base + r;
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], extra);
                    }
                });
            }
        } else {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], extra);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], extra);
                    }
                });
            }
        }
    }

    template<typename tile_shape, ElementOp op>
    void UnaryElementTileOp_Impl(typename tile_shape::TileDType dst, typename tile_shape::TileDType src,
                             unsigned validRow, unsigned validCol)
    {
        using DType = typename tile_shape::DType;
        if constexpr (tile_shape::SFractal == SLayout::NoneBox) {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    const std::size_t base = r * tile_shape::Cols;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = base + c;
                        ElementOpCal<DType, op>::apply(dst[idx], src[idx]);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    const std::size_t base = c * tile_shape::Rows;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = base + r;
                        ElementOpCal<DType, op>::apply(dst[idx], src[idx]);
                    }
                });
            }
        } else {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        ElementOpCal<DType, op>::apply(dst[idx], src[idx]);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        ElementOpCal<DType, op>::apply(dst[idx], src[idx]);
                    }
                });
            }
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TREM_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_REM>(dst.data(), src0.data(), src1.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TREM_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &tmp) {
        (void)tmp;
        TREM_IMPL(dst, src0, src1);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TSHL_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        using DType = typename tile_shape::DType;
        static_assert(std::is_integral_v<DType>, "TSHL: expected integral dtype");
        const DType scalar = src1.data()[0];
        const unsigned validRow = dst.GetValidRow();
        const unsigned validCol = dst.GetValidCol();
        if constexpr (tile_shape::SFractal == SLayout::NoneBox) {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    const std::size_t base = r * tile_shape::Cols;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = base + c;
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] << scalar);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    const std::size_t base = c * tile_shape::Rows;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = base + r;
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] << scalar);
                    }
                });
            }
        } else {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] << scalar);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] << scalar);
                    }
                });
            }
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TSHR_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        using DType = typename tile_shape::DType;
        static_assert(std::is_integral_v<DType>, "TSHR: expected integral dtype");
        const DType scalar = src1.data()[0];
        const unsigned validRow = dst.GetValidRow();
        const unsigned validCol = dst.GetValidCol();
        if constexpr (tile_shape::SFractal == SLayout::NoneBox) {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    const std::size_t base = r * tile_shape::Cols;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = base + c;
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] >> scalar);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    const std::size_t base = c * tile_shape::Rows;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = base + r;
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] >> scalar);
                    }
                });
            }
        } else {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] >> scalar);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        dst.data()[idx] = static_cast<DType>(src0.data()[idx] >> scalar);
                    }
                });
            }
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TAND_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_AND>(dst.data(), src0.data(), src1.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TOR_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_OR>(dst.data(), src0.data(), src1.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TXOR_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_XOR>(dst.data(), src0.data(), src1.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TXOR_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &tmp) {
        (void)tmp;
        TXOR_IMPL(dst, src0, src1);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TMIN_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_MIN>(dst.data(), src0.data(), src1.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TCMP_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1, CmpMode mode) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_CMP>(dst.data(), src0.data(), src1.data(), row, col,
                                                                static_cast<size_t>(mode));
    }

    template <typename tile_shape>
    PTO_INTERNAL void TLOG_IMPL(tile_shape &dst, tile_shape &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        UnaryElementTileOp_Impl<tile_shape, ElementOp::OP_LOG>(dst.data(), src.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TNEG_IMPL(tile_shape &dst, tile_shape &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        UnaryElementTileOp_Impl<tile_shape, ElementOp::OP_NEG>(dst.data(), src.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TNOT_IMPL(tile_shape &dst, tile_shape &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        UnaryElementTileOp_Impl<tile_shape, ElementOp::OP_NOT>(dst.data(), src.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TRECIP_IMPL(tile_shape &dst, tile_shape &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        UnaryElementTileOp_Impl<tile_shape, ElementOp::OP_RECIP>(dst.data(), src.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TRELU_IMPL(tile_shape &dst, tile_shape &src) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        UnaryElementTileOp_Impl<tile_shape, ElementOp::OP_RELU>(dst.data(), src.data(), row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TPRELU_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        BinaryElementTileOp_Impl<tile_shape, ElementOp::OP_PRELU>(dst.data(), src0.data(), src1.data(), row, col);
    }

    template<typename tile_shape, ElementOp op>
    void ElementTileOpWithCarry_Impl(typename tile_shape::TileDType dst, typename tile_shape::TileDType src0,
                                  typename tile_shape::TileDType src1, typename tile_shape::TileDType src2,
                                  unsigned validRow, unsigned validCol)
    {
        using DType = typename tile_shape::DType;
        if constexpr (tile_shape::SFractal == SLayout::NoneBox) {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    const std::size_t base = r * tile_shape::Cols;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = base + c;
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], src2[idx]);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    const std::size_t base = c * tile_shape::Rows;
                    PTO_CPU_VECTORIZE_LOOP
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = base + r;
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], src2[idx]);
                    }
                });
            }
        } else {
            if constexpr (tile_shape::isRowMajor) {
                cpu::parallel_for_rows(validRow, validCol, [&](std::size_t r) {
                    for (std::size_t c = 0; c < validCol; ++c) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], src2[idx]);
                    }
                });
            } else {
                cpu::parallel_for_rows(validCol, validRow, [&](std::size_t c) {
                    for (std::size_t r = 0; r < validRow; ++r) {
                        const std::size_t idx = GetTileElementOffset<tile_shape>(r, c);
                        ElementOpCal<DType, op>::apply(dst[idx], src0[idx], src1[idx], src2[idx]);
                    }
                });
            }
        }
    }

    template <typename tile_shape>
    PTO_INTERNAL void TADDC_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        ElementTileOpWithCarry_Impl<tile_shape, ElementOp::OP_ADDC>(dst.data(), src0.data(), src1.data(), src2.data(),
                                                                    row, col);
    }

    template <typename tile_shape>
    PTO_INTERNAL void TSUBC_IMPL(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2) {
        unsigned row = dst.GetValidRow();
        unsigned col = dst.GetValidCol();
        ElementTileOpWithCarry_Impl<tile_shape, ElementOp::OP_SUBC>(dst.data(), src0.data(), src1.data(), src2.data(),
                                                                    row, col);
    }
}
#endif
