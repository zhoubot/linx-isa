/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef COMMON_HPP
#define COMMON_HPP

#include "datatype.hpp"
#include <pto/common/type.hpp>

namespace pto {

    template <typename T>
    PTO_INTERNAL uint32_t GetByteSize(const uint32_t value) {
        if constexpr (std::is_same<T, float4_e1m2x2_t>::value || std::is_same<T, float4_e2m1x2_t>::value) {
            return value >> 1; // fp4 4bits
        }
        return sizeof(T) * value;
    }

    template <typename T, int U, int... Args> AICORE constexpr bool SupportBytes()
    {
        if constexpr (sizeof...(Args) > 0) {
            return sizeof(T) == U || SupportBytes<T, Args...>();
        }
        return sizeof(T) == U;
    }

    using MaskReg = vector_bool;
    using UnalignReg = vector_align;
    using AddrReg = vector_address;

    template <typename T> PTO_INTERNAL MaskReg CreatePredicateImpl(uint32_t &scalar)
    {
        MaskReg reg;
        if constexpr (sizeof(T) == 1) {
            reg = plt_b8(scalar, POST_UPDATE);
        } else if constexpr (sizeof(T) == 2) {
            reg = plt_b16(scalar, POST_UPDATE);
        } else if constexpr (sizeof(T) == 4) {
            reg = plt_b32(scalar, POST_UPDATE);
        }
        return reg;
    }

    template <typename T>
    PTO_INTERNAL MaskReg CreatePredicate(uint32_t &scalar)
    {
        return CreatePredicateImpl<T>(scalar);
    }

    template <typename T> struct RegTensor {
        PTO_INTERNAL RegTensor(){};
        using RegType = typename TypeGet<T>::T;
        RegType reg;

        PTO_INTERNAL operator RegType &()
        {
            return reg;
        }
        AICORE void Print() const;
    };

    template <typename SrcType, typename DstType>
    PTO_INTERNAL constexpr QuantMode_t GetCastPreQuantMode()
    {
        QuantMode_t quantPre = QuantMode_t::NoQuant;
        if constexpr (std::is_same<DstType, half>::value) {
            quantPre = QuantMode_t::F322F16;
        } else if constexpr (std::is_same<DstType, bfloat16_t>::value) {
            quantPre = QuantMode_t::F322BF16;
        }
        return quantPre;
    }

