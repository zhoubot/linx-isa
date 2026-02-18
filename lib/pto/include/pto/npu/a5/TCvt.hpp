/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

/**
 * @file TCvt.hpp
 * @brief Type Conversion (TCVT) Implementation for NPU A5 Architecture
 * 
 * FILE ORGANIZATION (for easy navigation):
 * =======================================
 * 
 * 1. CastMode enum and helper macros (lines ~77-100)
 * 
 * 2. 1D Helper Templates (lines ~103-466)
 *    - Optimized for contiguous data without padding
 *    - castS64to32_1D_NoPostUpdate, cast32to16_1D_NoPostUpdate, cast32to32_1D_NoPostUpdate, cast32toS64_1D_NoPostUpdate
 *    - cast16to16_1D_NoPostUpdate, cast16to32_1D_NoPostUpdate, cast16to8_1D_NoPostUpdate
 *    - cast8to16_1D_NoPostUpdate, cast8to32_1D_NoPostUpdate, cast32to8_1D_NoPostUpdate, cast32toH8_1D_NoPostUpdate, cast16toH8_1D_NoPostUpdate
 * 
 * 3. 2D Helper Templates (lines ~467-855)
 *    - For data with row/column layout and potential padding
 *    - Same function set as 1D but with row iteration
 * 
 * 4. castData Overloads - 2D versions (lines ~856-1503)
 *    Organized by SOURCE type for easy lookup:
 *    - FP32 (float)        → fp16, bf16, int16, int32, int64, fp8 variants
 *    - FP16 (half)         → fp32, int32, int16, int8, uint8, h8 (hifloat8 only)
 *    - BFloat16            → fp32, int32, half
 *    - U8, I8 (8-bit int)  → half, uint16, int16, int32
 *    - I16 (16-bit int)    → uint8, half, float, uint32, int32
 *    - I32 (32-bit int)    → float, int16, uint16, int64, uint8
 *    - U32 (32-bit uint)   → uint8, uint16, int16
 *    - I64 (64-bit int)    → float, int32
 *    - FP8 variants        → float
 * 
 * 5. castData_1D_NoPostUpdate Overloads (lines ~1504-1710)
 *    - Same organization as 2D versions, optimized for contiguous data
 * 
 * 6. Main TCVT Implementation (lines ~1711-end)
 *    - implTCVT: Main template function
 *    - TCVT_IMPL: Rounding mode dispatcher
 * 
 * QUICK FIND: To find a specific conversion, search for the source type section header,
 * e.g., "Source: FP32" or "Source: I16", then look for the destination type.
 */

#ifndef TCVT_HPP
#define TCVT_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include <array>
#include "common.hpp"
#include "utils.hpp"


namespace pto {

// Rounding type definitions come from the CCE compiler intrinsics headers.
// On current toolchains they are declared in the global namespace.
using ::RoundRType;
using ::RoundAType;
using ::RoundFType;
using ::RoundCType;
using ::RoundZType;
using ::RoundOType;

/**
 * Unified enum for all type conversion modes
 * Describes the vcvt intrinsic parameter pattern used for conversion
 */
enum class CastMode {
    EXPAND,          // vcvt(..., PART_EVEN) - Type expansion only, no conversion
    ROUND,           // vcvt(..., R()) - Conversion with rounding only
    ROUND_SAT,       // vcvt(..., R(), RS_ENABLE) - Conversion with rounding and saturation
    ROUND_PART,      // vcvt(..., R(), PART_EVEN) - Conversion with rounding and part operation
    ROUND_SAT_PART,  // vcvt(..., R(), RS_ENABLE, PART_EVEN) - Rounding, saturation, and part
    SAT_PART,        // vcvt(..., RS_ENABLE, PART_EVEN) - Saturation and part (no rounding)
    SAT_ROUND        // vcvt(..., RS_ENABLE, R()) - Saturation then rounding (reversed order)
};

#define FOR_ROWS \
    for (uint16_t row = 0; row < validRows; row++) {\
        int32_t dstOffset = row * dstCols;\
        int32_t srcOffset = row * srcCols;\
        uint32_t sreg = validCols;

#define FOR_ELEMENTS(elNum) constexpr uint16_t elementsNum = (elNum);\
    uint16_t repeatTimes = CeilDivision(sreg, elementsNum);\
    for(uint16_t idx = 0; idx < repeatTimes; idx++) {


#define END_FOR_ELEMENTS srcOffset += elementsNum;dstOffset += elementsNum;}

#define END_FOR_ROWS }

//=============================================================================================
// 1D Helper Templates - For contiguous data (optimized fast path)
//=============================================================================================
// These templates handle conversions when data is laid out contiguously in memory without
// padding. They process data in a single pass without row/column iteration overhead.
//
// PERFORMANCE NOTE: 1D versions are significantly faster than 2D versions when applicable,
// as they avoid the FOR_ROWS/FOR_ELEMENTS loop overhead and process data in bulk.

/**
 * Cast 64-bit integer to 32-bit (signed/float) - 1D version
 * Handles: s64 -> s32 #sat #part, s64 -> f32 #rnd #part
 */
template <typename R, typename DST, typename SRC>
inline AICORE void castS64to32_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    vector_s64 v_input_0;
    const uint32_t ELE_CNT_B64 = ELE_CNT_B32 / 2;
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B64);
    uint32_t sReg = totalElements;

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<DST> v_output;
        uint32_t len64 = sReg * 2;
        uint32_t len_even = sReg * 2;
        MaskReg preg_b64 = CreatePredicate<float>(len64);
        MaskReg preg_b32 = CreatePredicate<float>(len_even);
        
        vlds(v_input_0, src, i * ELE_CNT_B64, NORM);
        if constexpr (std::is_same<R, void>::value) {
            vcvt(v_output, v_input_0, preg_b64, RS_ENABLE, PART_EVEN);
        } else {
            vcvt(v_output, v_input_0, preg_b64, R(), PART_EVEN);
        }
        vsts(v_output, dst, i * ELE_CNT_B64, PK_B64, preg_b32);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 32-bit to 16-bit types - 1D version
 */
template <typename R, typename DST, typename SRC>
inline AICORE void cast32to16_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B32);
    uint32_t sReg = totalElements;
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input_0;
        RegTensor<DST> v_output_even;
        MaskReg preg_b32_st = CreatePredicate<float>(sReg);

        vlds(v_input_0, src, i * ELE_CNT_B32, NORM);
        if constexpr (std::is_same<R, void>::value) {
            vcvt(v_output_even, v_input_0, preg_b32, RS_ENABLE, PART_EVEN);
        } else {
            vcvt(v_output_even, v_input_0, preg_b32, R(), RS_ENABLE, PART_EVEN);
        }
        vsts(v_output_even, dst, i * ELE_CNT_B32, PK_B32, preg_b32_st);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast between 32-bit types - 1D version
 * Handles: f32 -> s32 #rnd #sat, s32 -> f32 #rnd, f32 -> f32 #rnd (same-type rounding)
 */
template <typename R, CastMode MODE, typename DST, typename SRC>
inline AICORE void cast32to32_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B32);
    uint32_t sReg = totalElements;
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input_0;
        RegTensor<DST> v_output;
        MaskReg preg_b32_st = CreatePredicate<float>(sReg);
        
        vlds(v_input_0, src, i * ELE_CNT_B32, NORM);
        if constexpr (std::is_same<DST, SRC>::value) {
            // Same type: use vtrc (truncate/round) instead of vcvt
            vtrc(v_output, v_input_0, R(), preg_b32_st);
        } else if constexpr (MODE == CastMode::ROUND_SAT) {
            vcvt(v_output, v_input_0, preg_b32, R(), RS_ENABLE);
        } else {
            vcvt(v_output, v_input_0, preg_b32, R());
        }
        vsts(v_output, dst, i * ELE_CNT_B32, NORM_B32, preg_b32_st);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 32-bit to 64-bit signed integer - 1D version
 */
