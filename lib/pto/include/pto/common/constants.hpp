/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP
#include <pto/common/type.hpp>
#include <pto/common/memory.hpp>

namespace pto {
constexpr int REPEAT_BYTE = 256;
constexpr int REPEAT_MAX = 255;
constexpr const int BLOCK_BYTE_SIZE = 32;
constexpr const uint32_t SHIFT_BLOCK_LEN = 4;
constexpr const uint32_t SHIFT_BLOCK_BYTE = 5;
constexpr const int REPEAT_STRIDE_MAX = 255;
constexpr const uint64_t BLOCK_MAX_PER_REPEAT = 8;
constexpr const uint32_t TMP_UB_SIZE = 8 * 1024;
constexpr const uint32_t TMP_UB_OFFSET = 184 * 1024;
constexpr const uint64_t MASK_LEN = 64;
constexpr const int BLOCK_LEN = 16;
constexpr const int CUBE_BLOCK_SIZE = 512;
constexpr const int C0_SIZE_BYTE = 32;
constexpr const int FRACTAL_NZ_ROW = 16;

enum VFImplKind : unsigned {
    VFIMPL_DEFAULT              = 0,    // 默认版本
    VFIMPL_1D_NO_POST_UPDATE    = 1,
    VFIMPL_2D_NO_POST_UPDATE    = 2,
    VFIMPL_1D_POST_UPDATE       = 3,
    VFIMPL_2D_POST_UPDATE       = 4,
};

enum class RoundMode : uint8_t {
    CAST_NONE = 0,
    CAST_RINT = 1,  // round to nearest, tie to even
    CAST_ROUND = 2, // round to nearest, tie away from zero
    CAST_FLOOR = 3, // round to minus infinity
    CAST_CEIL = 4,  // round to positive infinity
    CAST_TRUNC = 5, // round to zero
    CAST_ODD = 6,   // round to odd (Von Neumann rounding)
};

enum class TCopyMode : uint8_t {
    SHALLOW_COPY = 0,
    DEEP_COPY = 1,
};

enum class AccToVecMode : uint8_t {
    SingleModeVec0 = 0,
    SingleModeVec1 = 1,
    DualModeSplitM = 2,
    DualModeSplitN = 3,
};

enum class ReluPreMode : uint8_t {
    NoRelu = 0,
    NormalRelu = 1,
};

enum class AtomicType : uint8_t {
    AtomicNone = 0,
    AtomicAdd = 1,
};

enum class PadValue {
    Null,
    Zero,
    Max,
    Min,
};

enum class CompactMode {
    Null,
    Normal,
};

template <typename DType, PadValue PadVal>
struct PadValueMap {
    PTO_STATIC_ASSERT(sizeof(DType) < 0, "TLOAD: Unsupported DType for PadValue!");
};

template <PadValue PadVal>
struct PadValueMap<int64_t, PadVal> {
    static constexpr auto value = uint32_t(0);
};
template <PadValue PadVal>
struct PadValueMap<uint64_t, PadVal> {
    static constexpr auto value = uint32_t(0);
};

template <>
struct PadValueMap<float, PadValue::Null> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<float, PadValue::Zero> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<float, PadValue::Min> {
    static constexpr auto value = uint32_t(0xff800000UL);
};
template <>
struct PadValueMap<float, PadValue::Max> {
    static constexpr auto value = uint32_t(0x7f800000UL);
};

template <>
struct PadValueMap<int32_t, PadValue::Null> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<int32_t, PadValue::Zero> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<int32_t, PadValue::Min> {
    static constexpr auto value = uint32_t(0x80000000UL);
};
template <>
struct PadValueMap<int32_t, PadValue::Max> {
    static constexpr auto value = uint32_t(0x7fffffffUL);
};

template <>
struct PadValueMap<uint32_t, PadValue::Null> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<uint32_t, PadValue::Zero> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<uint32_t, PadValue::Min> {
    static constexpr auto value = uint32_t(0);
};
template <>
struct PadValueMap<uint32_t, PadValue::Max> {
    static constexpr auto value = uint32_t(0xffffffffUL);
};

#ifndef __CPU_SIM
template <>
struct PadValueMap<bfloat16_t, PadValue::Null> {
    static constexpr auto value = uint16_t(0);
};

template <>
struct PadValueMap<bfloat16_t, PadValue::Zero> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<bfloat16_t, PadValue::Min> {
    static constexpr auto value = uint16_t(0xff80);
};
template <>
struct PadValueMap<bfloat16_t, PadValue::Max> {
    static constexpr auto value = uint16_t(0x7f80);
};
#endif
template <>
struct PadValueMap<half, PadValue::Null> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<half, PadValue::Zero> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<half, PadValue::Min> {
    static constexpr auto value = uint16_t(0xfc00);
};
template <>
struct PadValueMap<half, PadValue::Max> {
    static constexpr auto value = uint16_t(0x7c00);
};

template <>
struct PadValueMap<int16_t, PadValue::Null> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<int16_t, PadValue::Zero> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<int16_t, PadValue::Min> {
    static constexpr auto value = uint16_t(0x8000);
};
template <>
struct PadValueMap<int16_t, PadValue::Max> {
    static constexpr auto value = uint16_t(0x7fff);
};

template <>
struct PadValueMap<uint16_t, PadValue::Null> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<uint16_t, PadValue::Zero> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<uint16_t, PadValue::Min> {
    static constexpr auto value = uint16_t(0);
};
template <>
struct PadValueMap<uint16_t, PadValue::Max> {
    static constexpr auto value = uint16_t(0xffff);
};

template <>
struct PadValueMap<int8_t, PadValue::Null> {
    static constexpr auto value = uint8_t(0);
};
template <>
struct PadValueMap<int8_t, PadValue::Zero> {
    static constexpr auto value = uint8_t(0);
};
template <>
struct PadValueMap<int8_t, PadValue::Min> {
    static constexpr auto value = uint8_t(0xff);
};
template <>
struct PadValueMap<int8_t, PadValue::Max> {
    static constexpr auto value = uint8_t(0x7f);
};

template <>
struct PadValueMap<uint8_t, PadValue::Null> {
    static constexpr auto value = uint8_t(0);
};
template <>
struct PadValueMap<uint8_t, PadValue::Zero> {
    static constexpr auto value = uint8_t(0);
};
template <>
struct PadValueMap<uint8_t, PadValue::Min> {
    static constexpr auto value = uint8_t(0);
};
template <>
struct PadValueMap<uint8_t, PadValue::Max> {
    static constexpr auto value = uint8_t(0xff);
};

#if defined(REGISTER_BASE)
template <PadValue PadVal>
struct PadValueMap<float4_e1m2x2_t, PadVal> {
    static constexpr auto value = uint8_t(0);
};
template <PadValue PadVal>
struct PadValueMap<float4_e2m1x2_t, PadVal> {
    static constexpr auto value = uint8_t(0);
};
#endif

template <typename TileData>
PTO_INTERNAL constexpr auto GetPadValue()
{
    using DType = typename TileData::DType;
    constexpr PadValue PadVal = TileData::PadVal;
    return PadValueMap<DType, PadVal>::value;
}

enum class TileLayoutCustom : uint8_t {
    ND,
    DN,
    NZ,
    ZN,
    ZZ,
    NONE,
};

template <typename TileData>
PTO_INTERNAL constexpr TileLayoutCustom GetTileLayoutCustom()
{
    if constexpr (TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)) {
        return TileLayoutCustom::ND;
    } else if constexpr (!TileData::isRowMajor && (TileData::SFractal == SLayout::NoneBox)) {
        return TileLayoutCustom::DN;
    } else if constexpr (!TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor) &&
                         TileData::SFractalSize == 512) {
        return TileLayoutCustom::NZ;
    } else if constexpr (TileData::isRowMajor && (TileData::SFractal == SLayout::ColMajor) &&
                         TileData::SFractalSize == 512) {
        return TileLayoutCustom::ZN;
    } else if constexpr (TileData::isRowMajor && (TileData::SFractal == SLayout::RowMajor) &&
                         TileData::SFractalSize == 512) {
        return TileLayoutCustom::ZZ;
    } else {
        return TileLayoutCustom::NONE;
    }
}
} // namespace pto
#endif