    template <typename SrcType, typename DstType>
    PTO_INTERNAL constexpr QuantMode_t GetScalarPreQuantMode()
    {
        QuantMode_t quantPre = QuantMode_t::NoQuant;
        if constexpr (std::is_same<SrcType, float>::value) {
            if constexpr ((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
                quantPre = QuantMode_t::QF322B8_PRE;
            } else if constexpr (std::is_same<DstType, hifloat8_t>::value) {
                quantPre = QuantMode_t::QF322HIF8_PRE;
            } else if constexpr (std::is_same<DstType, half>::value) {
                quantPre = QuantMode_t::QF322F16_PRE;
            } else if constexpr (std::is_same<DstType, bfloat16_t>::value) {
                quantPre = QuantMode_t::QF322BF16_PRE;
            }
#ifdef __CCE_AICORE__
            else if constexpr (std::is_same<DstType, float8_e4m3_t>::value) {
                quantPre = QuantMode_t::QF322FP8_PRE;
            } else if constexpr (std::is_same<DstType, float>::value) {
                quantPre = QuantMode_t::QF322F32_PRE;
            }
#endif
        } else if constexpr (std::is_same<SrcType, int32_t>::value) {
            if constexpr ((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
                quantPre = QuantMode_t::REQ8;
            } else if constexpr (std::is_same<DstType, half>::value) {
                quantPre = QuantMode_t::DEQF16;
            } else if constexpr (std::is_same<DstType, bfloat16_t>::value) {
                quantPre = QuantMode_t::QS322BF16_PRE;
            }
        }
        return quantPre;
    }

    template <typename SrcType, typename DstType>
    PTO_INTERNAL constexpr QuantMode_t GetVectorPreQuantMode()
    {
        QuantMode_t quantPre = QuantMode_t::NoQuant;
        if constexpr (std::is_same<SrcType, float>::value) {
            if constexpr ((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
                quantPre = QuantMode_t::VQF322B8_PRE;
            } else if constexpr (std::is_same<DstType, hifloat8_t>::value) {
                quantPre = QuantMode_t::VQF322HIF8_PRE;
            } else if constexpr (std::is_same<DstType, half>::value) {
                quantPre = QuantMode_t::VQF322F16_PRE;
            } else if constexpr (std::is_same<DstType, bfloat16_t>::value) {
                quantPre = QuantMode_t::VQF322BF16_PRE;
            }
#ifdef __CCE_AICORE__
            else if constexpr (std::is_same<DstType, float8_e4m3_t>::value) {
                quantPre = QuantMode_t::VQF322FP8_PRE;
            } else if constexpr (std::is_same<DstType, float>::value) {
                quantPre = QuantMode_t::VQF322F32_PRE;
            }
#endif
        } else if constexpr (std::is_same<SrcType, int32_t>::value) {
            if constexpr ((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value)) {
                quantPre = QuantMode_t::VREQ8;
            } else if constexpr (std::is_same<DstType, half>::value) {
                quantPre = QuantMode_t::VDEQF16;
            } else if constexpr (std::is_same<DstType, bfloat16_t>::value) {
                quantPre = QuantMode_t::VQS322BF16_PRE;
            }
        }
        return quantPre;
    }

template <typename DstTileData, typename SrcTileData, typename DstType, typename SrcType, bool isQuant = false>
PTO_INTERNAL void CheckTMovAccValid()
{
    static_assert((SrcTileData::Loc == TileType::Acc), "Source TileType only support Acc.");
    static_assert((!SrcTileData::isRowMajor && SrcTileData::SFractal == SLayout::RowMajor),
        "Src fractal format should be (BFractal: ColMajor, SFractal: RowMajor).");
    static_assert(((std::is_same<SrcType, float>::value) || (std::is_same<SrcType, int32_t>::value)),
        "Src data type only support float or int32_t.");
    if constexpr (isQuant) {
        if constexpr (std::is_same<SrcType, float>::value) {
            static_assert((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value) ||
                              (std::is_same<DstType, hifloat8_t>::value) || (std::is_same<DstType, half>::value) ||
                              (std::is_same<DstType, bfloat16_t>::value) || (std::is_same<DstType, float8_e4m3_t>::value) ||
                              (std::is_same<DstType, float>::value),
                "The output data type must be restricted to int8_t/uint8_t/hifloat/bfloat8_t/half/bfloat16_t/ \
                    float8_e4m3_t/float.");
        } else if constexpr (std::is_same<SrcType, int32_t>::value) {
            static_assert((std::is_same<DstType, int8_t>::value) || (std::is_same<DstType, uint8_t>::value) ||
                              (std::is_same<DstType, half>::value) || (std::is_same<DstType, bfloat16_t>::value),
                "The output data type must be restricted to int8_t/uint8_t/half/bfloat16_t.");
        }
    } else {
        if constexpr (std::is_same<SrcType, float>::value) {
            static_assert((std::is_same<DstType, half>::value) || (std::is_same<DstType, bfloat16_t>::value) ||
                              (std::is_same<DstType, float>::value),
                "The output data type must be restricted to half/bfloat16_t/float.");
        } else if constexpr (std::is_same<SrcType, int32_t>::value) {
            static_assert(
                (std::is_same<DstType, int32_t>::value), "The output data type must be restricted to int32_t.");
        }
    }
    static_assert(((DstTileData::isRowMajor && DstTileData::SFractal == SLayout::NoneBox) ||
                      (!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::NoneBox) ||
                      (!DstTileData::isRowMajor && DstTileData::SFractal == SLayout::RowMajor)),
        "Only support nz2nz, nz2nd or nz2dn.");
}
}

#endif