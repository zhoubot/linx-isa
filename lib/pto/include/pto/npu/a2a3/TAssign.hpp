/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TTILE_ASSIGN
#define TTILE_ASSIGN
#include <cstdint>
#include <pto/common/pto_tile.hpp>

namespace pto {
template <typename T, typename AddrType>
PTO_INTERNAL void TASSIGN_IMPL(T &obj, AddrType addr) {
  if constexpr (is_tile_data_v<T> || is_conv_tile_v<T>) {
#ifndef __PTO_AUTO__
    static_assert(std::is_integral_v<AddrType>,
                  "Tile can only be assigned with address of int type.");
    obj.assignData(reinterpret_cast<typename T::TileDType>(
        static_cast<std::uintptr_t>(addr)));
#else
    return;
#endif
  } else {
    static_assert(is_global_data_v<T>,
                  "Only Tile and GlobalTensor data types are supported.");
    static_assert(
        std::is_pointer_v<AddrType>,
        "GlobalTensor can only be assigned with address of pointer type.");
    static_assert(
        std::is_same_v<std::remove_cv_t<std::remove_pointer_t<AddrType>>, typename T::DType>,
        "GlobalTensor can only be assigned with pointer of same data type.");
    obj.SetAddr(addr);
  }
}
} // namespace pto
#endif