/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_CPUSTUB_HPP
#define PTO_CPUSTUB_HPP

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdint>

// CPU simulator assertion helper (always enabled).
#define PTO_CPU_STUB_ASSERT(cond)                                                                    \
    do {                                                                                             \
        if (!(cond)) {                                                                               \
            std::fprintf(stderr,                                                                     \
                "[PTO][CA] Constraint violated. Condition: %s. Hint: see docs/coding/debug.md\n",    \
                #cond);                                                                              \
            std::abort();                                                                            \
        }                                                                                            \
    } while (0)

#define __global__
#define AICORE
#define __aicore__
#define __gm__
#define __out__
#define __in__
#define __ubuf__
#define __cbuf__
#define __ca__
#define __cb__
#define __cc__
#define __fbuf__
#define __tf__

typedef void* aclrtStream;
typedef int pipe_t;
const pipe_t PIPE_S = 0;
const pipe_t PIPE_V = 1;
const pipe_t PIPE_MTE1 = 2;
const pipe_t PIPE_MTE2 = 3;
const pipe_t PIPE_MTE3 = 4;
const pipe_t PIPE_M = 5;
const pipe_t PIPE_ALL = 6;
inline void pipe_barrier(pipe_t pipe)
{
    (void)pipe;
}

constexpr pipe_t opPipeList[] = {
};

#define aclFloat16ToFloat(x) ((float)(x)
#define aclInit(x)
#define aclrtSetDevice(x)

#define aclrtCreateStream(x)

static inline void aclrtMallocHost(void**p, size_t sz){
    PTO_CPU_STUB_ASSERT(sz != 0);
    *p = malloc(sz);
}

#define aclrtMalloc(a,b,c) aclrtMallocHost(a,b)

#define aclrtMemcpy(dst, sz_dst, src, sz_src, type) {for(size_t i = 0; i<sz_src && i<sz_dst; i++) reinterpret_cast<char*>(dst)[i] = reinterpret_cast<char*>(src)[i];} 

#define aclrtSynchronizeStream(x)
#define aclrtFree(x) free(x)
#define aclrtFreeHost(x) free(x)
#define aclrtDestroyStream(x)
#define aclrtResetDevice(x)
#define aclFinalize(x)
#define set_flag(a,b,c)
#define wait_flag(a,b,c)
#define __cce_get_tile_ptr(x) x

typedef int event_t;
#define EVENT_ID0 0

// --- SPMD helpers for CPU simulator ---
//
// When ptoas emits `get_block_idx()` / `get_block_num()` in CPU-simulator mode,
// we provide a simple single-threaded "multi-block" model:
// - Python test harness sets `pto_cpu_block_num` and iterates `pto_cpu_block_idx`
//   before invoking the generated `pto_kernel_cpu(...)`.
// - The kernel can partition work using these accessors, matching NPU behavior.
//
// Use weak definitions so multiple CPU-simulator translation units can include
// this header without ODR/link errors.
extern "C" {
__attribute__((weak, visibility("default"))) uint32_t pto_cpu_block_idx = 0;
__attribute__((weak, visibility("default"))) uint32_t pto_cpu_block_num = 1;
}

static inline uint32_t get_block_idx() { return pto_cpu_block_idx; }
static inline uint32_t get_block_num() { return pto_cpu_block_num; }

#endif
