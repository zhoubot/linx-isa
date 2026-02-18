/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_CPU_PARALLEL_HPP
#define PTO_CPU_PARALLEL_HPP

#include <algorithm>
#include <cstddef>
#include <thread>
#include <vector>

// Tuning knobs (can be overridden via compile definitions).
#ifndef PTO_CPU_PARALLEL_THRESHOLD_ELEMS
#define PTO_CPU_PARALLEL_THRESHOLD_ELEMS 16384u
#endif

#ifndef PTO_CPU_MAX_THREADS
#define PTO_CPU_MAX_THREADS 0u
#endif

// Vectorization hints (portable fallbacks).
#if defined(__clang__)
#define PTO_CPU_PRAGMA(X) _Pragma(#X)
#define PTO_CPU_VECTORIZE_LOOP PTO_CPU_PRAGMA(clang loop vectorize(enable) interleave(enable))
#elif defined(__GNUC__)
#define PTO_CPU_PRAGMA(X) _Pragma(#X)
#define PTO_CPU_VECTORIZE_LOOP PTO_CPU_PRAGMA(GCC ivdep)
#else
#define PTO_CPU_VECTORIZE_LOOP
#endif

namespace pto::cpu {

inline unsigned get_thread_count() noexcept
{
    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) {
        hw = 1;
    }
    if constexpr (PTO_CPU_MAX_THREADS != 0u) {
        hw = std::min<unsigned>(hw, PTO_CPU_MAX_THREADS);
    }
    return std::max<unsigned>(1, hw);
}

template <typename Fn>
inline void parallel_for_1d(std::size_t begin, std::size_t end, std::size_t total_work_elems, Fn fn)
{
    constexpr std::size_t SIZE_TWO = 2;
    const std::size_t count = (end > begin) ? (end - begin) : 0;
    if (count == 0) {
        return;
    }

    if (total_work_elems < static_cast<std::size_t>(PTO_CPU_PARALLEL_THRESHOLD_ELEMS) || count < SIZE_TWO) {
        for (std::size_t i = begin; i < end; ++i) {
            fn(i);
        }
        return;
    }

    const unsigned threads = std::min<unsigned>(get_thread_count(), static_cast<unsigned>(count));
    if (threads <= 1) {
        for (std::size_t i = begin; i < end; ++i) {
            fn(i);
        }
        return;
    }

    const std::size_t chunk = (count + threads - 1) / threads;
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        const std::size_t b = begin + static_cast<std::size_t>(t) * chunk;
        const std::size_t e = std::min(end, b + chunk);
        if (b >= e) {
            break;
        }
        workers.emplace_back([&, b, e]() {
            for (std::size_t i = b; i < e; ++i) {
                fn(i);
            }
        });
    }
    for (auto &w : workers) {
        w.join();
    }
}

template <typename Fn>
inline void parallel_for_rows(std::size_t rows, std::size_t cols, Fn fn)
{
    parallel_for_1d(0, rows, rows * cols, fn);
}

} // namespace pto::cpu

#endif

