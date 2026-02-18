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
#include <pto/common/type.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {
  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void TUnaryOps_1D_NoPostUpdate(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    uint16_t repeatTimes = CeilDivision(validRow * validCol, nRepeatElem);
    __VEC_SCOPE__
    {
      RegTensor<T> srcReg;
      RegTensor<T> dstReg;
      unsigned sReg = validRow * validCol;
      MaskReg pReg;
      constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
      #pragma clang loop unroll(disable)
      for (uint16_t i = 0; i < repeatTimes; ++i) {
        pReg = CreatePredicate<T>(sReg);
        vlds(srcReg, src, i * nRepeatElem, NORM);
        Op::UnaryInstr(dstReg, srcReg, pReg);
        vsts(dstReg, dst, i * nRepeatElem, distValue, pReg);
      }
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void TUnaryOps_1D_PostUpdate(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    uint16_t repeatTimes = CeilDivision(validRow * validCol, nRepeatElem);
    __VEC_SCOPE__
    {
      RegTensor<T> srcReg;
      RegTensor<T> dstReg;
      MaskReg pReg;
      unsigned sReg = validRow * validCol;
      constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
      #pragma clang loop unroll(disable)
      for (uint16_t i = 0; i < repeatTimes; ++i) {
        pReg = CreatePredicate<T>(sReg);
        vlds(srcReg, src, nRepeatElem, NORM, POST_UPDATE);
        Op::UnaryInstr(dstReg, srcReg, pReg);
        vsts(dstReg, dst, nRepeatElem, distValue, pReg, POST_UPDATE);
      }
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void TUnaryOps_2D(__ubuf__ T *dst, __ubuf__ T *src, unsigned validRow, unsigned validCol) {
    uint16_t repeatTimes = CeilDivision(validCol, nRepeatElem);
    __VEC_SCOPE__
    {
      RegTensor<T> srcReg;
      RegTensor<T> dstReg;
      MaskReg pReg;
      unsigned sReg;
      constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
      for (uint16_t i = 0; i < (uint16_t)validRow; ++i) {
        sReg = validCol;
        for (uint16_t j = 0; j < repeatTimes; ++j) {
          pReg = CreatePredicate<T>(sReg);
          vlds(srcReg, src, i * SrcTile::RowStride + j * nRepeatElem, NORM);
          Op::UnaryInstr(dstReg, srcReg, pReg);
          vsts(dstReg, dst, i * DstTile::RowStride + j * nRepeatElem, distValue, pReg);
        }
      }
    }
  }

  template <typename Op, typename T, typename DstTile, typename SrcTile, unsigned nRepeatElem>
  PTO_INTERNAL void TUnaryOps_1D_Switch(__ubuf__ T *dst, __ubuf__ T *src,
    unsigned validRow, unsigned validCol, VFImplKind version) {
    switch (version) {
      case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
      case VFImplKind::VFIMPL_2D_NO_POST_UPDATE: {
        TUnaryOps_1D_NoPostUpdate<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
        break;
      }
      default: {
        TUnaryOps_1D_PostUpdate<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
        break;
      }
    }
  }

  template <typename DstTile, typename SrcTile, typename Op>
  __tf__ PTO_INTERNAL void TUnaryOp(typename DstTile::TileDType __out__ dstData, 
    typename SrcTile::TileDType __in__ srcData, unsigned validRow, unsigned validCol,
    VFImplKind version = VFImplKind::VFIMPL_DEFAULT) {
    using T = typename DstTile::DType;
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    constexpr unsigned nRepeatElem = CCE_VL / sizeof(T);
    if constexpr (((DstTile::ValidCol == DstTile::Cols) && (SrcTile::ValidCol == SrcTile::Cols)) ||
      ((DstTile::Rows == 1) && (SrcTile::Rows == 1))) {
      TUnaryOps_1D_Switch<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol, version);
    } else {
      TUnaryOps_2D<Op, T, DstTile, SrcTile, nRepeatElem>(dst, src, validRow, validCol);
    }
  }

  template <typename DstTile, typename SrcTile, bool floatOnly = true>
  PTO_INTERNAL void TUnaryCheck() {
    static_assert(DstTile::isRowMajor && SrcTile::isRowMajor,
      "TUnaryOp: Not supported Layout type");
    static_assert(DstTile::Loc == TileType::Vec && SrcTile::Loc == TileType::Vec,
      "TUnaryOp: TileType of src and dst tiles must be TileType::Vec.");
    static_assert(DstTile::ValidCol <= DstTile::Cols,
      "TUnaryOp: Number of dst's valid columns must not be greater than number of tile columns.");
    static_assert(DstTile::ValidRow <= DstTile::Rows,
      "TUnaryOp: Number of dst's valid rows must not be greater than number of tile rows.");
    static_assert(SrcTile::ValidCol <= SrcTile::Cols,
      "TUnaryOp: Number of src's valid columns must not be greater than number of tile columns.");
    static_assert(SrcTile::ValidRow <= SrcTile::Rows,
      "TUnaryOp: Number of src's valid rows must not be greater than number of tile rows.");
    static_assert(std::is_same_v<typename DstTile::DType, typename SrcTile::DType>,
      "TUnaryOp: The data type of dst must be consistent with of src");
    static_assert(!floatOnly || std::is_same_v<typename DstTile::DType, float32_t> ||
      std::is_same_v<typename DstTile::DType, float> ||
      std::is_same_v<typename DstTile::DType, float16_t> ||
      std::is_same_v<typename DstTile::DType, half>,
      "TUnaryOp: Invalid data type.");
  }

  /* TRSQRT */
  template <typename DstTile, typename SrcTile>
  __tf__ PTO_INTERNAL void TRsqrt(typename DstTile::TileDType __out__ dstData,
    typename SrcTile::TileDType __in__ srcData, unsigned validRow, unsigned validCol) {
    using T = typename DstTile::DType;
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);

    __VEC_SCOPE__
    {
      constexpr unsigned nRepeatElem = CCE_VL / sizeof(T);
      uint16_t repeatTimes = CeilDivision(validCol, nRepeatElem);
      uint32_t sReg = (uint32_t)validCol;

      RegTensor<T> vreg0;
      RegTensor<T> vreg1;
      RegTensor<T> vreg2;
      RegTensor<T> vreg3;
      constexpr auto distValue =
        std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
      MaskReg pReg = CreatePredicate<T>(sReg);
      vdup(vreg2, (T)1.0, pReg, MODE_MERGING);
      for (uint16_t i = 0; i < (uint16_t)validRow; ++i) {
        sReg = (uint32_t)validCol;
        for(uint16_t j = 0; j < repeatTimes; ++j) {
          pReg = CreatePredicate<T>(sReg);
          vlds(vreg0, src, (i * SrcTile::RowStride + j * nRepeatElem), NORM);
          vsqrt(vreg1, vreg0, pReg, MODE_ZEROING);
          vdiv(vreg3, vreg2, vreg1, pReg);
          vsts(vreg3, dst, (i * DstTile::RowStride + j * nRepeatElem), distValue, pReg);
        }
      }
    }
  }

  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TRSQRT_IMPL(DstTile &dst, SrcTile &src) {
    TUnaryCheck<DstTile, SrcTile>();
    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();
    PTO_ASSERT(dstValidCol == src.GetValidCol(),
      "TRSQRT: Number of columns of src and dst must be the same.");
    PTO_ASSERT(dstValidRow == src.GetValidRow(),
      "TRSQRT: Number of rows of src and dst must be the same.");
    TRsqrt<DstTile, SrcTile>(dst.data(), src.data(), dstValidRow, dstValidCol);
  }

  template <typename DstTile, typename SrcTile, typename Op, bool floatOnly = true>
  PTO_INTERNAL void TUNARY_IMPL(DstTile &dst, SrcTile &src) {
    TUnaryCheck<DstTile, SrcTile, floatOnly>();
    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();
    PTO_ASSERT(dstValidCol == src.GetValidCol(),
      "TUNARY: Number of columns of src and dst must be the same.");
    PTO_ASSERT(dstValidRow == src.GetValidRow(),
      "TUNARY: Number of rows of src and dst must be the same.");
    TUnaryOp<DstTile, SrcTile, Op>(dst.data(), src.data(), dstValidRow, dstValidCol);
  }

  /* TEXP */
  template <typename T>
  struct ExpOp {
    PTO_INTERNAL static void UnaryInstr(RegTensor<T> &dstReg, RegTensor<T> &srcReg, MaskReg &pReg) {
      vexp(dstReg, srcReg, pReg, MODE_ZEROING);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TEXP_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<DstTile, SrcTile, ExpOp<typename DstTile::DType>>(dst, src);
  }

  /* TNOT */
  template <typename T>
  struct NotOp {
    PTO_INTERNAL static void UnaryInstr(RegTensor<T> &dstReg, RegTensor<T> &srcReg, MaskReg &pReg) {
      vnot(dstReg, srcReg, pReg, MODE_ZEROING);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TNOT_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<DstTile, SrcTile, NotOp<typename DstTile::DType>, false>(dst, src);
  }

  /* TRELU */
  template <typename T>
  struct ReluOp {
    PTO_INTERNAL static void UnaryInstr(RegTensor<T> &dstReg, RegTensor<T> &srcReg, MaskReg &pReg) {
      vrelu(dstReg, srcReg, pReg, MODE_ZEROING);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TRELU_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<DstTile, SrcTile, ReluOp<typename DstTile::DType>, false>(dst, src);
  }

  /* TSQRT */
  template <typename T>
  struct SqrtOp {
    PTO_INTERNAL static void UnaryInstr(RegTensor<T> &dstReg, RegTensor<T> &srcReg, MaskReg &pReg) {
      vsqrt(dstReg, srcReg, pReg, MODE_ZEROING);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TSQRT_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<DstTile, SrcTile, SqrtOp<typename DstTile::DType>>(dst, src);
  }

  /* TABS */
  template <typename T>
  struct AbsOp {
    PTO_INTERNAL static void UnaryInstr(RegTensor<T> &dstReg, RegTensor<T> &srcReg, MaskReg &pReg) {
      vabs(dstReg, srcReg, pReg, MODE_ZEROING);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TABS_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<DstTile, SrcTile, AbsOp<typename DstTile::DType>>(dst, src);
  }

  /* TLOG */
  template <typename T>
  struct LogOp {
    PTO_INTERNAL static void UnaryInstr(RegTensor<T> &dstReg, RegTensor<T> &srcReg, MaskReg &pReg) {
      vln(dstReg, srcReg, pReg, MODE_ZEROING);
    }
  };
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TLOG_IMPL(DstTile &dst, SrcTile &src) {
    TUNARY_IMPL<DstTile, SrcTile, LogOp<typename DstTile::DType>>(dst, src);
  }

  /* TNEG */
  template <typename DstTile, typename SrcTile>
  PTO_INTERNAL void TNEG_IMPL(DstTile &dst, SrcTile &src) {
    TMULS_IMPL(dst, src, -1);
  }
}
#endif
