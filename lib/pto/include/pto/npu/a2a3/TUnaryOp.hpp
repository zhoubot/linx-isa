/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TUNARYOP_HPP
#define TUNARYOP_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <type_traits>

namespace pto {
  #define SMALL_RPT (4)
  template <typename Op, typename T>
  PTO_INTERNAL void Unary1LCountMode(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    set_mask_count();
    SetVectorCount(validRow * validCol);
    Op::UnaryInstr(dst, src, 0);
    set_mask_norm();
    SetFullVecMaskByDType<T>();
  }

  template <typename Op, typename T>
  PTO_INTERNAL void Unary1LNormMode(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    constexpr unsigned nRepeatElem = REPEAT_BYTE / sizeof(T);
    unsigned nElem = validRow * validCol;
    unsigned headRepeats = nElem / nRepeatElem;
    unsigned tailElements = nElem % nRepeatElem;

    Op::UnaryInstr(dst, src, headRepeats);
    if (tailElements) {
      unsigned offset = headRepeats * nRepeatElem;
      SetContMaskByDType<T>(tailElements);
      Op::UnaryInstr(dst + offset, src + offset, 1);
      SetFullVecMaskByDType<T>();
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile>
  PTO_INTERNAL void Unary2LCountMode(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    set_mask_count();
    SetVectorCount(validCol);
    for (uint32_t i = 0; i < validRow; i++) {
      Op::UnaryInstr(dst + i * DstTile::RowStride, src + i * SrcTile::RowStride, 0);
    }
    set_mask_norm();
    SetFullVecMaskByDType<T>();
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void Unary2LNormModeColVLAlign(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    unsigned headRepeats = validCol / nRepeatElem;
    for (uint32_t i = 0; i < validRow; i++) {
      Op::UnaryInstr(dst + i * DstTile::RowStride, src + i * SrcTile::RowStride, headRepeats);
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void Unary2LNormModeHead(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned nRepeatPerLine) {
    if (nRepeatPerLine) {
      unsigned loop = nRepeatPerLine / REPEAT_MAX;
      unsigned remain = nRepeatPerLine % REPEAT_MAX;
      for (unsigned i = 0; i < validRow; i++) {
        if (loop) {
          for (unsigned j = 0; j < loop; j++) {
            Op::UnaryInstr(dst + i * DstTile::RowStride + j * nRepeatElem * REPEAT_MAX,
              src + i * SrcTile::RowStride + j * nRepeatElem * REPEAT_MAX, REPEAT_MAX);
          }
        }
        if (remain) {
          Op::UnaryInstr(dst + i * DstTile::RowStride + loop * nRepeatElem * REPEAT_MAX,
            src + i * SrcTile::RowStride + loop * nRepeatElem * REPEAT_MAX, remain);
        }
      }
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem, unsigned blockSizeElem>
  PTO_INTERNAL void Unary2LNormModeTail(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned nRemainPerLine) {
    constexpr unsigned dstStride = DstTile::RowStride / blockSizeElem;
    constexpr unsigned srcStride = SrcTile::RowStride / blockSizeElem;
    unsigned loop = 0;
    unsigned remain = validRow;
    constexpr bool strideOverFlag = (dstStride > REPEAT_STRIDE_MAX || srcStride > REPEAT_STRIDE_MAX);
    SetContMaskByDType<T>(nRemainPerLine);
    if constexpr (DstTile::Rows > pto::REPEAT_MAX || SrcTile::Rows > pto::REPEAT_MAX) {
      loop = validRow / REPEAT_MAX;
      for (uint32_t i = 0; i < loop; i++) {
        if constexpr (strideOverFlag) {
          for (uint64_t j = 0; j < REPEAT_MAX; j++) {
            Op::UnaryInstr(dst + (i * REPEAT_MAX + j) * DstTile::RowStride,
                           src + (i * REPEAT_MAX + j) * SrcTile::RowStride, 1, 1, 1);
          }
        } else {
          Op::UnaryInstr(dst + i * REPEAT_MAX * DstTile::RowStride,
                         src + i * REPEAT_MAX * SrcTile::RowStride, REPEAT_MAX, dstStride, srcStride);
        }
      }
      remain = validRow % REPEAT_MAX;
    }
    if (remain) {
      if constexpr (strideOverFlag) {
        for (uint32_t j = 0; j < remain; j++) {
          Op::UnaryInstr(dst + (loop * REPEAT_MAX + j) * DstTile::RowStride,
                         src + (loop * REPEAT_MAX + j) * SrcTile::RowStride, 1, 1, 1);
        }
      } else {
        Op::UnaryInstr(dst + loop * REPEAT_MAX * DstTile::RowStride,
                       src + loop * REPEAT_MAX * SrcTile::RowStride, remain, dstStride, srcStride);
      }
    }
    SetFullVecMaskByDType<T>();
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void Unary2LNormModeRowRpt(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned dstStride = DstTile::RowStride / blockSizeElem;
    constexpr unsigned srcStride = SrcTile::RowStride / blockSizeElem;
    constexpr bool condRowRpt = ((DstTile::Rows <= pto::REPEAT_MAX) && (dstStride <= REPEAT_STRIDE_MAX) &&
                                (SrcTile::Rows <= pto::REPEAT_MAX) && (srcStride <= REPEAT_STRIDE_MAX));
    if constexpr (condRowRpt) {
      unsigned loop = validCol / nRepeatElem;
      unsigned tailElements = validCol % nRepeatElem;
      for (uint32_t i = 0; i < loop; i++) {
        Op::UnaryInstr(dst + i * nRepeatElem, src + i * nRepeatElem, validRow, dstStride, srcStride);
      }

      if (tailElements) {
        SetContMaskByDType<T>(tailElements);
        Op::UnaryInstr(dst + loop * nRepeatElem, src + loop * nRepeatElem, validRow, dstStride, srcStride);
        SetFullVecMaskByDType<T>();
      }
    } else {
      unsigned nRepeatPerLine = validCol / nRepeatElem;
      unsigned remain = validCol % nRepeatElem;
      if constexpr (DstTile::Rows > nRepeatElem) {
        Unary2LNormModeHead<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, nRepeatPerLine);
        dst += nRepeatPerLine * nRepeatElem;
        src += nRepeatPerLine * nRepeatElem;
      }
      if (remain) {
        Unary2LNormModeTail<Op, T, DstTile, SrcTile, nRepeatElem, blockSizeElem>(dst, src, validRow, remain);
      }
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void Unary2LProcess(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    constexpr unsigned normColRepeat = DstTile::Cols / nRepeatElem;
    if constexpr ((normColRepeat > 1) && ((DstTile::Rows * normColRepeat) < SMALL_RPT)) {
      Unary2LCountMode<Op, T, DstTile, SrcTile>(dst, src, validRow, validCol);
    } else if constexpr (DstTile::Rows < (normColRepeat + 1)) {
      unsigned tailElements = validCol % nRepeatElem;
      if (tailElements) {
        Unary2LCountMode<Op, T, DstTile, SrcTile>(dst, src, validRow, validCol);
      } else {
        Unary2LNormModeColVLAlign<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
      }
    } else {
      Unary2LNormModeRowRpt<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
    }
  }

  template <typename Op, typename DstTile, typename SrcTile>
  __tf__ PTO_INTERNAL void TUnaryOp(typename DstTile::TileDType __out__ dstData,
    typename SrcTile::TileDType __in__ srcData, unsigned validRow,unsigned validCol) {
    using T = typename DstTile::DType;
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    constexpr unsigned nRepeatElem = REPEAT_BYTE / sizeof(T);
    constexpr bool isCombined = ((DstTile::ValidCol == DstTile::Cols) && (SrcTile::ValidCol == SrcTile::Cols)) ||
                                ((DstTile::Rows == 1) && (SrcTile::Rows == 1));

    if constexpr (isCombined) {
      constexpr unsigned totalRepeats = (DstTile::Rows * DstTile::Cols + nRepeatElem - 1) / nRepeatElem;
      if constexpr (totalRepeats > pto::REPEAT_MAX) {
        Unary1LCountMode<Op, T>(dst, src, validRow, validCol);
      } else {
        Unary1LNormMode<Op, T>(dst, src, validRow, DstTile::Cols);
      }
    } else {
      constexpr bool isSameShape = (DstTile::Cols == SrcTile::Cols) && (DstTile::Rows == SrcTile::Rows);
      if constexpr (isSameShape) {
        if ((validCol == DstTile::Cols) || (validRow == 1)) {
          unsigned totalRepeats = (validRow * validCol + nRepeatElem - 1) / nRepeatElem;
          if (totalRepeats > pto::REPEAT_MAX) {
            Unary1LCountMode<Op, T>(dst, src, validRow, validCol);
          } else {
            Unary1LNormMode<Op, T>(dst, src, validRow, validCol);
          }
        } else {
          Unary2LProcess<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
        }
      } else {
        Unary2LProcess<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
      }
    }
  }

  template <typename Op, typename DstTile, typename SrcTile, bool floatOnly = true>
  PTO_INTERNAL void TUNARY_IMPL(DstTile &dst, SrcTile &src) {
    static_assert(DstTile::isRowMajor && SrcTile::isRowMajor,
      "TUnaryOp: Not supported Layout type");
    static_assert(DstTile::Loc == TileType::Vec && SrcTile::Loc == TileType::Vec,
      "TUnaryOp: TileType of src and dst tiles must be TileType::Vec.");
    static_assert(SrcTile::ValidCol <= SrcTile::Cols,
      "TUnaryOp: Number of src's valid columns must not be greater than number of tile columns.");
    static_assert(DstTile::ValidCol <= DstTile::Cols,
      "TUnaryOp: Number of dst's valid columns must not be greater than number of tile columns.");
    static_assert(SrcTile::ValidRow <= SrcTile::Rows,
      "TUnaryOp: Number of src's valid rows must not be greater than number of tile rows.");
    static_assert(DstTile::ValidRow <= DstTile::Rows,
      "TUnaryOp: Number of dst's valid rows must not be greater than number of tile rows.");
    static_assert(std::is_same<typename DstTile::DType, typename SrcTile::DType>::value,
      "TUnaryOp: The data type of dst must be consistent with of src");
    static_assert(!floatOnly || std::is_same<typename DstTile::DType, float32_t>::value ||
      std::is_same<typename DstTile::DType, float>::value ||
      std::is_same<typename DstTile::DType, half>::value ||
      std::is_same<typename DstTile::DType, float16_t>::value,
      "TUNARY: Invalid data type");

    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();
    PTO_ASSERT(dstValidRow == src.GetValidRow(),
      "TUNARY: Number of rows of src and dst must be the same.");
    PTO_ASSERT(dstValidCol == src.GetValidCol(),
      "TUNARY: Number of columns of src and dst must be the same.");
    TUnaryOp<Op, DstTile, SrcTile>(dst.data(), src.data(), dstValidRow, dstValidCol);
  }

  /* RSQRT */
  template <typename T>
  struct RsqrtOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vrsqrt(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TRSQRT_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<RsqrtOp<typename DstTile::DType>>(dst, src);
  }

  /* SQRT */
  template <typename T>
  struct SqrtOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vsqrt(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TSQRT_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<SqrtOp<typename DstTile::DType>>(dst, src);
  }

  /* EXP */
  template <typename T>
  struct ExpOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vexp(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TEXP_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<ExpOp<typename DstTile::DType>>(dst, src);
  }

  /* NOT */
  template <typename T, typename DstTile, typename SrcTile>
  __tf__ PTO_INTERNAL void TNotScalar(typename DstTile::TileDType __out__ dstData, typename SrcTile::TileDType __in__ srcData,
                                      unsigned validRow, unsigned validCol) {
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    for (unsigned r = 0; r < validRow; ++r) {
      for (unsigned c = 0; c < validCol; ++c) {
        dst[r * DstTile::RowStride + c] = ~src[r * SrcTile::RowStride + c];
      }
    }
  }

  template <typename T>
  struct NotOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vnot(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TNOT_IMPL(DstTile &dst, SrcTile &src) {
    using T = typename DstTile::DType;
    if constexpr (std::is_integral_v<T> && sizeof(T) == 4) {
      static_assert(DstTile::SFractal == SLayout::NoneBox && SrcTile::SFractal == SLayout::NoneBox,
                    "Fix: TNOT b32 fallback only supports non-boxed layouts.");
      unsigned validRow = dst.GetValidRow();
      unsigned validCol = dst.GetValidCol();
      TNotScalar<T, DstTile, SrcTile>(dst.data(), src.data(), validRow, validCol);
      return;
    }
    TUNARY_IMPL<NotOp<typename DstTile::DType>, DstTile, SrcTile, false>(dst, src);
  }

  /* RELU */
  template <typename T>
  struct ReluOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vrelu(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TRELU_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<ReluOp<typename DstTile::DType>, DstTile, SrcTile, false>(dst, src);
  }

  /* ABS */
  template <typename T>
  struct AbsOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vabs(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TABS_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<AbsOp<typename DstTile::DType>>(dst, src);
  }

  /* LOG */
  template <typename T>
  struct LogOp {
    PTO_INTERNAL static void UnaryInstr(__ubuf__ T* dst, __ubuf__ T* src, uint8_t repeat,
      uint8_t dstStride = BLOCK_MAX_PER_REPEAT, uint8_t srcStride = BLOCK_MAX_PER_REPEAT) {
      vln(dst, src, repeat, 1, 1, dstStride, srcStride);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TLOG_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<LogOp<typename DstTile::DType>>(dst, src);
  }

  /* TNEG */
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TNEG_IMPL(DstTile &dst, SrcTile &src) {
    TMULS_IMPL(dst, src, -1);
  }
}

#endif
