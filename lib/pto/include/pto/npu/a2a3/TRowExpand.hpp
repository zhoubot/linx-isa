/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TROWEXPAND_HPP
#define TROWEXPAND_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>

namespace pto {
constexpr const int vbrcbElem = 8;
constexpr const int B8_DATA_TYPE_OFFSET = 8;
  template<typename T>
  struct VdupTrait {
    static constexpr bool isB8 = (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>);
    using DupType = std::conditional_t<isB8, int16_t, T>;

    PTO_INTERNAL DupType DupValue(T value) {
      if constexpr (isB8) {
        // splice two b8 into one b16
        DupType u16 = static_cast<DupType>(value);
        return u16 + (u16 << B8_DATA_TYPE_OFFSET);
      } else {
        return value;
      }
    }

    PTO_INTERNAL uint64_t DupSize(uint64_t size) {
      if constexpr (isB8) {
        // UB是32B对齐，这是安全的
        return (size + sizeof(DupType) - 1) / sizeof(DupType);
      } else {
        return size;
      }
    }

    PTO_INTERNAL constexpr uint64_t DupDstStride(uint64_t stride) {
      if constexpr (isB8) {
        return stride / sizeof(DupType);
      } else {
        return stride;
      }
    }
  };
  
  template <typename T, typename TileDataDst, typename TileDataSrc>
  __tf__ PTO_INTERNAL void TRowExpand(typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src, int validRow, int validCol) {
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    using DupType = typename VdupTrait<T>::DupType;
    __ubuf__ DupType *dupDst = (__ubuf__ DupType *)dstPtr;
    VdupTrait<T> trait;

    constexpr int dstStride = TileDataDst::RowStride;
    constexpr int srcStride = TileDataSrc::RowStride;
    constexpr int dupStride = trait.DupDstStride(dstStride);
    int dupValidCol = trait.DupSize(validCol);

    set_mask_count();
    set_vector_mask(0, dupValidCol);
    for (int i = 0; i < validRow; i++) {
      PtoSetWaitFlag<PIPE_V, PIPE_S>();
      T tempValue = (T)(*(srcPtr + i * srcStride));
      DupType dupValue = trait.DupValue(tempValue);
      PtoSetWaitFlag<PIPE_S, PIPE_V>();
      vector_dup(dupDst + i * dupStride, dupValue, 0, 1, 1, BLOCK_MAX_PER_REPEAT, 0);
    }
    set_mask_norm();
    set_vector_mask(-1, -1);
  }

  template <typename T, typename TileDataDst, typename TileDataSrc>
  __tf__ PTO_INTERNAL void TRowExpandBrcb(typename TileDataDst::TileDType __out__ dstData,
    typename TileDataSrc::TileDType __in__ srcData) {
    using BrcbType = std::conditional_t<sizeof(T) == sizeof(uint16_t), uint16_t,
                                       std::conditional_t<sizeof(T) == sizeof(uint32_t), uint32_t, T>>;
    __ubuf__ BrcbType *dst = (__ubuf__ BrcbType *)__cce_get_tile_ptr(dstData);
    __ubuf__ BrcbType *src = (__ubuf__ BrcbType *)__cce_get_tile_ptr(srcData);
    constexpr int repeat = TileDataSrc::Numel / vbrcbElem;
    constexpr int elemPerRepeat = REPEAT_BYTE / sizeof(T);

    // vbrcb requires src to be 32B aligned, and offset REPEAT_MAX * vbrcbElem is non-32B aligned
    constexpr int loop = repeat / (REPEAT_MAX - 1);
    constexpr int remain = repeat % (REPEAT_MAX - 1);
    if constexpr (loop > 0) {
      for (int i = 0; i < loop; ++i) {
        vbrcb(dst + i * (REPEAT_MAX - 1) * elemPerRepeat, src + i * (REPEAT_MAX - 1) * vbrcbElem,
          1, BLOCK_MAX_PER_REPEAT, (REPEAT_MAX - 1));
      }
    }
    if constexpr (remain > 0) {
      vbrcb(dst + loop * (REPEAT_MAX - 1) * elemPerRepeat, src + loop * (REPEAT_MAX - 1) * vbrcbElem,
        1, BLOCK_MAX_PER_REPEAT, remain);
    }
  }

