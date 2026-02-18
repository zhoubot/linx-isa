/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TPATIALBINOPS_HPP
#define TPATIALBINOPS_HPP

#include <pto/common/constants.hpp>
#include <pto/common/utils.hpp>

namespace pto {

template<typename T>
struct Padding {
    using Type = std::make_unsigned_t<T>;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0;
    static constexpr Type Max  = (Type)0xffffffffffffffffUL;
};

template<>
struct Padding<float>{
    using Type = uint32_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0xff800000UL;
    static constexpr Type Max  = (Type)0x7f800000UL;
};

template<>
struct Padding<int32_t>{
    using Type = uint32_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0x80000000UL;
    static constexpr Type Max  = (Type)0x7fffffffUL;
};

template<>
struct Padding<uint32_t>{
    using Type = uint32_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0;
    static constexpr Type Max  = (Type)0xffffffffUL;
};

template<>
struct Padding<bfloat16_t>{
    using Type = uint16_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0xff80UL;
    static constexpr Type Max  = (Type)0x7f80UL;
};

template<>
struct Padding<half>{
    using Type = uint16_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0xfc00UL;
    static constexpr Type Max  = (Type)0x7c00UL;
};

template<>
struct Padding<int16_t>{
    using Type = uint16_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0x8000UL;
    static constexpr Type Max  = (Type)0x7fffUL;
};

template<>
struct Padding<uint16_t>{
    using Type = uint16_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0;
    static constexpr Type Max  = (Type)0xffffUL;
};

template<>
struct Padding<int8_t>{
    using Type = uint8_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0x80UL;
    static constexpr Type Max  = (Type)0x7fUL;
};

template<>
struct Padding<uint8_t>{
    using Type = uint8_t;
    static constexpr Type Null = (Type)0;
    static constexpr Type Zero = (Type)0;
    static constexpr Type Min  = (Type)0;
    static constexpr Type Max  = (Type)0xffUL;
};

template <typename Op, typename T, unsigned elementsPerRepeat, unsigned dstStride>
PTO_INTERNAL void TPadOp(__ubuf__ T * dstPtr, uint64_t DstvalidRow, uint64_t DstvalidCol)
{
    constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
    RegTensor<typename Padding<T>::Type> vreg;
    MaskReg preg;
    vbr(vreg, Op::PadVal);
    uint16_t repeatTimes = CeilDivision(DstvalidCol, elementsPerRepeat);
    for (uint16_t i = 0; i < (uint16_t)DstvalidRow; ++i){
        uint32_t sreg = (uint32_t)(DstvalidCol);
        for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j){
            preg = CreatePredicate<T>(sreg);
            vsts((RegTensor<T> &)vreg, dstPtr + i * dstStride, j * elementsPerRepeat, distValue, preg);
        }
    }

    mem_bar(VST_VLD);
}

