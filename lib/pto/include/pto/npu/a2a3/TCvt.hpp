/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCVT_HPP
#define TCVT_HPP

#include <pto/common/constants.hpp>

namespace pto {
    // ============================================================================
    // Type Conversion Functions
    // ============================================================================
    // These functions handle specialized data type conversions with different 
    // rounding modes. Each function supports multiple rounding modes and dispatches
    // to the appropriate vector conversion instruction (vconv_*) based on the
    // selected rounding method.
    //
    // Rounding modes: RINT (round-to-nearest-int), ROUND (arithmetic round),
    //                FLOOR, CEIL, TRUNC (truncate), ODD (odd rounding), NONE
    // ============================================================================

    // Converts float32 (fp32) to float16 (fp16) with various rounding modes
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp32ToFp16(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp32 to fp16 - Dispatch based on rounding mode
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f322f16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f322f16a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f322f16f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f322f16c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f322f16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ODD:
                vconv_f322f16o(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f322f16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Float32 to Float32 with rounding mode - Used for normalization/conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp32ToFp32(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp32 to fp32 - Apply rounding mode to float32 data
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f322f32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f322f32a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f322f32f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f322f32c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f322f32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f322f32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Float32 to signed 64-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp32ToInt64(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp32 to int64 - Convert floating point to 64-bit integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f322s64r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f322s64a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f322s64f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f322s64c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f322s64z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f322s64z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Float32 to signed 32-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp32ToInt32(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp32 to int32 - Convert floating point to 32-bit integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f322s32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f322s32a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f322s32f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f322s32c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f322s32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f322s32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Float32 to signed 16-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp32ToInt16(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp32 to int16 - Convert floating point to 16-bit integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f322s16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f322s16a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f322s16f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f322s16c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f322s16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f322s16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    } 

    // Float32 to bfloat16 conversion
    // Bfloat16 preserves the exponent range of float32 in a 16-bit format
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp32ToBf16(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp32 to bf16 - Convert floating point to bfloat16 format
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f322bf16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f322bf16a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f322bf16f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f322bf16c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f322bf16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f322bf16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    } 
        
    // Float16 (half) to signed 32-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp16ToInt32(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
            // fp16 to int32 - Convert half-precision float to 32-bit integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f162s32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f162s32a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f162s32f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f162s32c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f162s32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f162s32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Float16 (half) to signed 16-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp16ToInt16(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp16 to int16 - Convert half-precision float to 16-bit integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f162s16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f162s16a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f162s16f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f162s16c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f162s16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f162s16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }
    
    // Float16 (half) to signed 8-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp16ToInt8(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp16 to int8 - Convert half-precision float to 8-bit signed integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f162s8r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f162s8a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f162s8f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f162s8c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f162s8z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f162s8z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Float16 (half) to unsigned 8-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallFp16ToUint8(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // fp16 to uint8 - Convert half-precision float to 8-bit unsigned integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_f162u8r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_f162u8a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_f162u8f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_f162u8c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_f162u8z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_f162u8z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Bfloat16 to signed 32-bit integer conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallBf16ToInt32(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // bf16 to int32 - Convert bfloat16 to 32-bit signed integer
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_bf162s32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_bf162s32a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_bf162s32f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_bf162s32c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_bf162s32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_bf162s32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }
    
    // Signed 16-bit integer to float16 (half) conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallInt16ToFp16(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // int16 to fp16 - Convert signed integer to half-precision float
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_s162f16r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_s162f16a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_s162f16f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_s162f16c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_s162f16z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_s162f16(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }
        
    // Signed 32-bit integer to float32 conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallInt32ToFp32(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // int32 to fp32 - Convert 32-bit signed integer to single-precision float
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_s322f32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_s322f32a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_s322f32f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_s322f32c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_s322f32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_s322f32(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }
    
    // Signed 64-bit integer to float32 conversion
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallInt64ToFp32(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // int64 to fp32 - Convert 64-bit signed integer to single-precision float
        switch (static_cast<RoundMode>(mode)) {
            case RoundMode::CAST_RINT:
                vconv_s642f32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_ROUND:
                vconv_s642f32a(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_FLOOR:
                vconv_s642f32f(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_CEIL:
                vconv_s642f32c(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            case RoundMode::CAST_TRUNC:
                vconv_s642f32z(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
            default:
                vconv_s642f32r(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
                break;
        }
    }

    // Special case conversions using compile-time type checking
    // Handles conversions like: half<->fp32, bf16<->fp32, int/uint 8<->half,
    //                           int32<->int64, int32<->int16, int32->half (with deq)
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void GenCastCallSpecialCases(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        if constexpr (std::is_same<typename TileDataD::DType, float>::value &&
                             std::is_same<typename TileDataS::DType, half>::value) {  // half to fp32
            vconv_f162f32(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, float>::value &&
                           std::is_same<typename TileDataS::DType, bfloat16_t>::value) {  // bfloat16 to float
            vconv_bf162f32(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, half>::value &&
                           std::is_same<typename TileDataS::DType, uint8_t>::value) {  // uint8 to half
            vconv_u82f16(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, half>::value &&
                             std::is_same<typename TileDataS::DType, int8_t>::value) {  // int8 to half
            vconv_s82f16(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, float>::value &&
                             std::is_same<typename TileDataS::DType, int16_t>::value) {  // int16 to float32
            vconv_s162f32(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);        
        } else if constexpr (std::is_same<typename TileDataD::DType, int32_t>::value &&
                             std::is_same<typename TileDataS::DType, int64_t>::value) {  // int64 to int32
            vconv_s642s32(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int64_t>::value &&
                             std::is_same<typename TileDataS::DType, int32_t>::value) {  // int32 to int64
            vconv_s322s64(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int16_t>::value &&
                             std::is_same<typename TileDataS::DType, int32_t>::value) {  // int32 to int16
            vconv_s322s16(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, half>::value &&
                             std::is_same<typename TileDataS::DType, int32_t>::value) {  // int32 to half
            set_deqscale(static_cast<half>(1.0));
            pipe_barrier(PIPE_V);
            vconv_deq(dst, src, repeatNum, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        }
    }

    // ============================================================================
    // Type Conversion Dispatcher
    // ============================================================================
    // GenCastCall dispatches to specific conversion functions based on source/dest
    // data types using compile-time type checking (constexpr if).
    template <typename TileDataD, typename TileDataS>
    AICORE void GenCastCall(__ubuf__ typename TileDataD::DType *dst, __ubuf__ typename TileDataS::DType *src,
        uint8_t repeatNum, RoundMode mode, uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride,
        uint16_t srcRepeatStride) {
        // Dispatch to appropriate function based on compile-time type analysis
        if constexpr (std::is_same<typename TileDataD::DType, half>::value &&
                      std::is_same<typename TileDataS::DType, float>::value) {
            GenCastCallFp32ToFp16<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, float>::value &&
                             std::is_same<typename TileDataS::DType, float>::value) {  // fp32 to fp32
            GenCastCallFp32ToFp32<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int64_t>::value &&
                             std::is_same<typename TileDataS::DType, float>::value) {  // fp32 to int64
            GenCastCallFp32ToInt64<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int32_t>::value &&
                             std::is_same<typename TileDataS::DType, float>::value) {  // fp32 to int32
            GenCastCallFp32ToInt32<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int16_t>::value &&
                             std::is_same<typename TileDataS::DType, float>::value) {  // fp32 to int16
            GenCastCallFp32ToInt16<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, bfloat16_t>::value &&
                             std::is_same<typename TileDataS::DType, float>::value) {  // fp32 to bf16
            GenCastCallFp32ToBf16<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int32_t>::value &&
                             std::is_same<typename TileDataS::DType, half>::value) {  // half to int32
            GenCastCallFp16ToInt32<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int16_t>::value &&
                             std::is_same<typename TileDataS::DType, half>::value) {  // half to int16
            GenCastCallFp16ToInt16<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int8_t>::value &&
                             std::is_same<typename TileDataS::DType, half>::value) {  // half to int8
            GenCastCallFp16ToInt8<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, uint8_t>::value &&
                             std::is_same<typename TileDataS::DType, half>::value) {  // half to uint8
            GenCastCallFp16ToUint8<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, int32_t>::value &&
                             std::is_same<typename TileDataS::DType, bfloat16_t>::value) {  // bfloat16 to int32
            GenCastCallBf16ToInt32<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);        
        } else if constexpr (std::is_same<typename TileDataD::DType, half>::value &&
                             std::is_same<typename TileDataS::DType, int16_t>::value) {  // int16 to half
            GenCastCallInt16ToFp16<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, float>::value &&
                             std::is_same<typename TileDataS::DType, int32_t>::value) {  // int32 to float
            GenCastCallInt32ToFp32<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else if constexpr (std::is_same<typename TileDataD::DType, float>::value &&
                             std::is_same<typename TileDataS::DType, int64_t>::value) {  // int64 to float
            GenCastCallInt64ToFp32<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        } else {
            GenCastCallSpecialCases<TileDataD, TileDataS>(dst,src,repeatNum, mode, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
        }              
    }

    // ============================================================================
    // Tile Conversion Helper: Process Main Data Block
    // ============================================================================
    // TCvtHead processes the primary aligned portion of data in complete repeat units.
    // This handles data that fits evenly into repeat boundaries.
    // 
    // @param dstPtr: Destination buffer pointer
    // @param srcPtr: Source buffer pointer
    // @param mode: Rounding mode for type conversions
    // @param numRepeatPerLine: Number of complete repeats per line
    // @param validRow: Number of valid rows to process
    // @param elementsPerRepeat: Number of elements per repeat unit
    // @param dstRepeatStride: Stride between repeats in destination
    // @param srcRepeatStride: Stride between repeats in source
    template <typename TileDataD, typename TileDataS, unsigned SS, unsigned DS>
    PTO_INST void TCvtHead(__ubuf__ typename TileDataD::DType *dstPtr, __ubuf__ typename TileDataS::DType *srcPtr,
        RoundMode mode, unsigned numRepeatPerLine, unsigned validRow, unsigned elementsPerRepeat,
        unsigned dstRepeatStride, unsigned srcRepeatStride) 
    {
        unsigned numLoop = numRepeatPerLine / REPEAT_MAX;
        unsigned remainAfterLoop = numRepeatPerLine % REPEAT_MAX;
        for (uint32_t i = 0; i < validRow; i++) {
            if (numLoop > 0) {
                for (uint32_t j = 0; j < numLoop; j++) {
                    GenCastCall<TileDataD, TileDataS>(dstPtr + i * DS + j * elementsPerRepeat * REPEAT_MAX,
                        srcPtr + i * SS + j * elementsPerRepeat * REPEAT_MAX,
                        (uint8_t)REPEAT_MAX,
                        mode,
                        1,
                        1,
                        (uint16_t)dstRepeatStride,
                        (uint16_t)srcRepeatStride);
                }
            }
            if (remainAfterLoop > 0) {
                GenCastCall<TileDataD, TileDataS>(dstPtr + i * DS + numLoop * elementsPerRepeat * REPEAT_MAX,
                    srcPtr + i * SS + numLoop * elementsPerRepeat * REPEAT_MAX,
                    (uint8_t)remainAfterLoop,
                    mode,
                    1,
                    1,
                    (uint16_t)dstRepeatStride,
                    (uint16_t)srcRepeatStride);
            }   
        }
    }
   
    // ============================================================================
    // Core Tile Conversion Kernel
    // ============================================================================
    // TCvt orchestrates the complete tile conversion process by handling both:
    //   1. Aligned region: Complete repeat units processed via TCvtHead
    //   2. Remainder region: Partial repeats processed with vector masking
    //
    // Template parameters:
    //   SS: Source row stride
    //   DS: Destination row stride
    //
    // @param dst: Destination tile (output) - contains data after conversion
    // @param src: Source tile (input) - contains original data to be converted
    // @param mode: Rounding mode (RINT/ROUND/FLOOR/CEIL/TRUNC/NONE/ODD)
    // @param numRepeatPerLine: Number of complete repeats per line
    // @param numRemainPerLine: Remaining elements per line (not aligned to repeat)
    // @param validRow: Number of rows containing valid data
    // @param elementsPerRepeat: Number of elements per repeat operation
    // @param dstRepeatStride: Stride between repeats in destination buffer
    // @param srcRepeatStride: Stride between repeats in source buffer
    template <typename TileDataD, typename TileDataS, unsigned SS, unsigned DS>
    __tf__ AICORE void TCvt(typename TileDataD::TileDType __out__ dst, typename TileDataS::TileDType __in__ src,
        RoundMode mode, unsigned numRepeatPerLine, unsigned numRemainPerLine, unsigned validRow, unsigned elementsPerRepeat,
        unsigned dstRepeatStride, unsigned srcRepeatStride)
    {
        // Get tile buffer pointers and calculate elements per memory block
        __ubuf__ typename TileDataD::DType *dstPtr = (__ubuf__ typename TileDataD::DType *)__cce_get_tile_ptr(dst);
        __ubuf__ typename TileDataS::DType *srcPtr = (__ubuf__ typename TileDataS::DType *)__cce_get_tile_ptr(src);
        constexpr unsigned dstNElemPerBlock = BLOCK_BYTE_SIZE / sizeof(typename TileDataD::DType);
        constexpr unsigned srcNElemPerBlock = BLOCK_BYTE_SIZE / sizeof(typename TileDataS::DType);
        
        // Process main aligned region with complete repeat units
        if (numRepeatPerLine > 0) {
            TCvtHead<TileDataD, TileDataS, SS, DS>(dstPtr, srcPtr, mode, numRepeatPerLine, validRow, elementsPerRepeat, dstRepeatStride, srcRepeatStride);
        }
        // Advance pointers to unaligned remainder region
        dstPtr += numRepeatPerLine * elementsPerRepeat;
        srcPtr += numRepeatPerLine * elementsPerRepeat;
        
        // Process remainder region with partial repeats (requires vector masking)
        if (numRemainPerLine > 0) {
            unsigned numLoop = validRow / REPEAT_MAX;
            unsigned remainAfterLoop = validRow % REPEAT_MAX;
            SetContinuousMask(numRemainPerLine);
            if (numLoop > 0) {
                for (uint32_t j = 0; j < numLoop; j++) {
                    GenCastCall<TileDataD, TileDataS>(dstPtr + j * DS * REPEAT_MAX,
                        srcPtr + j * SS * REPEAT_MAX, (uint8_t)REPEAT_MAX, mode,
                        1, 1, (uint16_t)DS / dstNElemPerBlock, (uint16_t)SS / srcNElemPerBlock);
                }
            }
            if (remainAfterLoop > 0) {
                GenCastCall<TileDataD, TileDataS>(dstPtr + numLoop * DS * REPEAT_MAX,
                    srcPtr + numLoop * SS * REPEAT_MAX, (uint8_t)remainAfterLoop,
                    mode, 1, 1, (uint16_t)DS / dstNElemPerBlock, (uint16_t)SS / srcNElemPerBlock);
            }
            set_vector_mask(-1, -1);
        }
    }

    // ============================================================================
    // High-Level Tile Conversion Interface
    // ============================================================================
    // TCVT_IMPL is the main entry point for tile data type conversion.
    // Calculates optimal repeat configuration and delegates to TCvt kernel.
    template <typename TileDataD, typename TileDataS>
    PTO_INTERNAL void TCVT_IMPL(TileDataD &dst, TileDataS &src, RoundMode mode)
    {
        // Determine repeat width as max of source/destination element sizes
        uint64_t repeatWidth = 
            static_cast<uint64_t>(max(sizeof(typename TileDataD::DType), sizeof(typename TileDataS::DType)));
        
        // Calculate destination repeat stride
        unsigned dstRepeatStride = 
            repeatWidth == sizeof(typename TileDataD::DType)
            ? BLOCK_MAX_PER_REPEAT
            : (BLOCK_MAX_PER_REPEAT / sizeof(typename TileDataS::DType) * sizeof(typename TileDataD::DType));
        
        // Calculate source repeat stride
        unsigned srcRepeatStride = 
            repeatWidth == sizeof(typename TileDataS::DType)
            ? BLOCK_MAX_PER_REPEAT
            : (BLOCK_MAX_PER_REPEAT / sizeof(typename TileDataD::DType) * sizeof(typename TileDataS::DType));
        
        // Calculate elements per repeat and split data into aligned/remainder regions
        unsigned elementsPerRepeat = REPEAT_BYTE / repeatWidth;
        unsigned numRepeatPerLine = dst.GetValidCol() / elementsPerRepeat;    // Complete repeats
        unsigned numRemainPerLine = dst.GetValidCol() % elementsPerRepeat;    // Remainder elements
        
        // Get row strides (compile-time constants)
        constexpr unsigned SS = TileDataS::RowStride;
        constexpr unsigned DS = TileDataD::RowStride;
        unsigned validRow = dst.GetValidRow();
        
        // Invoke TCvt kernel with calculated parameters
        TCvt<TileDataD, TileDataS, SS, DS>(dst.data(),
            src.data(),
            mode,
            numRepeatPerLine,
            numRemainPerLine,
            validRow,
            elementsPerRepeat,
            dstRepeatStride,
            srcRepeatStride);
    }
}  // namespace pto
#endif
