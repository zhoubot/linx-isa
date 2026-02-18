/**
Copyright (c) 2026 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef COMMON_HPP
#define COMMON_HPP

#include <pto/common/type.hpp>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#endif

namespace pto {
template <typename SrcType, typename DstType>
PTO_INTERNAL constexpr QuantMode_t GetCastPreQuantMode()
{
    QuantMode_t quantPre = QuantMode_t::NoQuant;
    if constexpr (std::is_same<SrcType, float>::value) {
        if constexpr ((std::is_same<DstType, __gm__ half>::value) || (std::is_same<DstType, half>::value)) {
            quantPre = QuantMode_t::F322F16;
        } else if constexpr ((std::is_same<DstType, __gm__ bfloat16_t>::value) ||
                             (std::is_same<DstType, bfloat16_t>::value)) {
            quantPre = QuantMode_t::F322BF16;
        }
    }
    return quantPre;
}

template <typename SrcType, typename DstType>
PTO_INTERNAL constexpr QuantMode_t GetScalarPreQuantMode()
{
    QuantMode_t quantPre = QuantMode_t::NoQuant;
    if constexpr (std::is_same<SrcType, float>::value) {
        if constexpr ((std::is_same<DstType, __gm__ int8_t>::value) || (std::is_same<DstType, __gm__ uint8_t>::value) ||
                      (std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
            quantPre = QuantMode_t::QF322B8_PRE;
        } else if constexpr ((std::is_same<DstType, __gm__ half>::value) || (std::is_same<DstType, half>::value)) {
            quantPre = QuantMode_t::QF322F16_PRE;
        } else if constexpr ((std::is_same<DstType, __gm__ bfloat16_t>::value) ||
                             (std::is_same<DstType, bfloat16_t>::value)) {
            quantPre = QuantMode_t::QF322BF16_PRE;
        }
    } else if constexpr (std::is_same<SrcType, int32_t>::value) {
        if constexpr ((std::is_same<DstType, __gm__ int8_t>::value) || (std::is_same<DstType, __gm__ uint8_t>::value) ||
                      (std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
            quantPre = QuantMode_t::REQ8;
        } else if constexpr ((std::is_same<DstType, __gm__ half>::value) || (std::is_same<DstType, half>::value)) {
            quantPre = QuantMode_t::DEQF16;
        } else if constexpr ((std::is_same<DstType, __gm__ int16_t>::value) ||
                             (std::is_same<DstType, int16_t>::value)) {
            quantPre = QuantMode_t::SHIFTS322S16;
        }
    }
    return quantPre;
}

template <typename SrcType, typename DstType>
PTO_INTERNAL constexpr QuantMode_t GetVectorPreQuantMode()
{
    QuantMode_t quantPre = QuantMode_t::NoQuant;
    if constexpr (std::is_same<SrcType, float>::value) {
        if constexpr ((std::is_same<DstType, __gm__ int8_t>::value) || (std::is_same<DstType, __gm__ uint8_t>::value) ||
                      (std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
            quantPre = QuantMode_t::VQF322B8_PRE;
        }
    } else if constexpr (std::is_same<SrcType, int32_t>::value) {
        if constexpr ((std::is_same<DstType, __gm__ int8_t>::value) || (std::is_same<DstType, __gm__ uint8_t>::value) ||
                      (std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
            quantPre = QuantMode_t::VREQ8;
        } else if constexpr ((std::is_same<DstType, __gm__ half>::value) || (std::is_same<DstType, half>::value)) {
            quantPre = QuantMode_t::VDEQF16;
        } else if constexpr ((std::is_same<DstType, __gm__ int16_t>::value) ||
                             (std::is_same<DstType, int16_t>::value)) {
            quantPre = QuantMode_t::VSHIFTS322S16;
        }
    }
    return quantPre;
}

template <typename DstTileData, typename SrcTileData, typename DstType, typename SrcType, bool isCastQuant>
PTO_INTERNAL void CheckTMovAccToMat()
{
    static_assert((SrcTileData::Loc == TileType::Acc), "Source TileType only support Acc.");
    static_assert((DstTileData::Loc == TileType::Mat), "Destination TileType only support Mat.");
    static_assert(
        (DstTileData::SFractalSize == TileConfig::fractalABSize), "Destination SFractalSize only support 512.");
    static_assert(((DstTileData::Cols * sizeof(DstType) % C0_SIZE_BYTE == 0) && ((DstTileData::Cols) > 0)),
        "Dst Tile Cols * sizeof(DstType) must be multiples of 32 and not 0.");
    static_assert((!SrcTileData::isRowMajor && SrcTileData::SFractal == SLayout::RowMajor),
        "Src fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    static_assert((!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::RowMajor),
        "Dst fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    static_assert(((std::is_same<SrcType, float>::value) || (std::is_same<SrcType, int32_t>::value)),
        "Src data type only support float or int32_t.");
    if constexpr (isCastQuant) {
        static_assert((std::is_same<SrcType, float>::value), "The src data type must be restricted to float.");
        static_assert((std::is_same<DstType, half>::value) || (std::is_same<DstType, bfloat16_t>::value),
            "The output data type must be restricted to half/bfloat16_t.");
    } else {
        if constexpr (std::is_same<SrcType, float>::value) {
            static_assert((std::is_same<DstType, int8_t>::value), "The output data type must be restricted to int8_t.");
        } else if constexpr (std::is_same<SrcType, int32_t>::value) {
            static_assert((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value) ||
                              (std::is_same<DstType, half>::value) || (std::is_same<DstType, int16_t>::value),
                "The output data type must be restricted to int8_t/uint8_t/half/int16_t.");
        }
    }
}
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
