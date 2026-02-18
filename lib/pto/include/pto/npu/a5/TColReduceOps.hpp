/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/
#ifndef TCOL_REDUCE_OPS_HPP
#define TCOL_REDUCE_OPS_HPP

#include <pto/common/constants.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {
  template <typename TileDataOut, typename TileDataIn>
  PTO_INTERNAL void TColReduceCheck(int srcValidRow, int srcValidCol, int dstValidCol) {
    static_assert(TileDataOut::Loc == pto::TileType::Vec && TileDataIn::Loc == pto::TileType::Vec,
      "Fix: TCOLREDUCE only support Vec Tile");
    static_assert(TileDataIn::isRowMajor && TileDataIn::SFractal == SLayout::NoneBox,
      "Fix: TCOLREDUCE input tile only support Nd fractal Tile");
    static_assert(TileDataOut::isRowMajor && TileDataOut::SFractal == SLayout::NoneBox,
      "Fix: TCOLREDUCE output tile only support Nd fractal Tile");
    using T = typename TileDataIn::DType;
    static_assert(std::is_same_v<T, half> || std::is_same_v<T, float> || std::is_same_v<T, int8_t> ||
      std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, int16_t> ||
      std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, bfloat16_t>,
      "Fix: TCOLREDUCE input data type is not supported by this instruction.");

    static_assert(std::is_same_v<typename TileDataOut::DType, T>,
      "Fix: TCOLREDUCE input data type must be consistent with the output data type.");
    PTO_ASSERT(srcValidCol == dstValidCol,
      "Fix: TCOLREDUCE input valid row must be consistent with the output valid row.");
    PTO_ASSERT(srcValidCol != 0 && srcValidRow != 0,
      "Fix: TCOLREDUCE input shape is invalid, validCol or validRow is 0.");
  }

  template <typename InstrOp, typename T, unsigned SrcStride>
  PTO_INTERNAL void TColReduceInstr_PostUpdate(__ubuf__ T *dst, __ubuf__ T *src, uint16_t validRow, int validCol) {
    constexpr unsigned elmPerRpt = CCE_VL / sizeof(T);  // 每次repeat涉及多少个元素
    constexpr auto distValue = std::integral_constant<::DistVST,
      static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    uint16_t rptTimes = CeilDivision(validCol, elmPerRpt);
    uint16_t nLoop = (validRow - 1) / 2;  // 第一行vlds到dst 故-1
    bool remain = (validRow - 1) % 2;

    __VEC_SCOPE__
    {
      __ubuf__ T *srcP0;
      __ubuf__ T *srcP1;
      RegTensor<T> src0VReg;
      RegTensor<T> src1VReg;
      RegTensor<T> tmpVReg;
      RegTensor<T> dstVReg;
      MaskReg pReg;
      uint32_t sReg = validCol;
      for (uint16_t i = 0; i < rptTimes; ++i) {
        // sReg在每次执行CreatePredicate之后会累减ElmPerRpt, 直至0
        pReg = CreatePredicate<T>(sReg);

        // 指向src的后两行
        srcP0 = src + 1 * SrcStride;
        srcP1 = src + 2 * SrcStride;

        // 将src的第一行存入dst寄存器, 随后累加ElmPerRpt
        vlds(dstVReg, src, elmPerRpt, NORM, POST_UPDATE);

        // 读取第二行及以后的每行数据存入src寄存器, 与dst寄存器相加后存入dst寄存器
        for (uint16_t j = 0; j < nLoop; ++j) {
          vlds(src0VReg, srcP0, 2 * SrcStride, NORM, POST_UPDATE);
          vlds(src1VReg, srcP1, 2 * SrcStride, NORM, POST_UPDATE);
          InstrOp::ReduceInstr(tmpVReg, src0VReg, src1VReg, pReg);
          InstrOp::ReduceInstr(dstVReg, dstVReg, tmpVReg, pReg);
        }
        if (remain) {
          vlds(src0VReg, srcP0, 0, NORM);
          InstrOp::ReduceInstr(dstVReg, dstVReg, src0VReg, pReg);
        }

        // dst每次累加ElmPerRpt
        vsts(dstVReg, dst, elmPerRpt, distValue, pReg, POST_UPDATE);
      }
    } // end VF
  }

  template <typename InstrOp, typename T, unsigned SrcStride>
  PTO_INTERNAL void TColReduceInstr_NoPostUpdate(__ubuf__ T *dst, __ubuf__ T *src, uint16_t validRow, int validCol) {
    constexpr unsigned elmPerRpt = CCE_VL / sizeof(T);  // 每次repeat涉及多少个元素
    constexpr auto distValue = std::integral_constant<::DistVST,
      static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    uint16_t rptTimes = CeilDivision(validCol, elmPerRpt);
    uint16_t nLoop = (validRow - 1) / 2;  // 第一行vlds到dst 故-1
    bool remain = (validRow - 1) % 2;

    __VEC_SCOPE__
    {
      RegTensor<T> src0VReg;
      RegTensor<T> src1VReg;
      RegTensor<T> tmpVReg;
      RegTensor<T> dstVReg;
      MaskReg preg;
      uint32_t sReg = validCol;
      for (uint16_t i = 0; i < rptTimes; ++i) {
        preg = CreatePredicate<T>(sReg);
        vlds(dstVReg, src, i * elmPerRpt, NORM);
        for (uint16_t j = 0; j < nLoop; ++j) {
          vlds(src0VReg, src, i * elmPerRpt + (2 * j + 1) * SrcStride, NORM);
          vlds(src1VReg, src, i * elmPerRpt + (2 * j + 2) * SrcStride, NORM);
          InstrOp::ReduceInstr(tmpVReg, src0VReg, src1VReg, preg);
          InstrOp::ReduceInstr(dstVReg, dstVReg, tmpVReg, preg);
        }
        if (remain) {
          vlds(src0VReg, src, i * elmPerRpt + (2 * nLoop) * SrcStride, NORM);
          InstrOp::ReduceInstr(dstVReg, dstVReg, src0VReg, preg);
        }
        vsts(dstVReg, dst, i * elmPerRpt, distValue, preg);
      }
    }
  }

  template <typename InstrOp, typename T, typename TileDataIn>
  PTO_INTERNAL void TColReduceInstr(__ubuf__ T *dst, __ubuf__ T *src, int validRow, int validCol, unsigned version) {
    switch(version) {
      case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
      case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
        TColReduceInstr_NoPostUpdate<InstrOp, T, TileDataIn::Cols>(dst, src, validRow, validCol);
        break;
      case VFImplKind::VFIMPL_1D_POST_UPDATE:
      case VFImplKind::VFIMPL_2D_POST_UPDATE:
        TColReduceInstr_PostUpdate<InstrOp, T, TileDataIn::Cols>(dst, src, validRow, validCol);
        break;
      default:
        TColReduceInstr_PostUpdate<InstrOp, T, TileDataIn::Cols>(dst, src, validRow, validCol);
        break;
    }
  }
}
#endif