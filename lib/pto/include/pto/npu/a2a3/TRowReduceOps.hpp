/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef T_ROW_REDUCE_OPS_HPP
#define T_ROW_REDUCE_OPS_HPP
#include <pto/common/utils.hpp>
#include <pto/common/type.hpp>

#ifndef B16_REPEAT_MAX
#define B16_REPEAT_MAX 65535
#endif

namespace pto
{
  template <typename T, typename InstrOp>
  struct TRowReduceOp {
    PTO_INTERNAL static void BinInstr(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, uint8_t rptTimes,
      uint16_t dstRptStride, uint16_t src0RptStride, uint16_t src1RptStride) {
        InstrOp::BinInstrImpl(dst, src0, src1, rptTimes, dstRptStride, src0RptStride, src1RptStride);
    }

    PTO_INTERNAL static void ReduceInstr(__ubuf__ T *dst, __ubuf__ T *src, uint8_t rptTimes,
      uint16_t dstRptStride, uint16_t srcBlkStride, uint16_t srcRptStride) {
        InstrOp::ReduceInstrImpl(dst, src, rptTimes, dstRptStride, srcBlkStride, srcRptStride);
    }

    template <int Rows, int ValidRow, int Cols, int ValidCol>
    PTO_INTERNAL static void
    ReduceOptFP32_64x128(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *tmp) {
      static_assert(std::is_same_v<T, float>,
                    "This optimization is only for float type.");
      static_assert(Rows == 64 && ValidRow == 64 && Cols == 128 &&
                        ValidCol == 128,
                    "This optimization is only for [64, 128] input.");
      // [64, 128] -> [64, 16]
      InstrOp::GroupReduceInstrImpl(tmp, src, ValidRow * 2, 1, 1, 8);
      pipe_barrier(PIPE_V);
      // [64, 16] -> [64, 8]
      InstrOp::BinInstrImpl(tmp, tmp, tmp + 8, ValidRow / 8, 8, 16, 16, 1, 2, 2);
      pipe_barrier(PIPE_V);
      // [64, 8] -> [64, 1]
      InstrOp::GroupReduceInstrImpl(dst, tmp, ValidRow / 8, 1, 1, 8);
      pipe_barrier(PIPE_V);
      return;
    }

    template <int Rows, int ValidRow, int Cols, int ValidCol>
    PTO_INTERNAL static void
    ReduceOptFP32_32x256(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *tmp) {
      static_assert(std::is_same_v<T, float>,
                    "This optimization is only for float type.");
      static_assert(Rows == 32 && ValidRow == 32 && Cols == 256 &&
                        ValidCol == 256,
                    "This optimization is only for [32, 256] input.");
      // [32, 256] -> [32, 32]
      InstrOp::GroupReduceInstrImpl(tmp, src, ValidRow * 4, 1, 1, 8);
      pipe_barrier(PIPE_V);
      // [32, 32] -> [32, 16]
      InstrOp::BinInstrImpl(tmp, tmp, tmp + 8, ValidRow / 4, 8, 16, 16, 1, 2, 2);
      pipe_barrier(PIPE_V);
      // [32, 16] -> [32, 8]
      InstrOp::BinInstrImpl(tmp, tmp, tmp + 8, ValidRow / 8, 8, 16, 16, 1, 2, 2);
      pipe_barrier(PIPE_V);
      // [32, 8] -> [32, 1]
      InstrOp::GroupReduceInstrImpl(dst, tmp, ValidRow / 8, 1, 1, 8);
      pipe_barrier(PIPE_V);
      return;
    }

    template <int Rows, int ValidRow, int Cols, int ValidCol>
    PTO_INTERNAL static void
    ReduceOptFP32_16x512(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *tmp) {
      static_assert(std::is_same_v<T, float>,
                    "This optimization is only for float type.");
      static_assert(Rows == 16 && ValidRow == 16 && Cols == 512 &&
                        ValidCol == 512,
                    "This optimization is only for [16, 512] input.");
      // [16, 512] -> [16, 64]
      InstrOp::GroupReduceInstrImpl(tmp, src, ValidRow * 8, 1, 1, 8);
      pipe_barrier(PIPE_V);
      // [16, 64] -> [16, 8]
      InstrOp::GroupReduceInstrImpl(tmp, tmp, ValidRow, 1, 1, 8);
      pipe_barrier(PIPE_V);
      // [16, 8] -> [16, 1]
      InstrOp::GroupReduceInstrImpl(dst, tmp, ValidRow / 8, 1, 1, 8);
      pipe_barrier(PIPE_V);
      return;
    }

