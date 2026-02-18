/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include <stdint.h>
#include <type_traits>
#include <pto/common/type.hpp>

namespace pto {
enum class TileType {
  Vec,
  Mat,
  Left,
  Right,
  Acc,
  Bias,
  Scaling,
  ScaleLeft,
  ScaleRight,
};

enum class BLayout {
  RowMajor = 0,
  ColMajor = 1,
};

enum class SLayout {
  NoneBox = 0,
  RowMajor = 1,
  ColMajor = 2,
};

// returns the memory qualifier for a given TileType and data type.
// compilation errors occur if the TileType does not have a specialized version.
template <TileType L, typename DType> struct MemoryQualifier {};

template <typename DType> struct MemoryQualifier<TileType::Vec, DType> {
#ifdef __PTO_AUTO__
  using type = __ubuf__ DType;
#else
  using type = __ubuf__ DType *;
#endif
};

template <typename DType> struct MemoryQualifier<TileType::Mat, DType> {
#ifdef __PTO_AUTO__
  using type = __cbuf__ DType;
#else
  using type = __cbuf__ DType *;
#endif
};

template <typename DType> struct MemoryQualifier<TileType::Left, DType> {
#ifdef __PTO_AUTO__
  using type = __ca__ DType;
#else
  using type = __ca__ DType *;
#endif
};

template <typename DType> struct MemoryQualifier<TileType::Right, DType> {
#ifdef __PTO_AUTO__
  using type = __cb__ DType;
#else
  using type = __cb__ DType *;
#endif
};

template <typename DType> struct MemoryQualifier<TileType::Acc, DType> {
#ifdef __PTO_AUTO__
  using type = __cc__ DType;
#else
  using type = __cc__ DType *;
#endif
};

template <typename DType> struct MemoryQualifier<TileType::Bias, DType> {
  using type = uint64_t;
};

template <typename DType> struct MemoryQualifier<TileType::Scaling, DType> {
#ifdef __PTO_AUTO__
  using type = __fbuf__ DType;
#else
  using type = __fbuf__ DType *;
#endif
};

template <typename DType>
struct MemoryQualifier<TileType::ScaleLeft, DType> {
#ifdef __PTO_AUTO__
    using type = __ca__ DType;
#else
    using type = __ca__ DType *;
#endif
};

template <typename DType>
struct MemoryQualifier<TileType::ScaleRight, DType> {
#ifdef __PTO_AUTO__
    using type = __cb__ DType;
#else
    using type = __cb__ DType *;
#endif
};

PTO_INTERNAL constexpr const __gm__ char *
GetLayoutName(BLayout bType, SLayout sType) noexcept {
  switch (sType) {
  case SLayout::NoneBox:
    return (bType == BLayout::RowMajor) ? "ND" : "DN";
  case SLayout::RowMajor:
    return (bType == BLayout::RowMajor) ? "Zz" : "Nz";
  case SLayout::ColMajor:
    return (bType == BLayout::RowMajor) ? "Zn" : "Nn";
  default:
    return "Unknown";
  }
}

} // namespace pto

#endif