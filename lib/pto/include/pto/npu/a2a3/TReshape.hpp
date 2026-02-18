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

#ifndef __PTO_RESHAPE_A2A3__
#define __PTO_RESHAPE_A2A3__

#include "pto/common/pto_tile.hpp"
#include <type_traits>

namespace pto {


template <typename TileDataOut, typename TileDataIn>
PTO_INTERNAL void TRESHAPE_IMPL(TileDataOut &dst, TileDataIn &src) {
  static_assert(is_tile_data_v<TileDataIn>, "input must be a Tile instance.");
  static_assert(is_tile_data_v<TileDataOut>, "output must be a Tile instance.");

  using DType = typename TileDataIn::DType;
  using NewElement = typename TileDataOut::DType;

  constexpr auto Loc = TileDataIn::Loc;
  constexpr auto NewLoc = TileDataOut::Loc;

  constexpr int Numel = TileDataIn::Numel;
  constexpr int NewNumel = TileDataOut::Numel;

  constexpr auto SFractal = TileDataIn::SFractal;
  constexpr auto NewSFractal = TileDataOut::SFractal;

  // 1. TileType must match
  static_assert(Loc == NewLoc,
                "TRESHAPE: Source and target TileType must be identical.");

  // 2. Byte size must match
  static_assert(sizeof(DType) * Numel == sizeof(NewElement) * NewNumel,
                "TRESHAPE: Total byte size must match.");

  // 3. reshape between non-boxed and boxed tile is not allowed.
  static_assert(
      (SFractal == SLayout::NoneBox && NewSFractal == SLayout::NoneBox) ||
          (SFractal != SLayout::NoneBox && NewSFractal != SLayout::NoneBox),
      "TRESHAPE: Cannot reshape between boxed and non-boxed layouts.");

  TASSIGN_IMPL(dst, reinterpret_cast<uintptr_t>(src.data()));
}
} // namespace pto

#endif