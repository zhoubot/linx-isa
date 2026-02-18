/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_NPU_A5_TPUSH_POP_HPP
#define PTO_NPU_A5_TPUSH_POP_HPP

#include <pto/common/type.hpp>

#include "pto/npu/a5/TLoad.hpp"
#include "pto/npu/a5/TStore.hpp"

namespace pto {

// Prototype: treat TPUSH/TPOP as GM store/load with an extra `token` operand.
// Cross-core FIFO synchronization via FFTS is added in a dedicated pass/tooling layer.
template <typename GlobalData, typename TileData>
PTO_INTERNAL void TPUSH_IMPL(GlobalData &dst, TileData &src, uint16_t token)
{
    static_assert(TileData::Loc == TileType::Vec || TileData::Loc == TileType::Mat,
        "TPUSH currently supports Vec/Mat tiles only; move Acc tiles to Mat/Vec before pushing.");
    TSTORE_IMPL<TileData, GlobalData, AtomicType::AtomicNone>(dst, src);
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    // Token is an event id in the MTE3 -> MTE2 event pool (0..7). Keep the wait in TPOP_IMPL.
    set_flag(PIPE_MTE3, PIPE_MTE2, static_cast<event_t>(token));
#endif
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TPOP_IMPL(TileData &dst, GlobalData &src, uint16_t token)
{
#if defined(__CCE_IS_AICORE__) || defined(__CCE_AICORE__)
    wait_flag(PIPE_MTE3, PIPE_MTE2, static_cast<event_t>(token));
#endif
    TLOAD_IMPL(dst, src);
}

} // namespace pto

#endif // PTO_NPU_A5_TPUSH_POP_HPP
