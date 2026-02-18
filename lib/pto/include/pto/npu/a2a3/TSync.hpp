/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSYNC_HPP
#define TSYNC_HPP

#include <pto/common/type.hpp>
#include <pto/common/event.hpp>

#define FFTS_BASE_COUNT_WIDTH 0xf
#define FFTS_MODE_VAL 0x2
#define FFTS_MODE_WIDTH 0x3
#define FFTS_MODE_OFFSET 4
#define FFTS_EVENT_ID_WIDTH 0xf
#define FFTS_EVENT_ID_OFFSET 8
namespace pto {
#ifdef __CCE_AICORE__
  // opPipeList maps each operation in Op enum to its corresponding pipeline type.
  // This array is used to determine which hardware pipeline should be used for each operation.
constexpr pipe_t opPipeList[] = {
    PIPE_MTE2 /* TLOAD */, PIPE_MTE3 /* TSTORE_VEC */, PIPE_S /* SCALAR */, PIPE_S /* TRESHAPE */,
    PIPE_V /* VECTOR */, PIPE_V /* TADD */, PIPE_V /* TADDS */, PIPE_V /* TSUB */,
    PIPE_V /* TMUL */, PIPE_V /* TMULS */, PIPE_V /* TDIV */, PIPE_V /* TDIVS */,
    PIPE_V /* TMIN */, PIPE_V /* TMINS */, PIPE_V /* TMAX */, PIPE_V /* TAND */, PIPE_V /* TOR */, PIPE_V /* TSEL */,
    PIPE_V /* TEXP */, PIPE_V /* TSELS */, PIPE_V /* TSQRT */, PIPE_V /* TRSQRT */,
    PIPE_V /* TEXPANDS */, PIPE_V /* TPARTADD */, PIPE_V /* TPARTMAX */, PIPE_V /* TPARTMIN */,
    PIPE_V /* TCMPS */, PIPE_V /* TMRGSORT */, PIPE_V /* TSORT32 */, PIPE_S /* TCI */,
    PIPE_V /* TGATHER */, PIPE_V /* TGATHERB */, PIPE_V /* TCVT */, PIPE_V /* TROWSUM */,
    PIPE_V /* TROWMAX */, PIPE_V /* TROWMIN */, PIPE_V /* TROWEXPAND */, PIPE_V /* TCOLSUM */,
    PIPE_V /* TCOLMAX */, PIPE_V /* TCOLMIN */, PIPE_V /* TTRANS */, PIPE_V /* TTRI */, PIPE_V /* TREM */, 
    PIPE_V /* TREMS */, PIPE_V /* TSUBS */, PIPE_V /* TMAXS */, PIPE_V /* TLRELU */,
    PIPE_V /* TMOV_V2V */, PIPE_FIX /* TMOV_V2M */, PIPE_FIX /* TEXTRACT_V2M */, PIPE_MTE1 /* TMOV_M2B */, 
    PIPE_MTE1 /* TMOV_M2L */, PIPE_MTE1 /* TMOV_M2R */, PIPE_FIX /* TMOV_M2S */, PIPE_FIX /* TMOV_A2V */, 
    PIPE_FIX /* TMOV_A2M */, PIPE_FIX /* TSTORE_ACC */, PIPE_MTE3 /* TSTORE_MAT */, PIPE_M /* TMATMUL */, 
    PIPE_MTE1 /* TEXTRACT_M2LR */, PIPE_V /* TANDS */, PIPE_V /* TORS */, PIPE_V /* TSHLS */, PIPE_V /* TSHRS */,
    PIPE_V /* TXOR */, PIPE_V /* TXORS */, PIPE_FIX /* TEXTRACT_A2M */, PIPE_FIX /* TINSERT_A2M */,
  };

  template <Op OpCode>
  PTO_INTERNAL static constexpr pipe_t GetPipeByOp() {
    if constexpr ((OpCode >= static_cast<Op>(0)) && (OpCode < Op::OP_COUNT)) {
      return opPipeList[static_cast<int>(OpCode)];
    }
    return PIPE_ALL;
  }

  // single pipeline wait, only support Vector pipeline
  template <Op OpCode>
  PTO_INTERNAL void TSYNC_IMPL() {
    constexpr pipe_t pipe = GetPipeByOp<OpCode>();
    PTO_STATIC_ASSERT(pipe == PIPE_V, "Single Op TSYNC only supports Vector PTO Instruction.");
    pipe_barrier((pipe_t)pipe);
  }

