/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCOLEXPANDSUB_HPP
#define TCOLEXPANDSUB_HPP

#include <pto/common/constants.hpp>

namespace pto {

template <typename TileDataDst, typename TileDataSrc1>
PTO_INTERNAL void TCOLEXPANDSUB_IMPL(TileDataDst &dst, TileDataDst &src0, TileDataSrc1 &src1)
{
    using T = typename TileDataDst::DType;
    static_assert(sizeof(T) * TileDataDst::Rows * TileDataDst::Cols <= TMP_UB_SIZE,
                  "TCOLEXPANDSUB: scratch tile too large for TMP_UB");

    TileDataDst tmp;
    TASSIGN_IMPL(tmp, TMP_UB_OFFSET);
    TCOLEXPAND_IMPL(tmp, src1);
    pipe_barrier(PIPE_V);
    TSUB_IMPL(dst, src0, tmp);
}

} // namespace pto

#endif