    template <int Rows, int ValidRow, int Cols, int ValidCol>
    PTO_INTERNAL static void
    ReduceOptFP32_8x1024(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *tmp) {
      static_assert(std::is_same_v<T, float>,
                    "This optimization is only for float type.");
      static_assert(Rows == 8 && ValidRow == 8 && Cols == 1024 &&
                        ValidCol == 1024,
                    "This optimization is only for [8, 1024] input.");
      // [8, 1024] -> [8, 128]
      InstrOp::GroupReduceInstrImpl(tmp, src, ValidRow * 16, 1, 1, 8);
      pipe_barrier(PIPE_V);
      // [8, 128] -> [8, 16]
      InstrOp::GroupReduceInstrImpl(tmp, tmp, ValidRow * 2, 1, 1, 8);
      pipe_barrier(PIPE_V);
      // [8, 16] -> [8, 8]
      InstrOp::BinInstrImpl(tmp, tmp, tmp + 8, ValidRow / 8, 8, 16, 16, 1, 2, 2);
      pipe_barrier(PIPE_V);
      // [8, 8] -> [8, 1]
      InstrOp::GroupReduceInstrImpl(dst, tmp, ValidRow / 8, 1, 1, 8);
      pipe_barrier(PIPE_V);
      return;
    }

    template <bool CntModeEn, int Cols, uint32_t DstStride, uint32_t SrcStride, uint8_t ElemPerRpt>
    PTO_INTERNAL static void ReduceInstrByMode(__ubuf__ T *dst, __ubuf__ T *src, unsigned rptTimes) {
      if constexpr (DstStride > B16_REPEAT_MAX) {
        for (int i = 0; i < rptTimes; i++) {
          ReduceInstr(dst + i * DstStride, src + i * Cols, 1, 0, 1, 0);
        }
      } else if constexpr (CntModeEn) {
        set_mask_count();
        set_vector_mask(0, (uint32_t)rptTimes * ElemPerRpt);
        ReduceInstr(dst, src, 0, DstStride, 1, SrcStride);
        set_mask_norm();
        set_vector_mask(-1, -1);
      } else {
        ReduceInstr(dst, src, rptTimes, DstStride, 1, SrcStride);
      }
    }

    template <bool CntModeEn, int DstCols, int Src0Cols, int Src1Cols, uint32_t DstStride, uint32_t Src0RptStride,
      uint32_t Src1RptStride, uint8_t ElemPerRpt>
    PTO_INTERNAL static void BinInstrByMode(__ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1, unsigned rptTimes) {
      if constexpr (DstStride > REPEAT_MAX || Src0RptStride > REPEAT_MAX || Src1RptStride > REPEAT_MAX) {
        for (int i = 0; i < rptTimes; i++) {
          BinInstr(dst + i * DstCols, src0 + i * Src0Cols, src1 + i * Src1Cols, 1, 0, 0, 0);
        }
      } else if constexpr (CntModeEn) {
        set_mask_count();
        set_vector_mask(0, rptTimes * ElemPerRpt);
        BinInstr(dst, src0, src1, 0, DstStride, Src0RptStride, Src1RptStride);
        set_mask_norm();
        set_vector_mask(-1, -1);
      } else {
        BinInstr(dst, src0, src1, rptTimes, DstStride, Src0RptStride, Src1RptStride);
      }
    }

  template <int TmpCols, int SrcCols, uint32_t TmpStride, uint32_t SrcStride, uint8_t ElemPerRpt>
    PTO_INTERNAL static void FillTmp(__ubuf__ T *tmp, __ubuf__ T *src, int srcRptPerRow, int validRow, int validCol) {
      if (validCol >= 2 * ElemPerRpt) {
        // validcol大于等于2次repeat，将完整的2次repeat比较后写入tmp
        BinInstrByMode<true, TmpCols, SrcCols, SrcCols, TmpStride, SrcStride, SrcStride, ElemPerRpt>
          (tmp, src, src + ElemPerRpt, validRow);
        pipe_barrier(PIPE_V);
      }
    }

    template <int TmpCols, int SrcCols, uint32_t TmpStride, uint32_t SrcStride, uint8_t ElemPerRpt>
    PTO_INTERNAL static void TmpProc(__ubuf__ T *tmp, __ubuf__ T *src, int srcRptPerRow, int validRow) {
      for (int i = 2; i < srcRptPerRow; ++i) {
        BinInstrByMode<true, TmpCols, TmpCols, SrcCols, TmpStride, TmpStride, SrcStride, ElemPerRpt>
          (tmp, tmp, src + i * ElemPerRpt, validRow);
        pipe_barrier(PIPE_V);
      }
    }
  };