template <typename R, typename SRC>
inline AICORE void cast32toS64_1D_NoPostUpdate(__ubuf__ int64_t *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    const uint32_t ELE_CNT_B64 = ELE_CNT_B32 / 2;
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B64);
    uint32_t sReg = totalElements;
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input_0;
        vector_s64 v_output;
        uint32_t len64 = sReg * 2;
        MaskReg preg_b64 = CreatePredicate<float>(len64);
        
        vlds(v_input_0, src, i * ELE_CNT_B64, UNPK_B32);
        if constexpr (std::is_same<R, void>::value) {
            vcvt(v_output, v_input_0, preg_b32, PART_EVEN);
        } else {
            vcvt(v_output, v_input_0, preg_b32, R(), RS_ENABLE, PART_EVEN);
        }
        vsts(v_output, dst, i * ELE_CNT_B64, NORM_B32, preg_b64);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast between 16-bit types - 1D version
 */
template <typename R, CastMode MODE, typename DST, typename SRC>
inline AICORE void cast16to16_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B16);
    uint32_t sReg = totalElements;
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input_0;
        RegTensor<DST> v_output;
        MaskReg preg_b16_st = CreatePredicate<half>(sReg);
        
        vlds(v_input_0, src, i * ELE_CNT_B16, NORM);
        if constexpr (MODE == CastMode::ROUND_SAT) {
            vcvt(v_output, v_input_0, preg_b16, R(), RS_ENABLE);
        } else if constexpr (MODE == CastMode::SAT_ROUND) {
            vcvt(v_output, v_input_0, preg_b16, RS_ENABLE, R());
        } else {
            vcvt(v_output, v_input_0, preg_b16, R());
        }
        vsts(v_output, dst, i * ELE_CNT_B16, NORM_B16, preg_b16_st);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 16-bit to 32-bit types - 1D version
 */
template <typename R, CastMode MODE, typename DST, typename SRC>
inline AICORE void cast16to32_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B32);
    uint32_t sReg = totalElements;
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input_0;
        RegTensor<DST> v_output;
        MaskReg preg_b32_st = CreatePredicate<float>(sReg);
        
        vlds(v_input_0, src, i * ELE_CNT_B32, UNPK_B16);
        if constexpr (MODE == CastMode::EXPAND) {
            vcvt(v_output, v_input_0, preg_b16, PART_EVEN);
        } else if constexpr (MODE == CastMode::ROUND_SAT_PART) {
            vcvt(v_output, v_input_0, preg_b16, R(), RS_ENABLE, PART_EVEN);
        } else {
            vcvt(v_output, v_input_0, preg_b16, R(), PART_EVEN);
        }
        vsts(v_output, dst, i * ELE_CNT_B32, NORM_B32, preg_b32_st);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 16-bit to 8-bit types - 1D version
 */
template <typename R, CastMode MODE, typename DST_VEC, typename DST, typename SRC>
inline AICORE void cast16to8_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B16);
    uint32_t sReg = totalElements;
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input_0;
        DST_VEC v_output_even;
        MaskReg preg_b16_st = CreatePredicate<half>(sReg);

        vlds(v_input_0, src, i * ELE_CNT_B16, NORM);
        if constexpr (MODE == CastMode::ROUND_SAT_PART) {
            vcvt(v_output_even, v_input_0, preg_b16, R(), RS_ENABLE, PART_EVEN);
        } else {
            vcvt(v_output_even, v_input_0, preg_b16, RS_ENABLE, PART_EVEN);
        }
        vsts(v_output_even, dst, i * ELE_CNT_B16, PK_B16, preg_b16_st);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 8-bit to 16-bit types - 1D version
 */
template <typename SRC_VEC, typename DST, typename SRC>
inline AICORE void cast8to16_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B16);
    uint32_t sReg = totalElements;
    uint32_t len8 = ELE_CNT_B8;
    MaskReg preg_b8 = CreatePredicate<uint8_t>(len8);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        SRC_VEC v_input_0;
        RegTensor<DST> v_output;
        MaskReg preg_b16 = CreatePredicate<half>(sReg);

        vlds(v_input_0, src, i * ELE_CNT_B16, UNPK_B8);
        vcvt(v_output, v_input_0, preg_b8, PART_EVEN);
        vsts(v_output, dst, i * ELE_CNT_B16, NORM_B16, preg_b16);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 8-bit to 32-bit types - 1D version
 */
template <typename SRC_VEC, typename DST, typename SRC>
inline AICORE void cast8to32_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B16);
    uint32_t sReg = totalElements;
    uint32_t next_len = (sReg > ELE_CNT_B32) ? sReg - ELE_CNT_B32 : 0;
    uint32_t len8 = ELE_CNT_B8;
    MaskReg preg_b8 = CreatePredicate<uint8_t>(len8);
    MaskReg pg = pset_b8(PAT_ALL);
    SRC_VEC v_zero;
    vdup((RegTensor<uint8_t> &)v_zero, 0, pg, MODE_ZEROING);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        SRC_VEC v_input_0, v_input_1, v_input_2;
        RegTensor<DST> v_output_0, v_output_1;
        MaskReg preg_b16_cur = CreatePredicate<half>(sReg);
        MaskReg preg_b16_next = CreatePredicate<half>(next_len);
        MaskReg preg_b32, preg_b32_next;
        punpack(preg_b32, preg_b16_cur, LOWER);
        punpack(preg_b32_next, preg_b16_next, LOWER);

        vlds((RegTensor<uint8_t> &)v_input_0, (__ubuf__ uint8_t *)src, i * ELE_CNT_B16, UNPK_B8);
        vintlv((RegTensor<uint8_t> &)v_input_1, (RegTensor<uint8_t> &)v_input_2, (RegTensor<uint8_t> &)v_input_0, (RegTensor<uint8_t> &)v_zero);
        vcvt(v_output_0, v_input_1, preg_b8, PART_P0);
        vcvt(v_output_1, v_input_2, preg_b8, PART_P0);
        vsts(v_output_0, dst, ELE_CNT_B32 * (i * 2), NORM_B32, preg_b32);
        vsts(v_output_1, dst, ELE_CNT_B32 * (i * 2 + 1), NORM_B32, preg_b32_next);
    }
}

