/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_UTILS_H
#define PTO_UTILS_H

#include <pto/common/type.hpp>

namespace pto{
    const uint32_t VECTOR_REG_WIDTH = 256;
    const uint32_t VECTOR_REG_WIDTH_2XVL = 512;
    constexpr uint32_t SHIFT_MX_ADDR = 4;

    enum class DistVST {
        DIST_NORM_B8,
        DIST_NORM_B16,
        DIST_NORM_B32,
        DIST_ONEPT_B8,
        DIST_ONEPT_B16,
        DIST_ONEPT_B32,
        DIST_PK_B16,
        DIST_PK_B32,
        DIST_INTLV_B8,
        DIST_INTLV_B16,
        DIST_PK_B64,
        DIST_INTLV_B32,
        DIST_PK4_B32,
        DIST_MRG4CHN_B8,
        DIST_MRG2CHN_B8,
        DIST_MRG2CHN_B16,
        DIST_NORM,
        DIST_ONEPT
    };
    
    template <typename T, DistVST dist> PTO_INTERNAL constexpr DistVST GetDistVst()
    {
        if constexpr (dist == DistVST::DIST_NORM) {
            static_assert(SupportBytes<T, 1, 2, 4>(), "DistVST DIST_NORM only support type b8/b16/b32 on current device");
            if constexpr (sizeof(T) == 1) {
                return DistVST::DIST_NORM_B8;
            } else if constexpr (sizeof(T) == 2) {
                return DistVST::DIST_NORM_B16;
            } else if constexpr (sizeof(T) == 4) {
                return DistVST::DIST_NORM_B32;
            }
        } else if constexpr (dist == DistVST::DIST_ONEPT) {
          static_assert(SupportBytes<T, 1, 2, 4>(),
                        "DistVST DIST_ONEPT only support type b8/b16/b32 on "
                        "current device");
          if constexpr (sizeof(T) == 1) {
            return DistVST::DIST_ONEPT_B8;
          } else if constexpr (sizeof(T) == 2) {
            return DistVST::DIST_ONEPT_B16;
          } else if constexpr (sizeof(T) == 4) {
            return DistVST::DIST_ONEPT_B32;
          }
        }
        return dist;
    }

    template <typename T, typename U>
    PTO_INTERNAL MaskReg PSetWithType(U dist)
    {
        if constexpr (sizeof(T) == sizeof(float)) {
            return pset_b32(dist);
        } else if constexpr (sizeof(T) == sizeof(half)) {
            return pset_b16(dist);
        } else if constexpr (sizeof(T) == sizeof(uint8_t)) {
            return pset_b8(dist);
        }
    }
    
    template<typename T>
    PTO_INTERNAL uint64_t GetScaleAddr(T* dst)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(dst);
        return addr >> SHIFT_MX_ADDR;
    }

} // end pto

#endif