  template <typename TileDataOut, typename TileDataIn>
  PTO_INTERNAL void TRowReduceCheck(int validRow, int validCol, int dstValidRow) {
    static_assert(TileDataOut::Loc == pto::TileType::Vec && TileDataIn::Loc == pto::TileType::Vec,
      "Fix: TROWREDUCE only support Vec Tile");

    static_assert(TileDataIn::isRowMajor && TileDataIn::SFractal == SLayout::NoneBox,
      "Fix: TROWREDUCE only support Nd fractal Tile");

    static_assert((!TileDataOut::isBoxedLayout &&
      (TileDataOut::isRowMajor || (!TileDataOut::isRowMajor && TileDataOut::Cols == 1))),
      "Fix: TROWREDUCE only support Nd fractal Tile or DN Tile with Col is 1.");

    static_assert(std::is_same_v<typename TileDataIn::DType, half> ||
      std::is_same_v<typename TileDataIn::DType, float>,
      "Fix: TROWREDUCE input data type is not supported by this instruction.");

    static_assert(std::is_same_v<typename TileDataOut::DType, typename TileDataIn::DType>,
      "Fix: TROWREDUCE input data type must be consistent with the output data type.");

    PTO_ASSERT(validCol != 0 && validRow != 0, "Fix: TROWREDUCE input shape is invalid, validCol or validRow is 0.");
    PTO_ASSERT(validRow == dstValidRow, "Fix: TROWREDUCE input validRow must be consistent with the output validRow.");
  }

  template <typename InstrOp, typename T, uint32_t DstCols, uint32_t SrcCols, uint8_t elemPerRpt,
    uint32_t dstRptStride, uint32_t srcRptStride>
  PTO_INTERNAL void OneRepeatProc(__ubuf__ T *dst, __ubuf__ T *src, int validCol, int validRow, int remain,
    int rowRptTimes) {
    if (validCol == elemPerRpt) {
      InstrOp::template ReduceInstrByMode<true, SrcCols, dstRptStride, srcRptStride, elemPerRpt>
        (dst, src, validRow);
      pipe_barrier(PIPE_V);
      return;
    }

    unsigned rptTimes;
    SetContinuousMask(remain);
    do {
      rptTimes = rowRptTimes == 0 ? (validRow % REPEAT_MAX) : REPEAT_MAX;
      InstrOp::template ReduceInstrByMode<false, SrcCols, dstRptStride, srcRptStride, elemPerRpt>(dst, src, rptTimes);
      pipe_barrier(PIPE_V);
      rowRptTimes -= 1;
      dst += rptTimes * DstCols;
      src += rptTimes * SrcCols;
    } while (rowRptTimes >= 0);

    set_vector_mask(-1, -1);
  }

  template <typename InstrOp, typename T, typename TileOut, typename TileIn>
  PTO_INTERNAL bool TryOptimizeFP32Reduce(__ubuf__ T *dst, __ubuf__ T *src,
                                          __ubuf__ T *tmp) {
    if constexpr (!TileOut::isBoxedLayout && !TileOut::isRowMajor &&
                  TileOut::ValidCol == 1) {
      if constexpr (std::is_same_v<T, float>) {
        constexpr bool ShapeOf64x128 =
            TileIn::Rows == 64 && TileIn::ValidRow == 64 &&
            TileIn::Cols == 128 && TileIn::ValidCol == 128;
        constexpr bool ShapeOf32x256 =
            TileIn::Rows == 32 && TileIn::ValidRow == 32 &&
            TileIn::Cols == 256 && TileIn::ValidCol == 256;
        constexpr bool ShapeOf16x512 =
            TileIn::Rows == 16 && TileIn::ValidRow == 16 &&
            TileIn::Cols == 512 && TileIn::ValidCol == 512;
        constexpr bool ShapeOf8x1024 =
            TileIn::Rows == 8 && TileIn::ValidRow == 8 &&
            TileIn::Cols == 1024 && TileIn::ValidCol == 1024;
        if constexpr (ShapeOf64x128) {
          InstrOp::template ReduceOptFP32_64x128<
              TileIn::Rows, TileIn::ValidRow, TileIn::Cols, TileIn::ValidCol>(
              dst, src, tmp);
          return true;
        } else if constexpr (ShapeOf32x256) {
          InstrOp::template ReduceOptFP32_32x256<
              TileIn::Rows, TileIn::ValidRow, TileIn::Cols, TileIn::ValidCol>(
              dst, src, tmp);
          return true;
        } else if constexpr (ShapeOf16x512) {
          InstrOp::template ReduceOptFP32_16x512<
              TileIn::Rows, TileIn::ValidRow, TileIn::Cols, TileIn::ValidCol>(
              dst, src, tmp);
          return true;
        } else if constexpr (ShapeOf8x1024) {
          InstrOp::template ReduceOptFP32_8x1024<
              TileIn::Rows, TileIn::ValidRow, TileIn::Cols, TileIn::ValidCol>(
              dst, src, tmp);
          return true;
        }
      }
    }
    return false;
  }

