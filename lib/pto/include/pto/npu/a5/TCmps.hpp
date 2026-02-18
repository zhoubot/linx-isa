/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCMPS_HPP
#define TCMPS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {

constexpr const int NUM_BITS_IN_BYTE = 8;

template <typename RegTensorDst, typename RegTensorSrc, typename T>
AICORE void GenCmpCall (RegTensorDst &dst, RegTensorSrc &src0, T src1, CmpMode cmpMode, vector_bool &preg)
{
    switch (static_cast<CmpMode>(cmpMode)) {
        case CmpMode::EQ:
            vcmps_eq(dst, src0, src1, preg);
            break;
        case CmpMode::NE:
            vcmps_ne(dst, src0, src1, preg);
            break;
        case CmpMode::LT:
            vcmps_lt(dst, src0, src1, preg);
            break;
        case CmpMode::GT:
            vcmps_gt(dst, src0, src1, preg);
            break;
        case CmpMode::GE:
            vcmps_ge(dst, src0, src1, preg);
            break;
        case CmpMode::LE:
            vcmps_le(dst, src0, src1, preg);
            break;
        default:
            vcmps_eq(dst, src0, src1, preg);
            break;
    }
}

template <typename TileDataDst, typename TileDataSrc, typename T, typename dataType0>
__tf__ PTO_INTERNAL OP_NAME(TCMPS) OP_TYPE(element_wise)
void TCmps_8B(typename TileDataDst::TileDType __out__ dst,
        typename TileDataSrc::TileDType __in__ src0, T src1, 
        CmpMode mode, unsigned validRow, unsigned validCol,
        unsigned version = VFImplKind::VFIMPL_DEFAULT)
{
        __ubuf__ typename TileDataSrc::DType *srcPtr = (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src0);
        __ubuf__ typename TileDataDst::DType *dstPtr = (__ubuf__ typename TileDataDst::DType *)__cce_get_tile_ptr(dst);
        
        __VEC_SCOPE__
        {
            dataType0 vreg0;
            uint32_t sreg = (uint32_t)(validCol * validRow);
            vector_bool preg0;
            vector_bool preg1;
            uint32_t repeatElm = REPEAT_BYTE / sizeof(uint8_t);
            uint16_t repeatTimes = CeilDivision(validCol * validRow, repeatElm);
            for (uint16_t i = 0; i < (uint16_t)(repeatTimes); ++i) {
                preg0 = plt_b8(sreg, POST_UPDATE);
                vlds(vreg0, src0, i * repeatElm, NORM);
                GenCmpCall<vector_bool, dataType0, T>(preg1, vreg0, src1, mode, preg0);
                psts(preg1, ((__ubuf__ uint32_t *)dstPtr + i * 8), 0, PK);
            }
        }
}


template <typename TileDataDst, typename TileDataSrc, typename T, typename dataType0>
__tf__ PTO_INTERNAL OP_NAME(TCMPS) OP_TYPE(element_wise)
void TCmps_16B(typename TileDataDst::TileDType __out__ dst,
        typename TileDataSrc::TileDType __in__ src0, T src1, 
        CmpMode mode, unsigned validRow, unsigned validCol,
        unsigned version = VFImplKind::VFIMPL_DEFAULT)
{
        __ubuf__ typename TileDataDst::DType *dstPtr = (__ubuf__ typename TileDataDst::DType *)__cce_get_tile_ptr(dst);
        __ubuf__ typename TileDataSrc::DType *srcPtr = (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src0);
        
        __VEC_SCOPE__
        {
            dataType0 vreg0;
            uint32_t sreg = (uint32_t)(validCol * validRow);
            vector_bool preg0;
            vector_bool preg1;
            uint32_t repeatElm = REPEAT_BYTE / sizeof(uint16_t);
            uint16_t repeatTimes = CeilDivision(validCol * validRow, repeatElm);
            for (uint16_t i = 0; i < (uint16_t)(repeatTimes); ++i) {
                preg0 = plt_b16(sreg, POST_UPDATE);
                vlds(vreg0, src0, i * repeatElm, NORM);
                GenCmpCall<vector_bool, dataType0, T>(preg1, vreg0, src1, mode, preg0);
                psts(preg1, ((__ubuf__ uint32_t *)dstPtr + i * 4), 0, PK);
            }
        }
}


