/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPREFETCH_HPP
#define TPREFETCH_HPP

namespace pto {

template <typename TileData, typename GlobalData>
PTO_INTERNAL void TPREFETCH_IMPL(TileData &dst, GlobalData &src)
{
    // CPU simulator: prefetch is non-semantic. Keep it as a no-op.
    (void)dst;
    (void)src;
}

} // namespace pto

#endif

