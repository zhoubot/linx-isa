/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TCMP_HPP
#define TCMP_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>
#include "common.hpp"
#include "utils.hpp"

namespace pto {

    const int32_t CMP_BITS_PER_INDEX = 32;

template <typename RegTensorDst, typename RegTensorSrc>
AICORE void CmpCall (RegTensorDst &dst, RegTensorSrc &src0, RegTensorSrc &src1, CmpMode cmpMode, vector_bool &preg)
{
    switch (static_cast<CmpMode>(cmpMode)) {
        case CmpMode::EQ:
            vcmp_eq(dst, src0, src1, preg);
            break;
        case CmpMode::NE:
            vcmp_ne(dst, src0, src1, preg);
            break;
        case CmpMode::LT:
            vcmp_lt(dst, src0, src1, preg);
            break;
        case CmpMode::GT:
            vcmp_gt(dst, src0, src1, preg);
            break;
        case CmpMode::GE:
            vcmp_ge(dst, src0, src1, preg);
            break;
        case CmpMode::LE:
            vcmp_le(dst, src0, src1, preg);
            break;
        default:
            vcmp_eq(dst, src0, src1, preg);
            break;
    }
}

template <typename TileDataDst, typename TileDataSrc, typename dataType0, unsigned dTypeSize>
__tf__ PTO_INTERNAL OP_NAME(TCMP) OP_TYPE(element_wise)
void TCmp(
    typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src0, 
    typename TileDataSrc::TileDType __in__ src1, 
    CmpMode mode, 
    unsigned validRow, 
    unsigned validCol,
    unsigned version = VFImplKind::VFIMPL_DEFAULT) {
    __ubuf__ typename TileDataDst::DType *dstPtr = (__ubuf__ typename TileDataDst::DType *)__cce_get_tile_ptr(dst);
    
    __VEC_SCOPE__
    {
        dataType0 vreg0;
        dataType0 vreg1;
        uint32_t sreg = (uint32_t)(validCol * validRow);
        vector_bool preg0;
        vector_bool preg1;
        uint32_t repeatElm = REPEAT_BYTE / dTypeSize;
        int32_t dstStride = repeatElm / CMP_BITS_PER_INDEX;
        uint16_t repeatTimes = CeilDivision(validCol * validRow, repeatElm);
        for (uint16_t i = 0; i < (uint16_t)(repeatTimes); ++i) {
            if(dTypeSize == 1){
                preg0 = plt_b8(sreg, POST_UPDATE);
            }else{
                preg0 = plt_b16(sreg, POST_UPDATE);
            }
            vlds(vreg0, src0, i * repeatElm, NORM);
            vlds(vreg1, src1, i * repeatElm, NORM);
            CmpCall<vector_bool, dataType0>(preg1, vreg0, vreg1, mode, preg0);
            psts(preg1, ((__ubuf__ uint32_t *)dstPtr + i * dstStride), 0, PK);
        }
    }
}


template <typename TileDataDst, typename TileDataSrc, typename dataType0>
__tf__ PTO_INTERNAL OP_NAME(TCMP) OP_TYPE(element_wise)
void TCmp_32B(
    typename TileDataDst::TileDType __out__ dst,
    typename TileDataSrc::TileDType __in__ src0, 
    typename TileDataSrc::TileDType __in__ src1, 
    CmpMode mode, 
    unsigned validRow, 
    unsigned validCol,
    unsigned version = VFImplKind::VFIMPL_DEFAULT ) {
    __ubuf__ typename TileDataDst::DType *dstPtr = (__ubuf__ typename TileDataDst::DType *)__cce_get_tile_ptr(dst);

    __VEC_SCOPE__
    {
        dataType0 vreg2;
        dataType0 vreg0;
        dataType0 vreg1;
        dataType0 vreg3;
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
            vlds(vreg1, src1, i * 2 * repeatElm, NORM);
            CmpCall<vector_bool, dataType0>(preg1, vreg0, vreg1, mode, preg0);
            preg0 = plt_b32(sreg, POST_UPDATE);
            vlds(vreg2, src0, (i * 2 + 1) * repeatElm, NORM);
            vlds(vreg3, src1, (i * 2 + 1) * repeatElm, NORM);
            CmpCall<vector_bool, dataType0>(preg2, vreg2, vreg3, mode, preg0);
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
            vlds(vreg1, src1 + offset0, 0, NORM);
            CmpCall<vector_bool, dataType0>(preg5, vreg0, vreg1, mode, preg0);
            ppack(preg6, preg5, LOWER);
            psts(preg6, ((__ubuf__ uint32_t *)dstPtr + offset2), 0, PK);
        }
    }
}

template <typename TileDataDst, typename TileDataSrc>
PTO_INTERNAL void TCMP_IMPL(TileDataDst &dst, TileDataSrc &src0, TileDataSrc &src1, CmpMode cmpMode) {

    unsigned validRow = src0.GetValidRow();
    unsigned validCol = src0.GetValidCol();
    if constexpr (sizeof(typename TileDataSrc::DType) == 4) {
        if constexpr (std::is_same<typename TileDataSrc::DType, int32_t>::value) {
            TCmp_32B<TileDataDst, TileDataSrc, vector_s32>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc::DType, float>::value) {
            TCmp_32B<TileDataDst, TileDataSrc, vector_f32>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc::DType, uint32_t>::value) {
            TCmp_32B<TileDataDst, TileDataSrc, vector_u32>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }

    } else if constexpr (sizeof(typename TileDataSrc::DType) == 2) {
        if constexpr (std::is_same<typename TileDataSrc::DType, int16_t>::value) {
            TCmp<TileDataDst, TileDataSrc, vector_s16, sizeof(uint16_t)>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc::DType, half>::value) {
            TCmp<TileDataDst, TileDataSrc, vector_f16, sizeof(uint16_t)>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc::DType, uint16_t>::value) {
            TCmp<TileDataDst, TileDataSrc, vector_u16, sizeof(uint16_t)>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
    } else if constexpr (sizeof(typename TileDataSrc::DType) == 1) {
        if constexpr (std::is_same<typename TileDataSrc::DType, int8_t>::value) {
            TCmp<TileDataDst, TileDataSrc, vector_s8, sizeof(uint8_t)>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
        if constexpr (std::is_same<typename TileDataSrc::DType, uint8_t>::value) {
            TCmp<TileDataDst, TileDataSrc, vector_u8, sizeof(uint8_t)>(dst.data(), src0.data(), src1.data(), cmpMode, validRow, validCol);
        }
    }
}

}
#endif
