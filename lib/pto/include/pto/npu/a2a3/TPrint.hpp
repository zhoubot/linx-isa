/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under
the terms and conditions of CANN Open Software License Agreement Version 2.0
(the "License"). Please refer to the License for details. You may not use this
file except in compliance with the License. THIS SOFTWARE IS PROVIDED ON AN "AS
IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A
PARTICULAR PURPOSE. See LICENSE in the root of the software repository for the
full text of the License.
*/

#ifndef __PTO_TPRINT_A2A3__
#define __PTO_TPRINT_A2A3__

#include "pto/common/pto_tile.hpp"
#include <type_traits>

namespace pto {

#ifdef _DEBUG


template <typename T> PTO_INTERNAL constexpr const __gm__ char *GetDTypeName() {
  return "unknown";
}

#define DEFINE_TYPE_NAME_GROUP(name, ...)                                      \
  template <>                                                                  \
  PTO_INTERNAL constexpr const __gm__ char *GetDTypeName<__VA_ARGS__>() {      \
    return name;                                                               \
  }

DEFINE_TYPE_NAME_GROUP("uint32", std::uint32_t)
DEFINE_TYPE_NAME_GROUP("int32", std::int32_t)
DEFINE_TYPE_NAME_GROUP("uint16", std::uint16_t)
DEFINE_TYPE_NAME_GROUP("int16", std::int16_t)
DEFINE_TYPE_NAME_GROUP("uint8", std::uint8_t)
DEFINE_TYPE_NAME_GROUP("int8", std::int8_t)
DEFINE_TYPE_NAME_GROUP("float32", float)
DEFINE_TYPE_NAME_GROUP("float16", half)

template <typename T> PTO_INTERNAL void PrintValue(T &val, int col) {
  if (col > 0)
    cce::printf(" ");
  if constexpr (is_same_v<T, float> || is_same_v<T, half>) {
    cce::printf("%6.2f", static_cast<float>(val));
  } else if constexpr (is_integral_v<T>) {
    cce::printf("%6d", static_cast<int>(val));
  } else {
    static_assert(sizeof(T) == 0, "Unsupported data type for Print.");
  }
}

template <typename TileDataIn>
PTO_INTERNAL void PrintTileRow(__ubuf__ typename TileDataIn::DType *src,
                               int row, int validCols) {
  using DType = typename TileDataIn::DType;
  for (int j = 0; j < TileDataIn::Cols; ++j) {
    DType val = *(src + GetTileOffset<TileDataIn>(row, j));
    PrintValue(val, j);
    if (j == validCols - 1 && validCols > 0 && validCols < TileDataIn::Cols) {
      cce::printf("|");
    }
  }
}

PTO_INTERNAL void PrintHorizontalSeparator(int totalCols, int validCols) {
  for (int j = 0; j < totalCols; ++j) {
    if (j > 0)
      cce::printf(" ");
    cce::printf("------"); // 8 dashes to match %8 width

    if (j == validCols - 1 && validCols > 0 && validCols < totalCols) {
      cce::printf("|");
    }
  }
}

template <typename TileDataIn>
__tf__ PTO_INTERNAL void
TPrintTileImpl(typename TileDataIn::TileDType __in__ srcData, int validRows,
               int validCols) {
  using DType = typename TileDataIn::DType;
  __ubuf__ DType *src = (__ubuf__ DType *)__cce_get_tile_ptr(srcData);

  cce::printf("=== [TPRINT Tile] Data Type: %s, Layout: %s, TileType: %s ===\n",
              GetDTypeName<DType>(),
              GetLayoutName(TileDataIn::BFractal, TileDataIn::SFractal), "Vec");
  cce::printf("  Shape: [%d, %d], Valid Shape: [%d, %d]\n", TileDataIn::Rows,
              TileDataIn::Cols, validRows, validCols);
  for (int i = 0; i < TileDataIn::Rows; ++i) {
    PrintTileRow<TileDataIn>(src, i, validCols);
    cce::printf("\n");
    if (i == validRows - 1 && validRows > 0 && validRows < TileDataIn::Rows) {
      PrintHorizontalSeparator(TileDataIn::Cols, validCols);
      cce::printf("\n");
    }
  }
}

template <typename T>
PTO_INTERNAL void PrintRow(T *dataPtr, int i0, int i1, int i2, int r, int n4,
                           int s0, int s1, int s2, int s3, int s4) {
  for (int c = 0; c < n4; ++c) {
    size_t offset = i0 * s0 + i1 * s1 + i2 * s2 + r * s3 + c * s4;
    auto val = dataPtr[offset];
    PrintValue(val, c);
  }
  cce::printf("\n");
}

template <typename T>
PTO_INTERNAL void PrintGlobalTensorNDOrDN(T *dataPtr, int n0, int n1, int n2,
                                          int n3, int n4, int s0, int s1,
                                          int s2, int s3, int s4) {
  cce::printf("  Shape: [%d, %d, %d, %d, %d]\n", n0, n1, n2, n3, n4);
  // traverse the batch according to [n0, n1, n2]
  for (int i0 = 0; i0 < n0; ++i0) {
    for (int i1 = 0; i1 < n1; ++i1) {
      for (int i2 = 0; i2 < n2; ++i2) {
        cce::printf("  Batch [%d, %d, %d]:\n", i0, i1, i2);
        // print 2D matrix (Row: n3, Col: n4)
        for (int r = 0; r < n3; ++r) {
          PrintRow(dataPtr, i0, i1, i2, r, n4, s0, s1, s2, s3, s4);
        }
      }
    }
  }
}

template <typename T>
PTO_INTERNAL void PrintGlobalTensorNZ(T *dataPtr, int n0, int n1, int n2,
                                      int n3, int n4, int s0, int s1, int s2,
                                      int s3, int s4) {
  // Shape<1, Cols/(C0Size/sizeof(T)), Rows/FractalRow, FractalRow, C0Size/sizeof(T)>
  // Stride<C*R, R*C0Size/sizeof(T), FractalRow*C0Size/sizeof(T), C0Size/sizeof(T), 1>
  int logical_rows = n2 * n3;
  int logical_cols = n1 * n4;
  cce::printf("  Logical Shape: [%d, %d]\n", logical_rows, logical_cols);
  for (int r = 0; r < logical_rows; ++r) {
    for (int c = 0; c < logical_cols; ++c) {
      int block_row = r / n3;
      int in_block_row = r % n3;
      int block_col = c / n4;
      int in_block_col = c % n4;
      size_t offset = block_row * s2 + block_col * s1 + in_block_row * s3 +
                      in_block_col * s4;
      auto val = dataPtr[offset];
      PrintValue(val, c);
    }
    cce::printf("\n");
  }
}

template <typename GlobalData>
PTO_INTERNAL void TPrintGlobalTensorImpl(GlobalData &src) {
  using DType = typename GlobalData::DType;
  using ElemType = typename GlobalData::RawDType;

  int n0 = src.GetShape(GlobalTensorDim::DIM_0);
  int n1 = src.GetShape(GlobalTensorDim::DIM_1);
  int n2 = src.GetShape(GlobalTensorDim::DIM_2);
  int n3 = src.GetShape(GlobalTensorDim::DIM_3);
  int n4 = src.GetShape(GlobalTensorDim::DIM_4);

  int s0 = src.GetStride(GlobalTensorDim::DIM_0);
  int s1 = src.GetStride(GlobalTensorDim::DIM_1);
  int s2 = src.GetStride(GlobalTensorDim::DIM_2);
  int s3 = src.GetStride(GlobalTensorDim::DIM_3);
  int s4 = src.GetStride(GlobalTensorDim::DIM_4);

  typename GlobalData::DType *dataPtr = src.data();

  if constexpr (GlobalData::layout == Layout::ND ||
                GlobalData::layout == Layout::DN) {
    cce::printf("=== [TPRINT GlobalTensor] Data Type: %s, Layout: %s ===\n",
                GetDTypeName<ElemType>(),
                GlobalData::layout == Layout::ND ? "ND" : "DN");
    PrintGlobalTensorNDOrDN(dataPtr, n0, n1, n2, n3, n4, s0, s1, s2, s3, s4);
  } else if constexpr (GlobalData::layout == Layout::NZ) {
    cce::printf("=== [TPRINT GlobalTensor] Data Type: %s, Layout: %s ===\n",
                GetDTypeName<ElemType>(), "NZ");
    PrintGlobalTensorNZ(dataPtr, n0, n1, n2, n3, n4, s0, s1, s2, s3, s4);
  } else {
    static_assert(sizeof(GlobalData) == 0, "Unsupported GlobalTensor layout.");
  }
}

template <typename T> PTO_INTERNAL void TPRINT_IMPL(T &src) {
  pipe_barrier(PIPE_ALL);
  if constexpr (is_tile_data_v<T>) {
    static_assert(T::Loc == TileType::Vec,
                  "TileType of source tile must be Vec.");

    int validRows = src.GetValidRow();
    int validCols = src.GetValidCol();
    TPrintTileImpl<T>(src.data(), validRows, validCols);
    return;
  } else if constexpr (is_global_data_v<T>) {
    TPrintGlobalTensorImpl<T>(src);
    return;
  } else {
    static_assert(sizeof(T) == 0,
                  "TPRINT: Only Tile and GlobalTensor are supported.");
  }
}

#else  // !_DEBUG

template <typename T> PTO_INTERNAL void TPRINT_IMPL(T &src) { (void)src; }

#endif // _DEBUG
} // namespace pto

#endif