template <typename Op, typename TileData, unsigned elementsPerRepeat, unsigned src0Stride, unsigned src1Stride, unsigned dstStride>
__tf__ PTO_INTERNAL void TCopyPadOp(typename TileData::TileDType __out__ dst,
    typename TileData::TileDType __in__ src0, typename TileData::TileDType __in__ src1,
    uint64_t Src0validRow, uint64_t Src0validCol, uint64_t Src1validRow, uint64_t Src1validCol,
    uint64_t DstvalidRow, uint64_t DstvalidCol)
{
    using T = typename TileData::DType;
    __ubuf__ T * src0Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src0);
    __ubuf__ T * src1Ptr = (__ubuf__ T *)__cce_get_tile_ptr(src1);
    __ubuf__ T * dstPtr  = (__ubuf__ T *)__cce_get_tile_ptr(dst);

    #pragma no_simd_vf_fusion

    __VEC_SCOPE__
    {
        TPadOp<Op, T, elementsPerRepeat, dstStride>(dstPtr, DstvalidRow, DstvalidCol);

        MaskReg preg;
        RegTensor<T> vreg0;
        RegTensor<T> vreg1;
        RegTensor<T> vreg2;

        constexpr auto distValue = std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM>())>();
        uint16_t repeatTimes = CeilDivision(DstvalidCol, elementsPerRepeat);

        // COPY source0 into dst
        repeatTimes = CeilDivision(Src0validCol, elementsPerRepeat);
        for (uint16_t i = 0; i < (uint16_t)Src0validRow; ++i){
            uint32_t sreg = (uint32_t)(Src0validCol);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j){
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, src0Ptr + i * src0Stride, j * elementsPerRepeat, NORM);
                vsts(vreg0, dstPtr  + i * dstStride,  j * elementsPerRepeat, distValue, preg);
            } 
        }

        mem_bar(VST_VLD);

        // MAX (between the dst and source 1)
        repeatTimes = CeilDivision(Src1validCol, elementsPerRepeat);
        for (uint16_t i = 0; i < (uint16_t)Src1validRow; ++i){
            uint32_t sreg = (uint32_t)(Src1validCol);
            for (uint16_t j = 0; j < (uint16_t)repeatTimes; ++j){
                preg = CreatePredicate<T>(sreg);
                vlds(vreg0, dstPtr  + i * dstStride,  j * elementsPerRepeat, NORM);
                vlds(vreg1, src1Ptr + i * src1Stride, j * elementsPerRepeat, NORM);
                Op::BinInstr(vreg2, vreg0, vreg1, preg);
                vsts(vreg2, dstPtr  + i * dstStride, j * elementsPerRepeat, distValue, preg);
            } 
        }
    } // end __VEC_SCOPE__
}

template <typename Op, typename DstTileData, typename Src0TileData, typename Src1TileData> 
PTO_INTERNAL void TPartMasterImpl(DstTileData &dst, Src0TileData& src0, Src1TileData& src1,
    VFImplKind version)
{
    using T  = typename DstTileData::DType;
    using S0 = typename Src0TileData::DType;
    using S1 = typename Src1TileData::DType;

    static_assert(std::is_same_v<T, S0> && std::is_same_v<T, S1>,
        "Fix: TPARTMAX/MIN Input and output types should match" );

    static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>  || std::is_same_v<T, uint16_t> || 
                   std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
                   std::is_same_v<T, half>    || std::is_same_v<T, float>   || std::is_same_v<T, bfloat16_t>,
                   "Fix: TPARTMAX/MIN Invalid data type."
    );

    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);

    constexpr unsigned DstRowStride  = DstTileData::RowStride;
    constexpr unsigned Src0RowStride = Src0TileData::RowStride;
    constexpr unsigned Src1RowStride = Src1TileData::RowStride;

    unsigned src0ValidRow = src0.GetValidRow();
    unsigned src0ValidCol = src0.GetValidCol();
    unsigned src1ValidRow = src1.GetValidRow();
    unsigned src1ValidCol = src1.GetValidCol();
    unsigned dstValidRow = dst.GetValidRow();
    unsigned dstValidCol = dst.GetValidCol();

    if (src0ValidRow == 0U || src0ValidCol == 0U || src1ValidRow == 0U ||
        src1ValidCol == 0U || dstValidRow == 0U || dstValidCol == 0U) {
        return;
    }

    // dst has to be larger than or equal to both sources
    bool condDstgeSrc = (src1ValidRow <= dstValidRow && src1ValidCol <= dstValidCol) &&
                        (src0ValidRow <= dstValidRow && src0ValidCol <= dstValidCol);

    if (condDstgeSrc){             // src0 <= dst && src1 <= dst
        TCopyPadOp<Op, DstTileData, elementsPerRepeat, Src0RowStride, Src1RowStride, DstRowStride>
            (dst.data(), src0.data(), src1.data(), src0ValidRow, src0ValidCol,
             src1ValidRow, src1ValidCol, dstValidRow, dstValidCol);
    }  // other conditions not supported
}

}

#endif