  template <typename TileDataDst, typename TileDataSrc>
  PTO_INTERNAL void TROWEXPAND_IMPL(TileDataDst &dst, TileDataSrc &src) {
    using T = typename TileDataSrc::DType;
    static_assert((sizeof(typename TileDataSrc::DType) == 1) ||
      (sizeof(typename TileDataSrc::DType) == 2) ||
      (sizeof(typename TileDataSrc::DType) == 4),
      "Fix: TROWEXPAND Data type must be b8/b16/b32");
    static_assert(std::is_same_v<typename TileDataDst::DType, T>,
      "Fix: TROWEXPAND input data type must be consistent with the output data type");
    static_assert(TileDataSrc::Loc == pto::TileType::Vec, "Fix: TROWEXPAND Src TileType must be Vec!");
    static_assert(TileDataDst::Loc == pto::TileType::Vec, "Fix: TROWEXPAND Dst TileType must be Vec!");
    static_assert(TileDataSrc::SFractal == SLayout::NoneBox, "Fix: TROWEXPAND Src layout must be ND or DN!");
    static_assert((TileDataDst::isRowMajor && (TileDataDst::SFractal == SLayout::NoneBox)),
      "Fix: TROWEXPAND Src and dst layout must be ND!");
    int srcValidRow = src.GetValidRow();
    int srcValidCol = src.GetValidCol();
    int dstValidRow = dst.GetValidRow();
    int dstValidCol = dst.GetValidCol();
    PTO_ASSERT((TileDataSrc::isRowMajor && (srcValidRow == dstValidRow)) ||
               (!TileDataSrc::isRowMajor && (srcValidCol == dstValidRow)),
               "Fix: TROWEXPAND row major src tile's valid row must be consistent with dst tile's valid row, or "
               "col major src tile's valid col must be consistent with dst tile's valid row!");
    if (dstValidRow == 0 || dstValidCol == 0 || srcValidRow == 0 || srcValidCol == 0) {
      return;
    }

    constexpr bool isBroadcastSupportType = (sizeof(T) == 2 || sizeof(T) == 4);

    constexpr bool isStaticShape =
      (TileDataSrc::Rows == TileDataSrc::ValidRow) && (TileDataSrc::Cols == TileDataSrc::ValidCol) &&
      (TileDataDst::Rows == TileDataDst::ValidRow) && (TileDataDst::Cols == TileDataDst::ValidCol);

    constexpr unsigned elemPerBlock = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr bool isBroadcast = TileDataSrc::isRowMajor ?
      ((TileDataSrc::Rows == 1) && (TileDataSrc::Cols == TileDataDst::Rows) && (TileDataDst::Cols == elemPerBlock)) :
      ((TileDataSrc::Cols == 1) && (TileDataSrc::Rows == TileDataDst::Rows) && (TileDataDst::Cols == elemPerBlock));

    if constexpr (isBroadcastSupportType && isStaticShape && isBroadcast) {
      /*
        isBroadcastSupportType:
          Only b16 and b32 are supported.
        isStaticShape:
          Broadcast is a special case where the src tile is a single row or column,
          src and dst tile are static shapes to ensure that the tile data is saved continuously.
        isBroadcast:
          [1, M] -> [M, elemPerBlock], src is row major.
          [M, 1] -> [M, elemPerBlock], src is column major.
          The value of sizeof(T) x M is a multiple of 32Byte, it also means that M must be a multiple of 8,
          this constraint is implemented by the Tile basic definition.
      */
      TRowExpandBrcb<T, TileDataDst, TileDataSrc>(dst.data(), src.data());
    } else {
      TRowExpand<T, TileDataDst, TileDataSrc>(dst.data(), src.data(), dstValidRow, dstValidCol);
    }
  }
}
#endif