/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef __UIILS_HPP__
#define __UIILS_HPP__

#include <pto/common/constants.hpp>
#pragma once

namespace pto {
PTO_INTERNAL void SetContinuousMask(unsigned n) {
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    set_vector_mask(static_cast<uint64_t>(
                        (n > MASK_LEN) ? (((static_cast<uint64_t>(1)) << static_cast<uint32_t>(n - MASK_LEN)) - 1) : 0),
        static_cast<uint64_t>(
            (n >= MASK_LEN) ? 0xffffffffffffffff : (((static_cast<uint64_t>(1)) << static_cast<uint32_t>(n)) - 1)));
#else
    (void)n;
#endif
}

template <int index>
PTO_INTERNAL void movemask(uint64_t mask) {
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    if constexpr (index == 0) {
        asm volatile("MOVEMASK MASK[0], %0\n" ::"r"(mask));
    } else if constexpr (index == 1) {
        asm volatile("MOVEMASK MASK[1], %0\n" ::"r"(mask));
    } else {
        PTO_STATIC_ASSERT((index <= 1), "movemask: error mask index.");
    }
#else
    (void)mask;
    PTO_STATIC_ASSERT((index <= 1), "movemask: error mask index.");
#endif
}

PTO_INTERNAL void SetVectorCount(uint64_t n) {
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    set_vector_mask(0, n);
#else
    (void)n;
#endif
}

template <typename T>
PTO_INTERNAL void SetFullVecMaskByDType() {
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    set_vector_mask(-1, -1);
#else
    (void)sizeof(T);
#endif
}

template <typename T>
PTO_INTERNAL void SetContMaskByDType(unsigned n) {
    SetContinuousMask(n);
}

PTO_INTERNAL int32_t CeilDivision(int32_t num1, int32_t num2) {
    if (num2 == 0) {
        return 0;
    }
    return (num1 + num2 - 1) / num2;
}

template <typename T>
PTO_INTERNAL T CeilAlignment(T num1, T num2) {
    if (num2 == 0) {
        return 0;
    }
    return (num1 + num2 - 1) / num2 * num2;
}
} // namespace pto

#endif
