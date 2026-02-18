/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef T_CARRY_OP_HPP
#define T_CARRY_OP_HPP

#include <pto/common/constants.hpp>

#include "TAdd.hpp"
#include "TAddS.hpp"
#include "TSub.hpp"
#include "TSubS.hpp"

namespace pto {

template <typename TileData>
PTO_INTERNAL void TADDC_IMPL(TileData &dst, TileData &src0, TileData &src1, TileData &src2)
{
    TADD_IMPL(dst, src0, src1);
    pipe_barrier(PIPE_V);
    TADD_IMPL(dst, dst, src2);
}

template <typename TileData>
PTO_INTERNAL void TSUBC_IMPL(TileData &dst, TileData &src0, TileData &src1, TileData &src2)
{
    TSUB_IMPL(dst, src0, src1);
    pipe_barrier(PIPE_V);
    TADD_IMPL(dst, dst, src2);
}

template <typename TileData>
PTO_INTERNAL void TADDSC_IMPL(TileData &dst, TileData &src0, typename TileData::DType scalar, TileData &src1)
{
    TADDS_IMPL(dst, src0, scalar);
    pipe_barrier(PIPE_V);
    TADD_IMPL(dst, dst, src1);
}

template <typename TileData>
PTO_INTERNAL void TSUBSC_IMPL(TileData &dst, TileData &src0, typename TileData::DType scalar, TileData &src1)
{
    TSUBS_IMPL(dst, src0, scalar);
    pipe_barrier(PIPE_V);
    TADD_IMPL(dst, dst, src1);
}

} // namespace pto

#endif
