/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWSUM_HPP
#define TROWSUM_HPP

#include "TRowReduceOps.hpp"

namespace pto
{
  template <typename T>
  struct TRowSumOp : TRowReduceOp<T, TRowSumOp<T>> {
    using ReduceOp = TRowReduceOp<T, TRowSumOp<T>>;
    PTO_INTERNAL static void
    BinInstrImpl(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1,
                 uint8_t rptTimes, uint16_t dstRptStride,
                 uint16_t src0RptStride, uint16_t src1RptStride,
                 uint8_t dstBlockStride = 1, uint8_t src0BlockStride = 1,
                 uint8_t src1BlockStride = 1) {
      vadd(dst, src0, src1, rptTimes, dstBlockStride, src0BlockStride,
           src1BlockStride, dstRptStride, src0RptStride, src1RptStride);
    }

    PTO_INTERNAL static void ReduceInstrImpl(__ubuf__ T *dst, __ubuf__ T *src, uint8_t rptTimes,
      uint16_t dstRptStride, uint16_t srcBlkStride, uint16_t srcRptStride) {
      vcadd(dst, src, rptTimes, dstRptStride, srcBlkStride, srcRptStride, false);
    }

    PTO_INTERNAL static void
    GroupReduceInstrImpl(__ubuf__ T *dst, __ubuf__ T *src, uint8_t rptTimes,
                         uint16_t dstRptStride, uint16_t src0Stride,
                         uint16_t src1Stride) {
      vcgadd(dst, src, rptTimes, dstRptStride, src0Stride, src1Stride);
    }

    template <int TmpCols, int SrcCols, uint32_t TmpStride, uint32_t SrcStride, uint8_t ElemPerRpt>
    PTO_INTERNAL static void FillTmp(__ubuf__ T *tmp, __ubuf__ T *src, int srcRptPerRow, int validRow, int validCol) {
      // 二分Add, 将每行相邻的两个repeat相加存入tmp
      for (int i = 0; i < srcRptPerRow / 2; ++i) {
        ReduceOp::template BinInstrByMode<true, TmpCols, SrcCols, SrcCols, TmpStride, SrcStride, SrcStride, ElemPerRpt>
          (tmp + i * ElemPerRpt, src + (i * 2) * ElemPerRpt, src + (i * 2 + 1) * ElemPerRpt, validRow);
        pipe_barrier(PIPE_V);
      }
      // 若repeat为奇数, 则将最后的repeat加入tmp
      if (srcRptPerRow != 1 && srcRptPerRow % 2 == 1) {
        ReduceOp::template BinInstrByMode<true, TmpCols, TmpCols, SrcCols, TmpStride, TmpStride, SrcStride, ElemPerRpt>
          (tmp, tmp, src + (srcRptPerRow - 1) * ElemPerRpt, validRow);
        pipe_barrier(PIPE_V);
      }
    }

    template <int TmpCols, int SrcCols, uint32_t TmpStride, uint32_t SrcStride, uint8_t ElemPerRpt>
    PTO_INTERNAL static void TmpProc(__ubuf__ T *tmp, __ubuf__ T *src, int srcRptPerRow, int validRow) {
      // 二分Add后的repeat数
      unsigned curLen = srcRptPerRow / 2;
      unsigned loopRemain;
      int i;
      while (curLen > 1) {
        for (i = 0; i < curLen / 2; ++i) {
          ReduceOp::template BinInstrByMode<true, TmpCols, TmpCols, TmpCols, TmpStride, TmpStride, TmpStride,
            ElemPerRpt>(tmp + i * ElemPerRpt, tmp + i * 2 * ElemPerRpt, tmp + (i * 2 + 1) * ElemPerRpt, validRow);
          pipe_barrier(PIPE_V);
        }

        loopRemain = curLen % 2;
        curLen /= 2;
        if (loopRemain > 0) {
          ReduceOp::template BinInstrByMode<true, TmpCols, TmpCols, TmpCols, TmpStride, TmpStride, TmpStride,
            ElemPerRpt>(tmp + (curLen - 1) * ElemPerRpt, tmp + (curLen - 1) * ElemPerRpt,
            tmp + curLen * 2 * ElemPerRpt, validRow);
          pipe_barrier(PIPE_V);
        }
      }
    }
  };

  template <typename T, typename TileDataOut, typename TileDataIn, typename TileDataTmp>
  __tf__ PTO_INTERNAL void TRowSum(typename TileDataOut::TileDType __out__ dstData,
    typename TileDataIn::TileDType __in__ srcData, typename TileDataTmp::TileDType __in__ tmpData,
    int validCol, int validRow, unsigned version) {
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);
    __ubuf__ T *tmp = (__ubuf__ T *)__cce_get_tile_ptr(tmpData);

    TRowReduceInstr<TRowSumOp<T>, T, TileDataOut, TileDataIn, TileDataTmp>(dst, src, tmp, validCol, validRow);
  }

  template <typename TileDataOut, typename TileDataIn, typename TileDataTmp>
  PTO_INTERNAL void TROWSUM_IMPL(TileDataOut &dst, TileDataIn &src, TileDataTmp &tmp) {
    int validCol = src.GetValidCol();
    int validRow = src.GetValidRow();
    TRowReduceCheck<TileDataOut, TileDataIn>(validRow, validCol, dst.GetValidRow());
    if (validCol == 0 || validRow == 0) {
      return;
    }

    TRowSum<typename TileDataIn::DType, TileDataOut, TileDataIn, TileDataTmp>
      (dst.data(), src.data(), tmp.data(), validCol, validRow, VFImplKind::VFIMPL_DEFAULT);
  }
}
#endif