  template <typename InstrOp, typename T, typename TileDataOut, typename TileDataIn, typename TileDataTmp>
  PTO_INTERNAL void TRowReduceInstr(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *tmp, int validCol, int validRow) {
    if (TryOptimizeFP32Reduce<InstrOp, T, TileDataOut, TileDataIn>(dst, src, tmp)) {
      return;
    }
    constexpr uint8_t elemPerBlock = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr uint8_t elemPerRpt = REPEAT_BYTE / sizeof(T);
    constexpr uint32_t dstRptStride = TileDataOut::Cols;
    constexpr uint32_t srcRptStride = TileDataIn::Cols / elemPerBlock;
    constexpr uint32_t tmpRptStride = TileDataTmp::Cols / elemPerBlock;
    int srcRptPerRow = validCol / elemPerRpt;
    int remain = validCol % elemPerRpt;
    int rowRptTimes = validRow / REPEAT_MAX;   // 需要处理的行若超过uint8_max, 则拆分为多次进行循环
    unsigned rptTimes;

    if (validCol <= elemPerRpt) {
      OneRepeatProc<InstrOp, T, TileDataOut::Cols, TileDataIn::Cols, elemPerRpt, dstRptStride, srcRptStride>
        (dst, src, validCol, validRow, remain, rowRptTimes);
      return;
    }

    if (validCol < 2 * elemPerRpt) {
      // 解决 ccec 编译检查问题； 如果删除会导致copy_ubuf_to_ubuf编译错误，提醒第六、七个参数的范围必须是[0, 65535]
      if constexpr ((srcRptStride < BLOCK_MAX_PER_REPEAT) || (tmpRptStride < BLOCK_MAX_PER_REPEAT)) {
        return;
      }
      // 将满足一次repeat部分copy到dst
      copy_ubuf_to_ubuf(tmp, src, 0, validRow, BLOCK_MAX_PER_REPEAT, srcRptStride - BLOCK_MAX_PER_REPEAT,
        tmpRptStride - BLOCK_MAX_PER_REPEAT);
      pipe_barrier(PIPE_V);
    }

    InstrOp::template FillTmp<TileDataTmp::Cols, TileDataIn::Cols, tmpRptStride, srcRptStride,
      elemPerRpt>(tmp, src, srcRptPerRow, validRow, validCol);

    // 不足一次repeat的部分设置mask与tmp计算, 此时tmp必定存在有效数据
    if (remain > 0) {
      __ubuf__ T *srcP = src;
      __ubuf__ T *tmpP = tmp;
      SetContinuousMask(remain);
      do {
        rptTimes = rowRptTimes == 0 ? (validRow % REPEAT_MAX) : REPEAT_MAX;
        InstrOp::template BinInstrByMode<false, TileDataTmp::Cols, TileDataTmp::Cols, TileDataIn::Cols,
          tmpRptStride, tmpRptStride, srcRptStride, elemPerRpt>
          (tmpP, tmpP, srcP + srcRptPerRow * elemPerRpt, rptTimes);
        rowRptTimes -= 1;
        srcP += rptTimes * TileDataIn::Cols;
        tmpP += rptTimes * TileDataTmp::Cols;
      } while (rowRptTimes >= 0);
      set_vector_mask(-1, -1);
      pipe_barrier(PIPE_V);
    }

    InstrOp::template TmpProc<TileDataTmp::Cols, TileDataIn::Cols, tmpRptStride, srcRptStride, elemPerRpt>
      (tmp, src, srcRptPerRow, validRow);

    InstrOp::template ReduceInstrByMode<true, TileDataTmp::Cols, dstRptStride, tmpRptStride, elemPerRpt>
      (dst, tmp, validRow);
    pipe_barrier(PIPE_V);
  }

}

#endif