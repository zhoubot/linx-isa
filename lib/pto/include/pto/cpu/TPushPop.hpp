/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_CPU_TPUSH_POP_HPP
#define PTO_CPU_TPUSH_POP_HPP

#include <pto/common/type.hpp>

#include "pto/cpu/TLoad.hpp"
#include "pto/cpu/TStore.hpp"

namespace pto {

template <typename GlobalData, typename TileData>
PTO_INTERNAL void TPUSH_IMPL(GlobalData &dst, TileData &src, uint16_t token)
{
    (void)token;
    TSTORE_IMPL(dst, src);
}

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TPOP_IMPL(TileData &dst, GlobalData &src, uint16_t token)
{
    (void)token;
    TLOAD_IMPL(dst, src);
}

} // namespace pto

#endif // PTO_CPU_TPUSH_POP_HPP
