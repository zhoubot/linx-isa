/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWMIN_HPP
#define TROWMIN_HPP

#include "TRowReduceOps.hpp"

namespace pto
{
  template <typename T>
  struct TRowMinOp : TRowReduceOp<T, TRowMinOp<T>> {
    PTO_INTERNAL static void
    BinInstrImpl(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1,
                 uint8_t rptTimes, uint16_t dstRptStride,
                 uint16_t src0RptStride, uint16_t src1RptStride,
                 uint8_t dstBlockStride = 1, uint8_t src0BlockStride = 1,
                 uint8_t src1BlockStride = 1) {
      vmin(dst, src0, src1, rptTimes, dstBlockStride, src0BlockStride,
           src1BlockStride, dstRptStride, src0RptStride, src1RptStride);
    }

    PTO_INTERNAL static void ReduceInstrImpl(__ubuf__ T *dst, __ubuf__ T *src, uint8_t rptTimes,
      uint16_t dstRptStride, uint16_t srcBlkStride, uint16_t srcRptStride) {
      vcmin(dst, src, rptTimes, dstRptStride, srcBlkStride, srcRptStride, ONLY_VALUE);
    }

    PTO_INTERNAL static void
    GroupReduceInstrImpl(__ubuf__ T *dst, __ubuf__ T *src, uint8_t rptTimes,
                         uint16_t dstRptStride, uint16_t src0Stride,
                         uint16_t src1Stride) {
      vcgmin(dst, src, rptTimes, dstRptStride, src0Stride, src1Stride);
    }
  };

  template <typename T, typename TileDataOut, typename TileDataIn, typename TileDataTmp>
  __tf__ PTO_INTERNAL void TRowMin(typename TileDataOut::TileDType __out__ dstData,
    typename TileDataIn::TileDType __in__ srcData, typename TileDataTmp::TileDType __in__ tmpData,
    int validCol, int validRow, unsigned version) {
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    __ubuf__ T *tmp = (__ubuf__ T *)__cce_get_tile_ptr(tmpData);

    TRowReduceInstr<TRowMinOp<T>, T, TileDataOut, TileDataIn, TileDataTmp>(dst, src, tmp, validCol, validRow);
  }

  template <typename TileDataOut, typename TileDataIn, typename TileDataTmp>
  PTO_INTERNAL void TROWMIN_IMPL(TileDataOut &dst, TileDataIn &src, TileDataTmp &tmp) {
    int validCol = src.GetValidCol();
    int validRow = src.GetValidRow();
    TRowReduceCheck<TileDataOut, TileDataIn>(validRow, validCol, dst.GetValidRow());
    if (validCol == 0 || validRow == 0) {
      return;
    }

    TRowMin<typename TileDataIn::DType, TileDataOut, TileDataIn, TileDataTmp>
      (dst.data(), src.data(), tmp.data(), validCol, validRow, VFImplKind::VFIMPL_DEFAULT);
  }
}
#endif