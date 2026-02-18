/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSHIFT_HPP
#define TSHIFT_HPP

#include <cstddef>
#include <type_traits>

namespace pto {

template <typename TileData>
__tf__ PTO_INTERNAL typename TileData::DType ReadScalar0(typename TileData::TileDType __in__ src)
{
    __ubuf__ typename TileData::DType *ptr = (__ubuf__ typename TileData::DType *)__cce_get_tile_ptr(src);
    return ptr[0];
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TSHL_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1)
{
    using T = typename TileDataSrc0::DType;
    static_assert(std::is_same_v<T, typename TileDataDst::DType>, "TSHL: dst/src0 dtype mismatch");
    static_assert(std::is_same_v<T, typename TileDataSrc1::DType>, "TSHL: shift dtype mismatch");
    static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc0::Loc == TileType::Vec &&
                      TileDataSrc1::Loc == TileType::Vec,
                  "TSHL: only supports Vec tiles");

    const T sh = ReadScalar0<TileDataSrc1>(src1.data());
    TSHLS_IMPL(dst, src0, sh);
}

template <typename TileDataDst, typename TileDataSrc0, typename TileDataSrc1>
PTO_INTERNAL void TSHR_IMPL(TileDataDst &dst, TileDataSrc0 &src0, TileDataSrc1 &src1)
{
    using T = typename TileDataSrc0::DType;
    static_assert(std::is_same_v<T, typename TileDataDst::DType>, "TSHR: dst/src0 dtype mismatch");
    static_assert(std::is_same_v<T, typename TileDataSrc1::DType>, "TSHR: shift dtype mismatch");
    static_assert(TileDataDst::Loc == TileType::Vec && TileDataSrc0::Loc == TileType::Vec &&
                      TileDataSrc1::Loc == TileType::Vec,
                  "TSHR: only supports Vec tiles");

    const T sh = ReadScalar0<TileDataSrc1>(src1.data());
    TSHRS_IMPL(dst, src0, sh);
}

} // namespace pto

#endif