template <typename TileDataDst, typename TileDataSrc, typename T, typename dataType0>
__tf__ PTO_INTERNAL OP_NAME(TCMPS) OP_TYPE(element_wise)
void TCmps_32B(typename TileDataDst::TileDType __out__ dst,
        typename TileDataSrc::TileDType __in__ src0, T src1, 
        CmpMode mode, unsigned validRow, unsigned validCol, 
        unsigned version = VFImplKind::VFIMPL_DEFAULT)
{
        __ubuf__ typename TileDataDst::DType *dstPtr = (__ubuf__ typename TileDataDst::DType *)__cce_get_tile_ptr(dst);
        __ubuf__ typename TileDataSrc::DType *srcPtr = (__ubuf__ typename TileDataSrc::DType *)__cce_get_tile_ptr(src0);
        __VEC_SCOPE__
        {
            dataType0 vreg2;
            dataType0 vreg0;
            uint32_t sreg = (uint32_t)(validCol * validRow);
            vector_bool preg0;
            vector_bool preg1;
            vector_bool preg2;
            vector_bool preg3;
            vector_bool preg4;
            uint32_t repeatElm = REPEAT_BYTE / sizeof(uint32_t);
            uint16_t repeatTimes = CeilDivision(validCol * validRow, repeatElm);
            for (uint16_t i = 0; i < (uint16_t)(repeatTimes / 2); ++i) {
                preg0 = plt_b32(sreg, POST_UPDATE);
                vlds(vreg0, src0, i * 2 * repeatElm, NORM);
                GenCmpCall<vector_bool, dataType0, T>(preg1, vreg0, src1, mode, preg0);
                preg0 = plt_b32(sreg, POST_UPDATE);
                vlds(vreg2, src0, (i * 2 + 1) * repeatElm, NORM);
                GenCmpCall<vector_bool, dataType0, T>(preg2, vreg2, src1, mode, preg0);
                pdintlv_b8(preg3, preg4, preg1, preg2);
                psts(preg3, ((__ubuf__ uint32_t *)dstPtr + i * 4), 0, PK);
            }
            vector_bool preg5;
            vector_bool preg6;
            uint32_t offset0 = (validRow / 2) * 2 * repeatElm;
            uint32_t offset2 = (validRow / 2) * 4;
            for (uint16_t i = 0; i < (uint16_t)(repeatTimes % 2); ++i) {
                preg0 = plt_b32(sreg, POST_UPDATE);
                vlds(vreg0, src0 + offset0, 0, NORM);
                GenCmpCall<vector_bool, dataType0, T>(preg5, vreg0, src1, mode, preg0);
                ppack(preg6, preg5, LOWER);
                psts(preg6, ((__ubuf__ uint32_t *)dstPtr + offset2), 0, PK);
            }
        }
}


template <typename TileDataDst, typename TileDataSrc0, typename T>
PTO_INTERNAL void TCMPS_IMPL(TileDataDst &dst, TileDataSrc0 &src0, T src1, CmpMode cmpMode) {
    static_assert(std::is_same<typename TileDataSrc0::DType, int32_t>::value || 
                std::is_same<typename TileDataSrc0::DType, uint32_t>::value || std::is_same<typename TileDataSrc0::DType, float>::value ||
                std::is_same<typename TileDataSrc0::DType, int16_t>::value || std::is_same<typename TileDataSrc0::DType, uint16_t>::value ||
                std::is_same<typename TileDataSrc0::DType, half>::value || std::is_same<typename TileDataSrc0::DType, uint8_t>::value ||
                std::is_same<typename TileDataSrc0::DType, int8_t>::value,
                "TCMPS: Invalid data type.");
    static_assert(TileDataDst::isRowMajor, "TCMPS: not supported Layout type");
    static_assert(TileDataDst::Loc == TileType::Vec, "TileType of dst tile must be TileType::Vec.");
    static_assert(TileDataDst::ValidCol <= TileDataDst::Cols, "Number of valid columns for dst must not be greater than number of tile columns.");
    static_assert(TileDataDst::ValidRow <= TileDataDst::Rows, "Number of valid rows for dst must not be greater than number of tile rows.");
    static_assert(TileDataSrc0::Loc == TileType::Vec, "TileType of src tile must be TileType::Vec.");
    static_assert(TileDataSrc0::ValidCol <= TileDataSrc0::Cols, "Number of valid columns for scr must not be greater than number of tile columns.");
    static_assert(TileDataSrc0::ValidRow <= TileDataSrc0::Rows, "Number of valid rows for src must not be greater than number of tile rows.");
    PTO_ASSERT(src0.GetValidCol() == dst.GetValidCol(), "Number of columns of src and dst must be the same.");
    PTO_ASSERT(src0.GetValidRow() == dst.GetValidRow(), "Number of rows of src and dst must be the same.");
    unsigned validRow = dst.GetValidRow();
    unsigned validCol = dst.GetValidCol();
    if constexpr (sizeof(typename TileDataSrc0::DType) == 4) {
        if constexpr (std::is_same<typename TileDataSrc0::DType, int32_t>::value) {
            TCmps_32B<TileDataDst, TileDataSrc0, T, vector_s32>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc0::DType, float>::value) {
            TCmps_32B<TileDataDst, TileDataSrc0, T, vector_f32>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc0::DType, uint32_t>::value) {
            TCmps_32B<TileDataDst, TileDataSrc0, T, vector_u32>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
    } else if constexpr (sizeof(typename TileDataSrc0::DType) == 2) {
        if constexpr (std::is_same<typename TileDataSrc0::DType, int16_t>::value) {
            TCmps_16B<TileDataDst, TileDataSrc0, T, vector_s16>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc0::DType, half>::value) {
            TCmps_16B<TileDataDst, TileDataSrc0, T, vector_f16>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc0::DType, uint16_t>::value) {
            TCmps_16B<TileDataDst, TileDataSrc0, T, vector_u16>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
    } else if constexpr (sizeof(typename TileDataSrc0::DType) == 1) {
        if constexpr (std::is_same<typename TileDataSrc0::DType, int8_t>::value) {
            TCmps_8B<TileDataDst, TileDataSrc0, T, vector_s8>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc0::DType, uint8_t>::value) {
            TCmps_8B<TileDataDst, TileDataSrc0, T, vector_u8>(dst.data(), src0.data(), src1, cmpMode, validRow, validCol);
        }
    }
}
}
#endif