/**
 * Cast 32-bit to 8-bit types - 1D version
 * 
 * IMPLEMENTATION NOTE: Uses vselr with index vector to extract bytes from 32-bit words.
 * The conversion happens in two steps:
 *   1. vcvt: Convert 32-bit source to target type (PART_P0 extracts low byte)
 *   2. vselr: Gather bytes using index vector for proper byte packing
 */
template <typename R, CastMode MODE, typename DST_VEC, typename DST, typename SRC>
inline AICORE void cast32to8_1D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B32);
    uint32_t sReg = totalElements;
    MaskReg preg_idx = pset_b8(PAT_ALL);
    
    DST_VEC v_idx;
    vci((RegTensor<int8_t> &)v_idx, (int8_t)0, INC_ORDER);
    vmuls((RegTensor<int16_t> &)v_idx, (RegTensor<int16_t> &)v_idx, (int16_t)4, preg_idx);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        RegTensor<SRC> v_input;
        DST_VEC v_output_p0, v_output;
        uint32_t cur_len = sReg;
        MaskReg preg_b32 = CreatePredicate<float>(sReg);
        MaskReg preg_b8 = CreatePredicate<uint8_t>(cur_len);

        vlds(v_input, src, i * ELE_CNT_B32, NORM);
        
        if constexpr (MODE == CastMode::ROUND_SAT_PART) {
            vcvt(v_output_p0, v_input, preg_b32, ROUND_R, RS_ENABLE, PART_P0);
        } else {
            vcvt(v_output_p0, v_input, preg_b32, RS_ENABLE, PART_P0);
        }
        
        vselr((RegTensor<uint8_t> &)v_output, (RegTensor<uint8_t> &)v_output_p0, (RegTensor<uint8_t> &)v_idx);
        vsts((RegTensor<uint8_t> &)v_output, (__ubuf__ uint8_t *)dst, i * ELE_CNT_B32, NORM_B8, preg_b8);
        // sReg is decremented by the first CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 32-bit to hifloat8 - 1D version
 * 
 * SPECIAL HANDLING: H8 (hifloat8) requires ROUND_A (round away from zero) instead of
 * ROUND_R (round to nearest even) for correct IEEE-like behavior with 8-bit precision.
 * This is a hardware requirement specific to the hifloat8 format.
 */
template <typename R>
inline AICORE void cast32toH8_1D_NoPostUpdate(__ubuf__ hifloat8_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B32);
    uint32_t sReg = totalElements;
    MaskReg preg_idx = pset_b8(PAT_ALL);
    
    vector_hif8 v_idx;
    vci((RegTensor<int8_t> &)v_idx, (int8_t)0, INC_ORDER);
    vmuls((RegTensor<int16_t> &)v_idx, (RegTensor<int16_t> &)v_idx, (int16_t)4, preg_idx);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        vector_f32 v_input;
        vector_hif8 v_output_p0, v_output;
        uint32_t cur_len = sReg;
        MaskReg preg_b32 = CreatePredicate<float>(sReg);
        MaskReg preg_b8 = CreatePredicate<uint8_t>(cur_len);

        vlds(v_input, src, i * ELE_CNT_B32, NORM);
        vcvt(v_output_p0, v_input, preg_b32, ROUND_A, RS_ENABLE, PART_P0);
        vselr((RegTensor<uint8_t> &)v_output, (RegTensor<uint8_t> &)v_output_p0, (RegTensor<uint8_t> &)v_idx);
        vsts((RegTensor<uint8_t> &)v_output, (__ubuf__ uint8_t *)dst, i * ELE_CNT_B32, NORM_B8, preg_b8);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

/**
 * Cast 16-bit to hifloat8 - 1D version
 * Special version for H8 that uses ROUND_A instead of template parameter
 */
template <typename R>
inline AICORE void cast16toH8_1D_NoPostUpdate(__ubuf__ hifloat8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    uint32_t totalElements = validRows * validCols;
    uint16_t repeatTimes = CeilDivision(totalElements, ELE_CNT_B16);
    uint32_t sReg = totalElements;
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    for (uint16_t i = 0; i < repeatTimes; ++i) {
        vector_f16 v_input_0;
        vector_hif8 v_output_even;
        MaskReg preg_b16_st = CreatePredicate<half>(sReg);

        vlds(v_input_0, src, i * ELE_CNT_B16, NORM);
        vcvt(v_output_even, v_input_0, preg_b16, ROUND_A, RS_ENABLE, PART_EVEN);
        vsts((RegTensor<uint8_t> &)v_output_even, (__ubuf__ uint8_t *)dst, i * ELE_CNT_B16, PK_B16, preg_b16_st);
        // sReg is decremented by CreatePredicate with POST_UPDATE
    }
}

//=============================================================================================
// 2D Helper Templates - For non-contiguous data with padding
//=============================================================================================

/**
 * Cast 64-bit integer to 32-bit (signed/float) - 2D version
 * Handles: s64 -> s32 #sat #part, s64 -> f32 #rnd #part
 * Intrinsics:
 *   vcvt(output, input, preg, RS_ENABLE, PART_EVEN)  // s64 -> s32 with saturation
 *   vcvt(output, input, preg, R(), PART_EVEN)        // s64 -> f32 with rounding
 */
template <typename R, typename DST, typename SRC>
inline AICORE void castS64to32(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    vector_s64 v_input_0;

    const uint32_t ELE_CNT_B64 = ELE_CNT_B32 / 2;

    FOR_ROWS
        uint32_t len64 = sreg * 2; // As we operate with 64bit blocks using 32bit operations
        MaskReg preg_b64 = CreatePredicate<float>(len64);

        FOR_ELEMENTS(ELE_CNT_B64)
            RegTensor<DST> v_output;
            uint32_t len_even = sreg * 2; // As only the even part is taken
            MaskReg preg_b32 = CreatePredicate<float>(len_even);
            
            vlds(v_input_0, src, srcOffset, NORM);
            if constexpr (std::is_same<R, void>::value) {
                vcvt(v_output, v_input_0, preg_b64, RS_ENABLE, PART_EVEN);
            } else {
                vcvt(v_output, v_input_0, preg_b64, R(), PART_EVEN);
            }
            vsts(v_output, dst, dstOffset, PK_B64, preg_b32);
       END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 32-bit to 16-bit types
 * Handles: f32 -> f16 #rnd #sat #part, f32 -> bf16 #rnd #sat #part, f32 -> s16 #rnd #sat #part
 * Intrinsics:
 *   vcvt(out_odd, in_1, preg, RS_ENABLE, PART_ODD/EVEN)       // No rounding mode (saturation only)
 *   vcvt(out_odd, in_1, preg, R(), RS_ENABLE, PART_ODD/EVEN)  // With rounding mode
 */
template <typename R, typename DST, typename SRC>
inline AICORE void cast32to16(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);
    
    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B16)
            RegTensor<SRC> v_input_0, v_input_1;
            RegTensor<DST> v_output_odd, v_output_even, v_output;
            MaskReg preg_b16 = CreatePredicate<half>(sreg);

            vlds(v_input_0, v_input_1, src, srcOffset, DINTLV_B32);
            if constexpr (std::is_same<R, void>::value) {
                vcvt(v_output_odd, v_input_1, preg_b32, RS_ENABLE, PART_ODD);
                vcvt(v_output_even, v_input_0, preg_b32, RS_ENABLE, PART_EVEN);    
            } else {
                vcvt(v_output_odd, v_input_1, preg_b32, R(), RS_ENABLE, PART_ODD);
                vcvt(v_output_even, v_input_0, preg_b32, R(), RS_ENABLE, PART_EVEN);
            }
            vor(v_output, v_output_even, v_output_odd, preg_b16);
            vsts(v_output, dst, dstOffset, NORM_B16, preg_b16);
       END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 32-bit to 16-bit types 2D without interleave version for better fusion
 * Handles: f32 -> f16 #rnd #sat #part, f32 -> bf16 #rnd #sat #part, f32 -> s16 #rnd #sat #part
 * Intrinsics:
 *   vcvt(out_odd, in_1, preg, RS_ENABLE, PART_ODD/EVEN)       // No rounding mode (saturation only)
 *   vcvt(out_odd, in_1, preg, R(), RS_ENABLE, PART_ODD/EVEN)  // With rounding mode
 */
