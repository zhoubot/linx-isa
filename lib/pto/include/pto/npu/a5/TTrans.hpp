/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TTRANS_HPP
#define TTRANS_HPP

#include <pto/common/utils.hpp>
#include <pto/common/constants.hpp>
#include "common.hpp"
#include "utils.hpp"

using namespace pto;
using namespace std;

namespace pto {

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
__tf__ PTO_INTERNAL void TTransB32RowWise(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, float>) {
        uint16_t repeatTimes = CeilDivision(TileData::Cols, elementsPerRepeat);
        __VEC_SCOPE__ {
            RegTensor<uint32_t> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            constexpr auto distValue =
                std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM_B32>())>();
            for (uint16_t row = 0; row < (uint16_t)TileData::Rows; ++row) {
                uint32_t sreg = (uint32_t)TileData::Cols;
                for (uint16_t chunk = 0; chunk < repeatTimes; ++chunk) {
                    preg = CreatePredicate<T>(sreg);
                    vci((RegTensor<int32_t> &)vreg0, (int32_t)(chunk * elementsPerRepeat), INC_ORDER);
                    vmins(vreg0, vreg0, (uint32_t)(TileData::Cols - 1), preg);
                    vmuls(vreg0, vreg0, srcStride, preg);
                    vadds(vreg0, vreg0, row, preg);
                    vgather2(vreg1, srcPtr, (RegTensor<uint32_t> &)vreg0, preg);
                    vsts(vreg1, dstPtr, (row * dstStride + chunk * elementsPerRepeat), distValue, preg);
                }
            }
        }
    } else {
        static_assert(sizeof(T) == 4, "Fix: TTRANS has Invalid b32 data type.");
    }
}

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
__tf__ PTO_INTERNAL void TTransB16RowWise(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    if constexpr (std::is_same_v<T, uint16_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, half> ||
                  std::is_same_v<T, bfloat16_t>) {
        uint16_t repeatTimes = CeilDivision(TileData::Cols, elementsPerRepeat);
        __VEC_SCOPE__ {
            RegTensor<uint16_t> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            constexpr auto distValue =
                std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM_B16>())>();
            for (uint16_t row = 0; row < (uint16_t)TileData::Rows; ++row) {
                uint32_t sreg = (uint32_t)TileData::Cols;
                for (uint16_t chunk = 0; chunk < repeatTimes; ++chunk) {
                    preg = CreatePredicate<T>(sreg);
                    vci((RegTensor<int16_t> &)vreg0, (int16_t)(chunk * elementsPerRepeat), INC_ORDER);
                    vmins(vreg0, vreg0, (uint16_t)(TileData::Cols - 1), preg);
                    vmuls(vreg0, vreg0, srcStride, preg);
                    vadds(vreg0, vreg0, row, preg);
                    vgather2(vreg1, srcPtr, (RegTensor<uint16_t> &)vreg0, preg);
                    vsts(vreg1, dstPtr, (row * dstStride + chunk * elementsPerRepeat), distValue, preg);
                }
            }
        }
    } else {
        static_assert(sizeof(T) == 2, "Fix: TTRANS has invalid b16 data type.");
    }
}

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
__tf__ PTO_INTERNAL void TTransB8RowWise(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>) {
        constexpr uint32_t sregLower = elementsPerRepeat >> 1;
        uint16_t repeatTimes = CeilDivision(TileData::Cols, sregLower);
        __VEC_SCOPE__ {
            RegTensor<uint16_t> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            constexpr auto distValue =
                std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_PK_B16>())>();
            for (uint16_t row = 0; row < (uint16_t)TileData::Rows; ++row) {
                uint32_t sreg = (uint32_t)TileData::Cols;
                for (uint16_t chunk = 0; chunk < repeatTimes; ++chunk) {
                    preg = CreatePredicate<uint16_t>(sreg);
                    vci((RegTensor<int16_t> &)vreg0, (int16_t)(chunk * sregLower), INC_ORDER);
                    vmins(vreg0, vreg0, (uint16_t)(TileData::Cols - 1), preg);
                    vmuls(vreg0, vreg0, srcStride, preg);
                    vadds(vreg0, vreg0, row, preg);
                    vgather2((RegTensor<uint16_t> &)vreg1, (__ubuf__ uint8_t *)srcPtr, (RegTensor<uint16_t> &)vreg0, preg);
                    vsts(vreg1, dstPtr, (row * dstStride + chunk * sregLower), distValue, preg);
                }
            }
        }
    } else {
        static_assert(sizeof(T) == 1, "Fix: TTRANS has invalid b8 data type.");
    }
}

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
__tf__ PTO_INTERNAL void TTransB32ColWise(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, float>) {
        uint16_t repeatTimes = CeilDivision(TileData::Rows, elementsPerRepeat);
        __VEC_SCOPE__ {
            RegTensor<uint32_t> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            constexpr auto distValue =
                std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM_B32>())>();
            for (uint16_t col = 0; col < (uint16_t)TileData::Cols; ++col) {
                uint32_t sreg = (uint32_t)TileData::Rows;
                for (uint16_t chunk = 0; chunk < repeatTimes; ++chunk) {
                    preg = CreatePredicate<T>(sreg);
                    vci((RegTensor<int32_t> &)vreg0, (int32_t)(chunk * elementsPerRepeat), INC_ORDER);
                    vmins(vreg0, vreg0, (uint32_t)(TileData::Rows - 1), preg);
                    vmuls(vreg0, vreg0, srcStride, preg);
                    vadds(vreg0, vreg0, col, preg);
                    vgather2(vreg1, srcPtr, (RegTensor<uint32_t> &)vreg0, preg);
                    vsts(vreg1, dstPtr, (col * dstStride + chunk * elementsPerRepeat), distValue, preg);
                }
            }
        }
    } else {
        static_assert(sizeof(T) == 4, "Fix: TTRANS has Invalid b32 data type.");
    }
}

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
__tf__ PTO_INTERNAL void TTransB16ColWise(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    if constexpr (std::is_same_v<T, uint16_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, half> ||
                  std::is_same_v<T, bfloat16_t>) {
        uint16_t repeatTimes = CeilDivision(TileData::Rows, elementsPerRepeat);
        __VEC_SCOPE__ {
            RegTensor<uint16_t> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            constexpr auto distValue =
                std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_NORM_B16>())>();
            for (uint16_t col = 0; col < (uint16_t)TileData::Cols; ++col) {
                uint32_t sreg = (uint32_t)TileData::Rows;
                for (uint16_t chunk = 0; chunk < repeatTimes; ++chunk) {
                    preg = CreatePredicate<T>(sreg);
                    vci((RegTensor<int16_t> &)vreg0, (int16_t)(chunk * elementsPerRepeat), INC_ORDER);
                    vmins(vreg0, vreg0, (uint16_t)(TileData::Rows - 1), preg);
                    vmuls(vreg0, vreg0, srcStride, preg);
                    vadds(vreg0, vreg0, col, preg);
                    vgather2(vreg1, srcPtr, (RegTensor<uint16_t> &)vreg0, preg);
                    vsts(vreg1, dstPtr, (col * dstStride + chunk * elementsPerRepeat), distValue, preg);
                }
            }
        }
    } else {
        static_assert(sizeof(T) == 2, "Fix: TTRANS has invalid b16 data type.");
    }
}

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstStride, unsigned srcStride>
__tf__ PTO_INTERNAL void TTransB8ColWise(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src) {
    using T = typename TileData::DType;
    __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
    __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);

    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>) {
        constexpr uint32_t sregLower = elementsPerRepeat >> 1;
        uint16_t repeatTimes = CeilDivision(TileData::Rows, sregLower);
        __VEC_SCOPE__ {
            RegTensor<uint16_t> vreg0;
            RegTensor<T> vreg1;
            MaskReg preg;
            constexpr auto distValue =
                std::integral_constant<::DistVST, static_cast<::DistVST>(GetDistVst<T, DistVST::DIST_PK_B16>())>();
            for (uint16_t col = 0; col < (uint16_t)TileData::Cols; ++col) {
                uint32_t sreg = (uint32_t)TileData::Rows;
                for (uint16_t chunk = 0; chunk < repeatTimes; ++chunk) {
                    preg = CreatePredicate<uint16_t>(sreg);
                    vci((RegTensor<int16_t> &)vreg0, (int16_t)(chunk * sregLower), INC_ORDER);
                    vmins(vreg0, vreg0, (uint16_t)(TileData::Rows - 1), preg);
                    vmuls(vreg0, vreg0, srcStride, preg);
                    vadds(vreg0, vreg0, col, preg);
                    vgather2((RegTensor<uint16_t> &)vreg1, (__ubuf__ uint8_t *)srcPtr, (RegTensor<uint16_t> &)vreg0, preg);
                    vsts(vreg1, dstPtr, (col * dstStride + chunk * sregLower), distValue, preg);
                }
            }
        }
    } else {
        static_assert(sizeof(T) == 1, "Fix: TTRANS has invalid b8 data type.");
    }
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataTmp>
PTO_INTERNAL void TTRANS_IMPL(TileDataDst &dst, TileDataSrc &src, TileDataTmp &tmp) {
    using T = typename TileDataSrc::DType;
    using U = typename TileDataDst::DType;
    static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "Fix: TTRANS has unsupported data type.");
    static_assert(sizeof(T) == sizeof(U), "Fix: TTRANS has inconsistent input and output data types.");
    static_assert(TileDataSrc::isRowMajor, "Fix: TTRANS has not supported layout type.");

    if constexpr (TileDataSrc::isRowMajor) {
        static_assert(TileDataSrc::Cols * sizeof(T) % 32 == 0, "Fix: TTRANS has inconsistent input shape.");
        static_assert(TileDataDst::Cols * sizeof(U) % 32 == 0, "Fix: TTRANS has inconsistent output shape.");
    } else {
        static_assert(TileDataSrc::Rows * sizeof(T) % 32 == 0, "Fix: TTRANS has inconsistent input shape.");
        static_assert(TileDataDst::Rows * sizeof(U) % 32 == 0, "Fix: TTRANS has inconsistent output shape.");
    }

    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T); // REPEAT_BYTE = 256
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
    constexpr unsigned srcStride = TileDataSrc::RowStride;
    constexpr unsigned dstStride = TileDataDst::RowStride;
    constexpr unsigned staticRepeatTimes = (TileDataSrc::Rows + elementsPerRepeat - 1) / elementsPerRepeat;
    if constexpr (sizeof(T) == 4) {
        if constexpr (staticRepeatTimes > TileDataSrc::Cols) {
            TTransB32RowWise<TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(dst.data(), src.data());
        } else {
            TTransB32ColWise<TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(dst.data(), src.data());
        }
    } else if constexpr (sizeof(T) == 2) {
        if constexpr (staticRepeatTimes > TileDataSrc::Cols) {
            TTransB16RowWise<TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(dst.data(), src.data());
        } else {
            TTransB16ColWise<TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(dst.data(), src.data());
        }
    } else if constexpr (sizeof(T) == 1) {
        if constexpr (staticRepeatTimes > TileDataSrc::Cols) {
            TTransB8RowWise<TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(dst.data(), src.data());
        } else {
            TTransB8ColWise<TileDataSrc, elementsPerRepeat, blockSizeElem, dstStride, srcStride>(dst.data(), src.data());
        }
    } else {
        static_assert(sizeof(T) == 4 || sizeof(T) == 2 || sizeof(T) == 1, "Fix: TTRANS has invalid data type.");
    }
}
} // namespace pto

#endif // TTRANS_HPP