/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLMIN_HPP
#define TCOLMIN_HPP

#include "TColReduceOps.hpp"

namespace pto {
  template <typename T>
  struct TColMinOp {
    PTO_INTERNAL static void ReduceInstr(RegTensor<T> &dst, RegTensor<T> &src0, RegTensor<T> &src1, MaskReg &pReg) {
      vmin(dst, src0, src1, pReg, MODE_ZEROING);
    }
  };

  template <typename T, typename TileDataOut, typename TileDataIn>
  __tf__ PTO_INTERNAL void TColMin(typename TileDataOut::TileDType __out__ dstData,
    typename TileDataIn::TileDType __in__ srcData, uint16_t validRow, int validCol, unsigned version) {
    __ubuf__ T *dst = (__ubuf__ T *)__cce_get_tile_ptr(dstData);
    __ubuf__ T *src = (__ubuf__ T *)__cce_get_tile_ptr(srcData);

    TColReduceInstr<TColMinOp<T>, T, TileDataIn>(dst, src, validRow, validCol, version);
  }

  template <typename TileDataOut, typename TileDataIn>
  PTO_INTERNAL void TCOLMIN_IMPL(TileDataOut &dst, TileDataIn &src) {
    int validCol = src.GetValidCol();
    int validRow = src.GetValidRow();
    TColReduceCheck<TileDataOut, TileDataIn>(validRow, validCol, dst.GetValidCol());
    if (validCol == 0 || validRow == 0) {
      return;
    }
    TColMin<typename TileDataIn::DType, TileDataOut, TileDataIn>(dst.data(), src.data(), validRow, validCol,
      VFImplKind::VFIMPL_DEFAULT);
  }
}
#endif