template <typename R, typename DST, typename SRC>
inline AICORE void cast32to16_2D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);
    
    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B32)
            RegTensor<SRC> v_input_0, v_input_1;
            RegTensor<DST> v_output_odd, v_output_even, v_output;
            MaskReg preg_b32_st = CreatePredicate<float>(sreg);

            vlds(v_input_0, src, srcOffset, NORM);
            if constexpr (std::is_same<R, void>::value) {
                vcvt(v_output_even, v_input_0, preg_b32, RS_ENABLE, PART_EVEN);    
            } else {
                vcvt(v_output_even, v_input_0, preg_b32, R(), RS_ENABLE, PART_EVEN);
            }
            vsts(v_output_even, dst, dstOffset, PK_B32, preg_b32_st);
       END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast between 32-bit types (float <-> int)
 * Modes:
 *   ROUND_SAT: f32 -> s32 #rnd #sat → vcvt(output, input, preg, R(), RS_ENABLE)
 *   ROUND:     s32 -> f32 #rnd     → vcvt(output, input, preg, R())
 */
template <typename R, CastMode MODE, typename DST, typename SRC>
inline AICORE void cast32to32(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B32)
            RegTensor<SRC> v_input_0;
            RegTensor<DST> v_output;
            MaskReg preg_b32 = CreatePredicate<float>(sreg);
            
            vlds(v_input_0, src, srcOffset, NORM);
            if constexpr (MODE == CastMode::ROUND_SAT) {
                vcvt(v_output, v_input_0, preg_b32, R(), RS_ENABLE);
            } else {
                vcvt(v_output, v_input_0, preg_b32, R());
            }
            vsts(v_output, dst, dstOffset, NORM_B32, preg_b32);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 32-bit to 64-bit signed integer
 * Handles: s32 -> s64 #part, f32 -> s64 #rnd #sat #part
 * Intrinsics:
 *   vcvt(output, input, preg, PART_EVEN)                    // s32 -> s64 (type expansion)
 *   vcvt(output, input, preg, R(), RS_ENABLE, PART_EVEN)    // f32 -> s64 (with rounding and saturation)
 */
