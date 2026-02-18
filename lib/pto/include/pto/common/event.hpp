/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef EVENT_HPP
#define EVENT_HPP

#define EVENT_ID_MAX 8

#include <pto/common/type.hpp>

namespace pto {
  enum class Op : uint16_t {
    TLOAD,          /* GM to Vec/Mat/ */
    TSTORE_VEC,     /* Vec to GM */
    SCALAR,
    TRESHAPE,
    VECTOR,
    TADD,
    TADDS,
    TSUB,
    TMUL,
    TMULS,
    TDIV,
    TDIVS,
    TMIN,
    TMINS,
    TMAX,
    TAND,
    TOR,
    TSEL,
    TEXP,
    TSELS,
    TSQRT,
    TRSQRT,
    TEXPANDS,
    TPARTADD,
    TPARTMAX,
    TPARTMIN,
    TCMPS,
    TMRGSORT,
    TSORT32,
    TCI,
    TGATHER,
    TGATHERB,
    TCVT,
    TROWSUM,
    TROWMAX,
    TROWMIN,
    TROWEXPAND,
    TCOLSUM,
    TCOLMAX,
    TCOLMIN,
    TTRANS,
    TTRI,
    TREM,
    TREMS,
    TSUBS,
    TMAXS,
    TLRELU,
    TMOV_V2V,       /* Vec to Vec */
    TMOV_V2M,       /* Vec to Mat */
    TEXTRACT_V2M,   /* Vec to Mat */
    TMOV_M2B,       /* Mat to Bias */
    TMOV_M2L,       /* Mat to Left */
    TMOV_M2R,       /* Mat to Right */
    TMOV_M2S,       /* Mat to Scaling */
    TMOV_A2V,       /* Acc to Vec */
    TMOV_A2M,       /* Acc to Mat */
    TSTORE_ACC,     /* Acc to GM */
    TSTORE_MAT,     /* Mat to GM */
    TMATMUL,
    TMATMUL_MX,
    TEXTRACT_M2LR,  /* Mat to Left/Right */
    TANDS,
    TORS,
    TSHLS,
    TSHRS,
    TXOR,
    TXORS,
    TEXTRACT_A2M,   /* Acc to Mat */
    TINSERT_A2M,
    OP_COUNT, // The Total number of operations, please add new operations before OP_COUNT
  };

  struct RecordEvent {};

  template<pipe_t SrcPipe, pipe_t DstPipe>
  class EventIdCounter {
    public:
      PTO_INTERNAL static event_t GetNextId() {
        event_t id = NextId();
        NextId() = (event_t)(((uint8_t)NextId() + 1) % EVENT_ID_MAX);
        return id;
      }
      PTO_INTERNAL static void Reset() {
        NextId() = EVENT_ID0;
      }
      PTO_INTERNAL static event_t PeekNextId() {
        return NextId();
      }
    private:
      static event_t& NextId() {
        static event_t id = EVENT_ID0;
        return id;
      }
  };

  template <typename... WaitEvents>
  PTO_INTERNAL void WaitAllEvents(WaitEvents&... events) {
    (events.Wait(), ...);
  }

  template <pipe_t SrcPipe, pipe_t DstPipe>
  PTO_INTERNAL void PtoSetWaitFlag() {
#ifdef PTO_FLAG_TEST
    CceEventIdType token = __pto_set_flag(SrcPipe, DstPipe);
    __pto_wait_flag(SrcPipe, DstPipe, token);
#else
    // Some toolchains compile host-side stubs in addition to AICORE code paths.
    // Guard intrinsic calls so non-AICORE compilation units don't fail to build.
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    set_flag(SrcPipe, DstPipe, EVENT_ID0);
    wait_flag(SrcPipe, DstPipe, EVENT_ID0);
#else
    (void)SrcPipe;
    (void)DstPipe;
#endif
#endif
  }
} // namespace pto
#endif
