/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_CPU_TPRINT_HPP
#define PTO_CPU_TPRINT_HPP

#include <pto/common/pto_tile.hpp>

#include <cstdio>
#include <type_traits>

namespace pto {

template <typename T> PTO_INTERNAL constexpr const char *GetCpuDTypeName() { return "unknown"; }

#define PTO_CPU_DEFINE_TYPE_NAME(name, ty)                                                              \
  template <> PTO_INTERNAL constexpr const char *GetCpuDTypeName<ty>() { return name; }

PTO_CPU_DEFINE_TYPE_NAME("uint32", std::uint32_t)
PTO_CPU_DEFINE_TYPE_NAME("int32", std::int32_t)
PTO_CPU_DEFINE_TYPE_NAME("uint16", std::uint16_t)
PTO_CPU_DEFINE_TYPE_NAME("int16", std::int16_t)
PTO_CPU_DEFINE_TYPE_NAME("uint8", std::uint8_t)
PTO_CPU_DEFINE_TYPE_NAME("int8", std::int8_t)
PTO_CPU_DEFINE_TYPE_NAME("float32", float)
PTO_CPU_DEFINE_TYPE_NAME("float16", half)

#undef PTO_CPU_DEFINE_TYPE_NAME

template <typename T> PTO_INTERNAL void CpuPrintValue(T val, int col) {
  if (col > 0)
    std::printf(" ");
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, half>) {
    std::printf("%6.2f", static_cast<float>(val));
  } else if constexpr (std::is_integral_v<T>) {
    std::printf("%6d", static_cast<int>(val));
  } else {
    static_assert(sizeof(T) == 0, "Unsupported data type for TPRINT (CPU).");
  }
}

PTO_INTERNAL void CpuPrintHSeparator(int totalCols, int validCols) {
  for (int j = 0; j < totalCols; ++j) {
    if (j > 0)
      std::printf(" ");
    std::printf("------");
    if (j == validCols - 1 && validCols > 0 && validCols < totalCols)
      std::printf("|");
  }
}

template <typename TileDataIn> PTO_INTERNAL const char *CpuTileTypeName() {
  if constexpr (TileDataIn::Loc == TileType::Vec)
    return "Vec";
  if constexpr (TileDataIn::Loc == TileType::Mat)
    return "Mat";
  if constexpr (TileDataIn::Loc == TileType::Left)
    return "Left";
  if constexpr (TileDataIn::Loc == TileType::Right)
    return "Right";
  if constexpr (TileDataIn::Loc == TileType::Acc)
    return "Acc";
  if constexpr (TileDataIn::Loc == TileType::Bias)
    return "Bias";
  if constexpr (TileDataIn::Loc == TileType::Scaling)
    return "Scaling";
  if constexpr (TileDataIn::Loc == TileType::ScaleLeft)
    return "ScaleLeft";
  if constexpr (TileDataIn::Loc == TileType::ScaleRight)
    return "ScaleRight";
  return "Unknown";
}

template <typename TileDataIn> PTO_INTERNAL void TPrintTileImpl(TileDataIn &src) {
  using DType = typename TileDataIn::DType;
  auto validRows = src.GetValidRow();
  auto validCols = src.GetValidCol();
  auto *srcPtr = (DType *)__cce_get_tile_ptr(src.data());

  std::printf("=== [TPRINT Tile] Data Type: %s, Layout: %s, TileType: %s ===\n", GetCpuDTypeName<DType>(),
              GetLayoutName(TileDataIn::BFractal, TileDataIn::SFractal), CpuTileTypeName<TileDataIn>());
  std::printf("  Shape: [%d, %d], Valid Shape: [%d, %d]\n", TileDataIn::Rows, TileDataIn::Cols, validRows, validCols);

  for (int i = 0; i < TileDataIn::Rows; ++i) {
    for (int j = 0; j < TileDataIn::Cols; ++j) {
      auto v = srcPtr[GetTileOffset<TileDataIn>(i, j)];
      CpuPrintValue<DType>(v, j);
      if (j == validCols - 1 && validCols > 0 && validCols < TileDataIn::Cols)
        std::printf("|");
    }
    std::printf("\n");
    if (i == validRows - 1 && validRows > 0 && validRows < TileDataIn::Rows) {
      CpuPrintHSeparator(TileDataIn::Cols, validCols);
      std::printf("\n");
    }
  }
}

template <typename GlobalData> PTO_INTERNAL void TPrintGlobalTensorImpl(GlobalData &src) {
  using DType = typename GlobalData::RawDType;
  auto *ptr = src.data();
  const int n0 = src.GetShape(GlobalTensorDim::DIM_0);
  const int n1 = src.GetShape(GlobalTensorDim::DIM_1);
  const int n2 = src.GetShape(GlobalTensorDim::DIM_2);
  const int n3 = src.GetShape(GlobalTensorDim::DIM_3);
  const int n4 = src.GetShape(GlobalTensorDim::DIM_4);
  const int s0 = src.GetStride(GlobalTensorDim::DIM_0);
  const int s1 = src.GetStride(GlobalTensorDim::DIM_1);
  const int s2 = src.GetStride(GlobalTensorDim::DIM_2);
  const int s3 = src.GetStride(GlobalTensorDim::DIM_3);
  const int s4 = src.GetStride(GlobalTensorDim::DIM_4);

  std::printf("=== [TPRINT GlobalTensor] Data Type: %s, Layout: %d ===\n", GetCpuDTypeName<DType>(),
              static_cast<int>(GlobalData::layout));
  std::printf("  Shape: [%d, %d, %d, %d, %d]\n", n0, n1, n2, n3, n4);

  for (int i0 = 0; i0 < n0; ++i0) {
    for (int i1 = 0; i1 < n1; ++i1) {
      for (int i2 = 0; i2 < n2; ++i2) {
        std::printf("  Batch [%d, %d, %d]:\n", i0, i1, i2);
        for (int r = 0; r < n3; ++r) {
          for (int c = 0; c < n4; ++c) {
            auto off = static_cast<size_t>(i0) * s0 + static_cast<size_t>(i1) * s1 + static_cast<size_t>(i2) * s2 +
                       static_cast<size_t>(r) * s3 + static_cast<size_t>(c) * s4;
            CpuPrintValue<DType>(ptr[off], c);
          }
          std::printf("\n");
        }
      }
    }
  }
}

template <typename T> PTO_INTERNAL void TPRINT_IMPL(T &src) {
  if constexpr (is_tile_data_v<T>) {
    TPrintTileImpl<T>(src);
  } else if constexpr (is_global_data_v<T>) {
    TPrintGlobalTensorImpl<T>(src);
  } else {
    static_assert(sizeof(T) == 0, "TPRINT: Only Tile and GlobalTensor are supported (CPU).");
  }
}

} // namespace pto

#endif