template <typename R, typename SRC>
inline AICORE void cast32toS64(__ubuf__ int64_t *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {

    const uint32_t ELE_CNT_B64 = ELE_CNT_B32 / 2;
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B64)
            RegTensor<SRC> v_input_0;
            vector_s64 v_output;
            uint32_t len64 = sreg * 2; // As we operate with 64bit blocks using 32bit operations
            MaskReg preg_b64 = CreatePredicate<float>(len64);
            
            vlds(v_input_0, src, srcOffset, UNPK_B32);
            if constexpr (std::is_same<R, void>::value) {
                vcvt(v_output, v_input_0, preg_b32, PART_EVEN);
            } else {
                vcvt(v_output, v_input_0, preg_b32, R(), RS_ENABLE, PART_EVEN);
            }
            vsts(v_output, dst, dstOffset, NORM_B32, preg_b64);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast between 16-bit types
 * Modes:
 *   ROUND_SAT:  f16 -> s16 #rnd #sat → vcvt(output, input, preg, R(), RS_ENABLE)
 *   SAT_ROUND:  bf16 -> f16 #sat #rnd → vcvt(output, input, preg, RS_ENABLE, R()) [reversed order]
 *   ROUND:      s16 -> f16 #rnd      → vcvt(output, input, preg, R())
 */
template <typename R, CastMode MODE, typename DST, typename SRC >
inline AICORE void cast16to16(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B16)
            RegTensor<SRC> v_input_0;
            RegTensor<DST> v_output;
            MaskReg preg_b16 = CreatePredicate<half>(sreg);
            
            vlds(v_input_0, src, srcOffset, NORM);
            if constexpr (MODE == CastMode::ROUND_SAT) {
                vcvt(v_output, v_input_0, preg_b16, R(), RS_ENABLE);
            } else if constexpr (MODE == CastMode::SAT_ROUND) {
                vcvt(v_output, v_input_0, preg_b16, RS_ENABLE, R());
            } else {
                vcvt(v_output, v_input_0, preg_b16, R());
            }
            vsts(v_output, dst, dstOffset, NORM_B16, preg_b16);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 16-bit to 32-bit types
 * Modes:
 *   EXPAND:          Type expansion (f16/bf16/s16 -> f32/u32/s32 #part) → vcvt(output, input, preg, PART_EVEN)
 *   ROUND_PART:      f16 -> s32 #rnd #part                             → vcvt(output, input, preg, R(), PART_EVEN)
 *   ROUND_SAT_PART:  bf16 -> s32 #rnd #sat #part                       → vcvt(output, input, preg, R(), RS_ENABLE, PART_EVEN)
 */
template <typename R, CastMode MODE, typename DST, typename SRC >
inline AICORE void cast16to32(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {

    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B32)
            RegTensor<SRC> v_input_0;
            RegTensor<DST> v_output;
            MaskReg preg_b32 = CreatePredicate<float>(sreg);
            
            vlds(v_input_0, src, srcOffset, UNPK_B16);
            if constexpr (MODE == CastMode::EXPAND) {
                vcvt(v_output, v_input_0, preg_b16, PART_EVEN);
            } else if constexpr (MODE == CastMode::ROUND_SAT_PART) {
                vcvt(v_output, v_input_0, preg_b16, R(), RS_ENABLE, PART_EVEN);
            } else {
                vcvt(v_output, v_input_0, preg_b16, R(), PART_EVEN);
            }
            vsts(v_output, dst, dstOffset, NORM_B32, preg_b32);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 16-bit to 8-bit types
 * Modes:
 *   ROUND_SAT_PART: f16 -> s8/u8 #rnd #sat #part → vcvt(..., R(), RS_ENABLE, PART_*)
 *   SAT_PART:       s16 -> u8 #sat #part         → vcvt(..., RS_ENABLE, PART_*)
 */
template <typename R, CastMode MODE, typename DST_VEC, typename DST, typename SRC>
inline AICORE void cast16to8(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
   
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B8)
            RegTensor<SRC> v_input_0, v_input_1;
            DST_VEC v_output_odd, v_output_even, v_output;
            MaskReg preg_b8 = CreatePredicate<uint8_t>(sreg);

            vlds(v_input_0, v_input_1, src, srcOffset, DINTLV_B16);
            if constexpr (MODE == CastMode::ROUND_SAT_PART) {
                vcvt(v_output_odd, v_input_1, preg_b16, R(), RS_ENABLE, PART_ODD);
                vcvt(v_output_even, v_input_0, preg_b16, R(), RS_ENABLE, PART_EVEN);
            } else {
                // SAT_PART mode: s16 -> u8 without rounding
                vcvt(v_output_odd, v_input_1, preg_b16, RS_ENABLE, PART_ODD);
                vcvt(v_output_even, v_input_0, preg_b16, RS_ENABLE, PART_EVEN);
            }
            vor(v_output, v_output_even, v_output_odd, preg_b8);
            vsts(v_output, dst, dstOffset, NORM_B8, preg_b8);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 16-bit to 8-bit types 2D without interleave version for better fusion
 * Modes:
 *   ROUND_SAT_PART: f16 -> s8/u8 #rnd #sat #part → vcvt(..., R(), RS_ENABLE, PART_EVEN)
 *   SAT_PART:       s16 -> u8 #sat #part         → vcvt(..., RS_ENABLE, PART_EVEN)
 */
template <typename R, CastMode MODE, typename DST_VEC, typename DST, typename SRC>
inline AICORE void cast16to8_2D_NoPostUpdate(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
   
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B16)
            RegTensor<SRC> v_input_0;
            DST_VEC v_output_even;
            MaskReg preg_b16_st = CreatePredicate<half>(sreg);

            vlds(v_input_0, src, srcOffset, NORM);
            if constexpr (MODE == CastMode::ROUND_SAT_PART) {
                vcvt(v_output_even, v_input_0, preg_b16, R(), RS_ENABLE, PART_EVEN);
            } else {
                // SAT_PART mode: s16 -> u8 without rounding
                vcvt(v_output_even, v_input_0, preg_b16, RS_ENABLE, PART_EVEN);
            }
            vsts(v_output_even, dst, dstOffset, PK_B16, preg_b16_st);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 8-bit to 16-bit types
 * Handles: u8/s8 -> f16/u16/s16 #part (type expansion)
 * Intrinsic: vcvt(output, input, preg, PART_EVEN)
 */
template <typename SRC_VEC, typename DST, typename SRC>
inline AICORE void cast8to16(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
   
    uint32_t len8 = ELE_CNT_B8;
    MaskReg preg_b8 = CreatePredicate<uint8_t>(len8);

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B16)
            SRC_VEC v_input_0;
            RegTensor<DST> v_output;
            MaskReg preg_b16 = CreatePredicate<half>(sreg);

            vlds(v_input_0, src, srcOffset, UNPK_B8);
            vcvt(v_output, v_input_0, preg_b8, PART_EVEN);
            vsts(v_output, dst, dstOffset, NORM_B16, preg_b16);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 8-bit to 32-bit types (FP8 to FP32 type expansion)
 * Handles: e4m3/e5m2/h8 -> f32 #part
 * Intrinsic: vcvt(output, input, preg, PART_*)
 */
template <typename SRC_VEC, typename DST, typename SRC>
inline AICORE void cast8to32(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
   
    uint32_t len8 = ELE_CNT_B8;
    MaskReg preg_b8 = CreatePredicate<uint8_t>(len8);
    MaskReg pg = pset_b8(PAT_ALL);
    SRC_VEC v_zero;
    vdup((RegTensor<uint8_t> &) v_zero, 0, pg, MODE_ZEROING);  

    FOR_ROWS
        int32_t rowDstOffset = row * dstCols;
        FOR_ELEMENTS(ELE_CNT_B16)
            SRC_VEC v_input_0, v_input_1, v_input_2;
            RegTensor<DST> v_output_0, v_output_1;
            uint32_t next_len = (sreg > ELE_CNT_B32) ? sreg - ELE_CNT_B32 : 0;
            MaskReg preg_b16_cur = CreatePredicate<half>(sreg);
            MaskReg preg_b16_next = CreatePredicate<half>(next_len);
            MaskReg preg_b32;
            MaskReg preg_b32_next;
            punpack(preg_b32, preg_b16_cur, LOWER);
            punpack(preg_b32_next, preg_b16_next, LOWER);

            vlds((RegTensor<uint8_t> &) v_input_0, (__ubuf__ uint8_t *) src, srcOffset, UNPK_B8);
            vintlv((RegTensor<uint8_t> &) v_input_1, (RegTensor<uint8_t> &) v_input_2, (RegTensor<uint8_t> &) v_input_0, (RegTensor<uint8_t> &) v_zero); // interleave with zero
            vcvt(v_output_0, v_input_1, preg_b8, PART_P0);
            vcvt(v_output_1, v_input_2, preg_b8, PART_P0);
            vsts(v_output_0, dst, rowDstOffset + ELE_CNT_B32 * (idx * 2), NORM_B32, preg_b32);
            vsts(v_output_1, dst, rowDstOffset + ELE_CNT_B32 * (idx * 2 + 1), NORM_B32, preg_b32_next);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * Cast 32-bit to 8-bit types (both floating point and integer)
 * Handles: 
 *   - f32 -> e4m3/e5m2/h8 #rnd #sat #part (ROUND_SAT_PART mode)
 *   - u32/s32 -> u8/s8 #sat #part (SAT_PART mode)
 * Intrinsics:
 *   vcvt(..., R(), RS_ENABLE, PART_P0) for floating point with rounding
 *   vcvt(..., RS_ENABLE, PART_P0) for integer without rounding
 */
template <typename R, CastMode MODE, typename DST_VEC, typename DST, typename SRC>
inline AICORE void cast32to8(__ubuf__ DST *dst, __ubuf__ SRC *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {

    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);
    MaskReg preg_idx = pset_b8(PAT_ALL);
    
    // Create index vector for vselr (selecting every 4th byte)
    DST_VEC v_idx;
    vci((RegTensor<int8_t> &) v_idx, (int8_t) 0, INC_ORDER);
    vmuls((RegTensor<int16_t> &) v_idx, (RegTensor<int16_t> &) v_idx, (int16_t) 4, preg_idx);

    FOR_ROWS
        uint32_t preg_len_tail = (sreg % ELE_CNT_B32 == 0) ? ELE_CNT_B32 : (sreg % ELE_CNT_B32);

        FOR_ELEMENTS(ELE_CNT_B32)
            RegTensor<SRC> v_input;
            DST_VEC v_output_p0, v_output;
            uint32_t preg_len = (idx == repeatTimes - 1) ? preg_len_tail : ELE_CNT_B32;
            MaskReg preg_b8 = CreatePredicate<uint8_t>(preg_len);

            vlds(v_input, src, srcOffset, NORM);
            
            // Convert with or without rounding based on mode
            if constexpr (MODE == CastMode::ROUND_SAT_PART) {
                vcvt(v_output_p0, v_input, preg_b32, ROUND_R, RS_ENABLE, PART_P0);
            } else {
                vcvt(v_output_p0, v_input, preg_b32, RS_ENABLE, PART_P0);
            }
            
            // Select every 4th byte to compact the result
            vselr((RegTensor<uint8_t> &) v_output, (RegTensor<uint8_t> &) v_output_p0, (RegTensor<uint8_t> &) v_idx);
            vsts((RegTensor<uint8_t> &) v_output, (__ubuf__ uint8_t *) dst, dstOffset, NORM_B8, preg_b8);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

//=============================================================================================
// castData Overloads (2D - with row/column iteration for non-contiguous data)
//=============================================================================================
// These are the main conversion functions organized by source type for easy navigation.
// Each source type section contains conversions to all supported destination types.
// 
// ORGANIZATION: Grouped by source type in ascending bit-width order (8→16→32→64-bit)
// WHY: This ordering provides quick lookup - if you know the source type, you can
// jump directly to its section and find all target conversions in one place.

//---------------------------------------------------------------------------------------------
// Source: FP32 (float) - 2D versions
//---------------------------------------------------------------------------------------------

/**
 * FP32 to FP32 - Applies rounding mode without type conversion
 * Intrinsic: vtrc(output, input, R(), preg)
 * 
 * NOTE: Same-type conversions like FP32→FP32 are useful for applying rounding modes
 * to existing data without changing the underlying type (e.g., rounding to nearest even).
 */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B32)
            vector_f32 v_input_0, v_output;
            MaskReg preg_b32 = CreatePredicate<float>(sreg);
            
            vlds(v_input_0, src, srcOffset, NORM);
            vtrc(v_output, v_input_0, R(), preg_b32);
            vsts(v_output, dst, dstOffset, NORM_B32, preg_b32);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B32)
            vector_f32 v_input_0, v_output;
            MaskReg preg_b32 = CreatePredicate<float>(sreg);
            
            vlds(v_input_0, src, srcOffset, NORM);
            vtrc(v_output, v_input_0, R(), preg_b32);
            vsts(v_output, dst, dstOffset, NORM_B32, preg_b32);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

/**
 * FP32 to FP16
 * Conversion: f32 -> f16 #rnd #sat #part
 * Uses cast32to16 helper
 */
template <typename R>
inline AICORE void castData(__ubuf__ float16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}


/**
 * FP32 to BF16
 * Conversion: f32 -> bf16 #rnd #sat #part
 * Uses cast32to16 helper
 */
template <typename R>
inline AICORE void castData(__ubuf__ bfloat16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ bfloat16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

/**
 * FP32 to I16
 * Conversion: f32 -> s16 #rnd #sat #part
 * Uses cast32to16 helper
 */
template <typename R>
inline AICORE void castData(__ubuf__ int16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

/**
 * FP32 to I32
 * Conversion: f32 -> s32 #rnd #sat
 * Intrinsic: vcvt(output, input, preg, R(), RS_ENABLE)
 */
template <typename R>
inline AICORE void castData(__ubuf__ int32_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32<R, CastMode::ROUND_SAT>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32<R, CastMode::ROUND_SAT>(dst, src, validRows, validCols, dstCols, srcCols);
}

/**
 * FP32 to I64
 * Conversion: f32 -> s64 #rnd #sat #part
 * Uses cast32toS64 helper
 */
template <typename R>
inline AICORE void castData(__ubuf__ int64_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toS64<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int64_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toS64<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

/**
 * FP32 to FP8_E4M3
 * Conversion: f32 -> e4m3 #rnd #sat #pp
 * Uses cast32to8 helper
 */
template <typename R>
inline AICORE void castData(__ubuf__ float8_e4m3_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<R, CastMode::ROUND_SAT_PART, vector_f8e4m3>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float8_e4m3_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<R, CastMode::ROUND_SAT_PART, vector_f8e4m3>(dst, src, validRows, validCols, dstCols, srcCols);
}

/**
 * FP32 to FP8_E5M2
 * Conversion: f32 -> e5m2 #rnd #sat #pp
 * Uses cast32to8 helper
 */
template <typename R>
inline AICORE void castData(__ubuf__ float8_e5m2_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<R, CastMode::ROUND_SAT_PART, vector_f8e5m2>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float8_e5m2_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<R, CastMode::ROUND_SAT_PART, vector_f8e5m2>(dst, src, validRows, validCols, dstCols, srcCols);
}

/**
 * FP32 to H8
 * Conversion: f32 -> h8 #rnd #sat #part
 * Note: H8 conversion requires ROUND_A mode
 */
template <typename R>
inline AICORE void castData(__ubuf__ hifloat8_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    
    uint32_t len32 = ELE_CNT_B32;
    MaskReg preg_b32 = CreatePredicate<float>(len32);
    MaskReg preg_idx = pset_b8(PAT_ALL);
    
    // Create index vector for vselr (selecting every 4th byte)
    vector_u8 v_idx;
    vci((RegTensor<int8_t> &) v_idx, (int8_t) 0, INC_ORDER);
    vmuls((RegTensor<int16_t> &) v_idx, (RegTensor<int16_t> &) v_idx, (int16_t) 4, preg_idx);
    
    FOR_ROWS
        uint32_t preg_len_tail = (sreg % ELE_CNT_B32 == 0) ? ELE_CNT_B32 : (sreg % ELE_CNT_B32);
        
        FOR_ELEMENTS(ELE_CNT_B32)
            vector_f32 v_input;
            vector_hif8 v_output_p0, v_output;
            uint32_t preg_len = (idx == repeatTimes - 1) ? preg_len_tail : ELE_CNT_B32;
            MaskReg preg_b8 = CreatePredicate<uint8_t>(preg_len);
            
            vlds(v_input, src, srcOffset, NORM);
            vcvt(v_output_p0, v_input, preg_b32, ROUND_A, RS_ENABLE, PART_P0);
            
            // Select every 4th byte to compact the result
            vselr((RegTensor<uint8_t> &) v_output, (RegTensor<uint8_t> &) v_output_p0, (RegTensor<uint8_t> &) v_idx);
            vsts((RegTensor<uint8_t> &) v_output, (__ubuf__ uint8_t *) dst, dstOffset, NORM_B8, preg_b8);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ hifloat8_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    // Same complex logic as castData - just reuse it
    castData<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: FP16 (half) - 2D versions
//---------------------------------------------------------------------------------------------

/** FP16 -> FP32 #part (type expansion) → vcvt(output, input, preg, PART_EVEN) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** FP16 -> I32 #rnd #part → vcvt(output, input, preg, R(), PART_EVEN) */
template <typename R>
inline AICORE void castData(__ubuf__ int32_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<R, CastMode::ROUND_PART>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<R, CastMode::ROUND_PART>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** FP16 -> I16 #rnd #sat → vcvt(output, input, preg, R(), RS_ENABLE) */
template <typename R>
inline AICORE void castData(__ubuf__ int16_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16<R, CastMode::ROUND_SAT>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16<R, CastMode::ROUND_SAT>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** FP16 -> I8 #rnd #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ int8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8<R, CastMode::ROUND_SAT_PART, vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8_2D_NoPostUpdate<R, CastMode::ROUND_SAT_PART, vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** FP16 -> U8 #rnd #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ uint8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8<R, CastMode::ROUND_SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8_2D_NoPostUpdate<R, CastMode::ROUND_SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}


/** FP16 -> H8 #rnd #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ hifloat8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    // FP16->H8 conversion only supports ROUND_A or ROUND_H modes
    // static_assert(std::is_same<R, RoundAType>::value || std::is_same<R, RoundCType>::value,
    //               "Fix: FP16 to HIFLOAT8 conversion only supports ROUND_A (CAST_ROUND) or ROUND_H (CAST_CEIL) rounding modes");
    uint32_t len16 = ELE_CNT_B16;
    MaskReg preg_b16 = CreatePredicate<half>(len16);

    FOR_ROWS
        FOR_ELEMENTS(ELE_CNT_B8)
            vector_f16 v_input_0, v_input_1;
            vector_hif8 v_output_odd, v_output_even, v_output;
            MaskReg preg_b8 = CreatePredicate<uint8_t>(sreg);

            vlds(v_input_0, v_input_1, src, srcOffset, DINTLV_B16);
            vcvt(v_output_odd, v_input_1, preg_b16, ROUND_A, RS_ENABLE, PART_ODD);
            vcvt(v_output_even, v_input_0, preg_b16, ROUND_A, RS_ENABLE, PART_EVEN);
            vor((RegTensor<uint8_t> &) v_output, (RegTensor<uint8_t> &) v_output_even, (RegTensor<uint8_t> &) v_output_odd, preg_b8);
            vsts((RegTensor<uint8_t> &) v_output, (__ubuf__ uint8_t *) dst, dstOffset, NORM_B8, preg_b8);
        END_FOR_ELEMENTS
    END_FOR_ROWS
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ hifloat8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    // Same complex logic as castData - just reuse it
    castData<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: BFloat16 - 2D versions
//---------------------------------------------------------------------------------------------

/** BF16 -> FP32 #part (type expansion) → vcvt(output, input, preg, PART_EVEN) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** BF16 -> I32 #rnd #sat #part → vcvt(output, input, preg, R(), RS_ENABLE, PART_EVEN) */
template <typename R>
inline AICORE void castData(__ubuf__ int32_t *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<R, CastMode::ROUND_SAT_PART>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<R, CastMode::ROUND_SAT_PART>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** BF16 -> F16 #sat #rnd → vcvt(output, input, preg, RS_ENABLE, R()) [reversed order] */
template <typename R>
inline AICORE void castData(__ubuf__ half *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16<R, CastMode::SAT_ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16<R, CastMode::SAT_ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}


//---------------------------------------------------------------------------------------------
// Source: U8, I8 (8-bit integers) - 2D versions
//---------------------------------------------------------------------------------------------

/** U8 -> FP16 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ half *dst, __ubuf__ uint8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ uint8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** U8 -> U16 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ uint16_t *dst, __ubuf__ uint8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint16_t *dst, __ubuf__ uint8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I8 -> FP16 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ half *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I8 -> I16 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ int16_t *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I8 -> I32 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ int32_t *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: I16 (signed 16-bit integer) - 2D versions
//---------------------------------------------------------------------------------------------

/** I16 -> U8 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I16 -> FP16 #rnd → vcvt(output, input, preg, R()) */
template <typename R>
inline AICORE void castData(__ubuf__ half *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I16 -> FP32 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I16 -> U32 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ uint32_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint32_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I16 -> I32 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ int32_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: I32 (signed 32-bit integer) - 2D versions
//---------------------------------------------------------------------------------------------

/** I32 -> FP32 #rnd → vcvt(output, input, preg, R()) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I32 -> I16 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ int16_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I32 -> U16 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ uint16_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint16_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I32 -> I64 #part (type expansion) */
template <typename R>
inline AICORE void castData(__ubuf__ int64_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toS64<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int64_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toS64<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I32 -> U8 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ uint8_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: U32 (unsigned 32-bit integer) - 2D versions
//---------------------------------------------------------------------------------------------

/** U32 -> U8 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ uint8_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** U32 -> U16 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ uint16_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ uint16_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** U32 -> I16 #sat #part */
template <typename R>
inline AICORE void castData(__ubuf__ int16_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_2D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: I64 (signed 64-bit integer) - 2D versions
//---------------------------------------------------------------------------------------------

/** I64 -> FP32 #rnd #part → vcvt(output, input, preg, R(), PART_EVEN) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ int64_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    castS64to32<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ int64_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    castS64to32<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** I64 -> I32 #sat #part → vcvt(output, input, preg, RS_ENABLE, PART_EVEN) */
template <typename R>
inline AICORE void castData(__ubuf__ int32_t *dst, __ubuf__ int64_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    castS64to32<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ int64_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    castS64to32<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: FP8 variants (float8_e4m3_t, float8_e5m2_t, hifloat8_t) - 2D versions
//---------------------------------------------------------------------------------------------
// FP8 formats are specialized 8-bit floating-point types with different exponent/mantissa splits:
//   - E4M3: 4 exponent bits, 3 mantissa bits (higher precision, smaller range)
//   - E5M2: 5 exponent bits, 2 mantissa bits (lower precision, larger range)
//   - HIF8: Hardware-specific 8-bit float format

/** E4M3 -> FP32 #part (type expansion) → vcvt(output, input, preg, PART_P0) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ float8_e4m3_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_f8e4m3>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ float8_e4m3_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_f8e4m3>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** E5M2 -> FP32 #part (type expansion) → vcvt(output, input, preg, PART_P0) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ float8_e5m2_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_f8e5m2>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ float8_e5m2_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_f8e5m2>(dst, src, validRows, validCols, dstCols, srcCols);
}

/** H8 -> FP32 #part (type expansion) → vcvt(output, input, preg, PART_P0) */
template <typename R>
inline AICORE void castData(__ubuf__ float *dst, __ubuf__ hifloat8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_hif8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_2D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ hifloat8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32<vector_hif8>(dst, src, validRows, validCols, dstCols, srcCols);
}

//=============================================================================================
// castData_1D_NoPostUpdate Overloads - Organized by Source Type (8→16→32→64-bit sources)
//=============================================================================================
// Optimized 1D versions for contiguous data without padding
// Each section contains conversions FROM a specific source type TO all supported destination types

//---------------------------------------------------------------------------------------------
// Source: 8-bit types (uint8_t, int8_t, float8_e4m3_t, float8_e5m2_t, hifloat8_t) - 1D versions
//---------------------------------------------------------------------------------------------

// Source: U8 (unsigned 8-bit integer)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ uint8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16_1D_NoPostUpdate<vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint16_t *dst, __ubuf__ uint8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16_1D_NoPostUpdate<vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: I8 (signed 8-bit integer)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16_1D_NoPostUpdate<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to16_1D_NoPostUpdate<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ int8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32_1D_NoPostUpdate<vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: FP8_E4M3
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ float8_e4m3_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32_1D_NoPostUpdate<vector_f8e4m3>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: FP8_E5M2
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ float8_e5m2_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32_1D_NoPostUpdate<vector_f8e5m2>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: Hifloat8
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ hifloat8_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast8to32_1D_NoPostUpdate<vector_hif8>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: 16-bit types (half/fp16, bfloat16, int16_t) - 1D versions
//---------------------------------------------------------------------------------------------
// 16-bit conversions are commonly used for mixed-precision training and inference:
//   - FP16 (half): Standard IEEE 754 half-precision (1 sign, 5 exp, 10 mantissa)
//   - BF16 (bfloat16): Brain Float16 (1 sign, 8 exp, 7 mantissa) - FP32-compatible exponent
//   - I16: Signed 16-bit integer

// Source: FP16 (half)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<R, CastMode::ROUND_PART>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16_1D_NoPostUpdate<R, CastMode::ROUND_SAT>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8_1D_NoPostUpdate<R, CastMode::ROUND_SAT_PART, vector_s8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8_1D_NoPostUpdate<R, CastMode::ROUND_SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Note: FP16 -> FP8_E5M2 and FP16 -> FP8_E4M3 conversions are NOT supported
// Only FP16 -> Hifloat8 (H8) conversion is supported

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ hifloat8_t *dst, __ubuf__ half *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16toH8_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: BFloat16
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<R, CastMode::ROUND_SAT_PART>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ bfloat16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16_1D_NoPostUpdate<R, CastMode::SAT_ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}


// Source: I16 (signed 16-bit integer)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to8_1D_NoPostUpdate<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ half *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to16_1D_NoPostUpdate<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint32_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ int16_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast16to32_1D_NoPostUpdate<void, CastMode::EXPAND>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: 32-bit types (float, int32_t, uint32_t) - 1D versions
//---------------------------------------------------------------------------------------------
// Note: Keep FP32/I32/U32 together for quick lookup of all 32-bit source conversions.

// Source: FP32 (float)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32_1D_NoPostUpdate<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ bfloat16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32_1D_NoPostUpdate<R, CastMode::ROUND_SAT>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int64_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toS64_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float8_e4m3_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8_1D_NoPostUpdate<R, CastMode::ROUND_SAT_PART, vector_f8e4m3>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float8_e5m2_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8_1D_NoPostUpdate<R, CastMode::ROUND_SAT_PART, vector_f8e5m2>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ hifloat8_t *dst, __ubuf__ float *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toH8_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: I32 (signed 32-bit integer)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to32_1D_NoPostUpdate<R, CastMode::ROUND>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint16_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int64_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32toS64_1D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ int32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8_1D_NoPostUpdate<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

// Source: U32 (unsigned 32-bit integer)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint8_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to8_1D_NoPostUpdate<void, CastMode::SAT_PART, vector_u8>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ uint16_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int16_t *dst, __ubuf__ uint32_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    cast32to16_1D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

//---------------------------------------------------------------------------------------------
// Source: 64-bit types (int64_t)
//---------------------------------------------------------------------------------------------

// Source: I64 (signed 64-bit integer)
template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ float *dst, __ubuf__ int64_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    castS64to32_1D_NoPostUpdate<R>(dst, src, validRows, validCols, dstCols, srcCols);
}

template <typename R>
inline AICORE void castData_1D_NoPostUpdate(__ubuf__ int32_t *dst, __ubuf__ int64_t *src, uint32_t validRows, uint32_t validCols, uint32_t dstCols, uint32_t srcCols) {
    castS64to32_1D_NoPostUpdate<void>(dst, src, validRows, validCols, dstCols, srcCols);
}

//=============================================================================================
// Main TCVT Implementation
//=============================================================================================

/**
 * Main TCVT implementation function
 * Converts tile data from source type to destination type using specified rounding mode
 * Iterates over rows and calls appropriate castData specialization
 */
template <typename TileDataD, typename TileDataS, typename R>
__tf__ PTO_INTERNAL OP_NAME(TCVT) OP_TYPE(element_wise)
void implTCVT(typename TileDataD::TileDType __out__ dst, 
              typename TileDataS::TileDType __in__ src, 
    unsigned validRows, unsigned validCols, VFImplKind version = VFImplKind::VFIMPL_DEFAULT)
{
    using T1 = typename TileDataD::DType;
    using T2 = typename TileDataS::DType;
    __ubuf__ T1 *dstPtr = (__ubuf__ T1 *)__cce_get_tile_ptr(dst);
    __ubuf__ T2 *srcPtr = (__ubuf__ T2 *)__cce_get_tile_ptr(src);
    __VEC_SCOPE__ {
        // Compile-time check: Use 1D optimization if:
        // 1. ValidCol == Cols (no column padding) for both src and dst, OR
        // 2. Both tiles have Rows == 1 (single row case)
        if constexpr (((TileDataD::ValidCol == TileDataD::Cols) && (TileDataS::ValidCol == TileDataS::Cols)) ||
            ((TileDataD::Rows == 1) && (TileDataS::Rows == 1))) {
            // Use 1D path: faster bulk processing without row iteration overhead
            switch (version) {
                case VFImplKind::VFIMPL_DEFAULT:
                case VFImplKind::VFIMPL_1D_NO_POST_UPDATE:
                case VFImplKind::VFIMPL_1D_POST_UPDATE:
                    castData_1D_NoPostUpdate<R>(dstPtr, srcPtr, validRows, validCols, TileDataD::Cols, TileDataS::Cols);
                    break;
                default:
                    castData_1D_NoPostUpdate<R>(dstPtr, srcPtr, validRows, validCols, TileDataD::Cols, TileDataS::Cols);
                    break;
            }

        } else {
            // Use 2D path: handles strided/padded data with row-by-row iteration
            // version parameter controls predicate update strategy:
            // VFIMPL_2D_NO_POST_UPDATE: manual predicate handling
            // default: auto predicate update
            switch (version) {
                case VFImplKind::VFIMPL_2D_NO_POST_UPDATE:
                    castData_2D_NoPostUpdate<R>(dstPtr, srcPtr, validRows, validCols, TileDataD::Cols, TileDataS::Cols);
                    break;
                default:
                    castData<R>(dstPtr, srcPtr, validRows, validCols, TileDataD::Cols, TileDataS::Cols);
                    break;
            }
        }
        
    }
}

template <typename TileDataD, typename TileDataS>
PTO_INTERNAL void TCVT_IMPL(TileDataD &dst, TileDataS &src, RoundMode mode)
{
    switch (mode) {
        case RoundMode::CAST_RINT:
            implTCVT<TileDataD,TileDataS,RoundRType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            break;
        case RoundMode::CAST_ROUND:
            implTCVT<TileDataD,TileDataS,RoundAType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            break;
        case RoundMode::CAST_FLOOR:
            implTCVT<TileDataD,TileDataS,RoundFType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            break;
        case RoundMode::CAST_CEIL:
            implTCVT<TileDataD,TileDataS,RoundCType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            break;
        case RoundMode::CAST_TRUNC:
            implTCVT<TileDataD,TileDataS,RoundZType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            break;
        case RoundMode::CAST_ODD:
            if constexpr (std::is_same<typename TileDataD::DType, half>::value && 
                std::is_same<typename TileDataS::DType, float>::value) {
                implTCVT<TileDataD,TileDataS,RoundOType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            } 
            break;
        default:
            implTCVT<TileDataD,TileDataS,RoundRType>(dst.data(), src.data(), dst.GetValidRow(), dst.GetValidCol());
            break;
    }
}

}  // namespace pto
#endif