  PTO_INTERNAL uint16_t getFFTSMsg(uint16_t mode, uint16_t eventId, uint16_t baseConst = 0x1) {
    return ((baseConst & FFTS_BASE_COUNT_WIDTH) +
      ((mode & FFTS_MODE_WIDTH) << FFTS_MODE_OFFSET) +
      ((eventId & FFTS_EVENT_ID_WIDTH) << FFTS_EVENT_ID_OFFSET));
  }

  template <Op SrcOp, Op DstOp, bool AutoToken = true, event_t EventID = EVENT_ID0>
  struct Event {
    static constexpr Op srcOp = SrcOp;
    static constexpr Op dstOp = DstOp;
    static constexpr pipe_t srcPipe = GetPipeByOp<srcOp>();
    static constexpr pipe_t dstPipe = GetPipeByOp<dstOp>();
    PTO_STATIC_ASSERT(SrcOp != DstOp, "SrcOp is not allowed to be equal to DstOp.");
    PTO_STATIC_ASSERT(dstPipe != srcPipe, "SrcPipe is not allowed to be equal to dstPipe.");

    PTO_INTERNAL static constexpr bool IsCrossCoreEvent() {
      return ((srcOp == Op::TMOV_A2V) && (GetPipeByOp<dstOp>() == PIPE_V)) || // dstOp为搬运到GM的MTE3是否需要考虑
             ((srcOp == Op::TMOV_V2M || srcOp == Op::TEXTRACT_V2M) && (GetPipeByOp<dstOp>() == PIPE_MTE1));
    }

    static constexpr bool IsCrossCore = IsCrossCoreEvent();
    PTO_STATIC_ASSERT(IsCrossCore || (srcPipe != PIPE_ALL), "SrcOp are invalid.");
    PTO_STATIC_ASSERT(IsCrossCore || (dstPipe != PIPE_ALL), "DstOp are invalid.");
    PTO_STATIC_ASSERT((!IsCrossCore) || (!AutoToken), "Cross-core events must manually specify EventID.");

#ifdef PTO_FLAG_TEST
    CceEventIdType token = {};
#else
    const event_t token = AutoToken ? EventIdCounter<srcPipe, dstPipe>::GetNextId() : EventID;
#endif

    PTO_INTERNAL Event& InitAddr(uint64_t fftsAddr) {
      PTO_STATIC_ASSERT(IsCrossCore, "Only cross-core events require setting the initial addr.");
      set_ffts_base_addr(fftsAddr);
      return *this;
    }

    template <uint8_t CrossCoreId = 0xff>
    PTO_INTERNAL Event& Wait() {
      if constexpr (IsCrossCore) {
        PTO_STATIC_ASSERT(CrossCoreId != 0xff,
          "Fix: The cross-core id must be assigned by user when the event is a cross-core event.");
        wait_flag_dev(CrossCoreId);
      } else {
#ifdef PTO_FLAG_TEST
        __pto_wait_flag((pipe_t)srcPipe, (pipe_t)dstPipe, token);
#else
        wait_flag((pipe_t)srcPipe, (pipe_t)dstPipe, token);
#endif
      }
      return *this;
    }

    template <uint8_t CrossCoreId = 0xff>
    PTO_INTERNAL Event& Init() {
      if constexpr (IsCrossCore) {
        PTO_STATIC_ASSERT(CrossCoreId != 0xff,
          "Fix: The cross-core id must be assigned by user when the event is a cross-core event.");
        ffts_cross_core_sync(srcPipe, getFFTSMsg(FFTS_MODE_VAL, CrossCoreId));
      } else {
#ifdef PTO_FLAG_TEST
        token = __pto_set_flag((pipe_t)srcPipe, (pipe_t)dstPipe);
#else
        set_flag((pipe_t)srcPipe, (pipe_t)dstPipe, token);
#endif
      }
      return *this;
    }

    template <uint8_t CrossCoreId = 0xff>
    PTO_INTERNAL Event& Record() {
      return Init<CrossCoreId>();
    }

    PTO_INTERNAL Event& operator=(RecordEvent) {
      PTO_STATIC_ASSERT(!IsCrossCore,
        "Fix: The cross-core event must be manually initialized and specify the cross-core ID.");
      return Init();
    }
  };
#endif
} // namespace pto
#endif
