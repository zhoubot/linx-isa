/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TGATHER_HPP
#define TGATHER_HPP

#include <pto/common/constants.hpp>

namespace pto {
PTO_INTERNAL int CEIL(int a, int b)
{
    return (a + (b - 1)) / (b);
}

template <typename DstTileData, typename Src0TileData, typename Src1TileData>
PTO_INTERNAL void CheckValid()
{
    static_assert((sizeof(typename DstTileData::DType) == 2) || (sizeof(typename DstTileData::DType) == 4),
        "Fix: TGATHER expect b16/b32");
    static_assert((sizeof(typename Src1TileData::DType) == 4), "Fix: TGATHER expect b32");
    static_assert((std::is_same<typename DstTileData::DType, typename Src0TileData::DType>::value),
        "Fix: TGATHER expect same size for indice and dst");
}

template <typename TileDataD, typename TileDataS0, typename TileDataS1>
__tf__ AICORE void TGather(typename TileDataD::TileDType __out__ dst, typename TileDataS0::TileDType __in__ src0,
    typename TileDataS1::TileDType __in__ src1, unsigned validCol, unsigned validRow)
{
    __ubuf__ typename TileDataS0::DType *src0Ptr = (__ubuf__ typename TileDataS0::DType *)__cce_get_tile_ptr(src0);
    __ubuf__ typename TileDataS1::DType *src1Ptr = (__ubuf__ typename TileDataS1::DType *)__cce_get_tile_ptr(src1);
    __ubuf__ typename TileDataD::DType *dstPtr = (__ubuf__ typename TileDataD::DType *)__cce_get_tile_ptr(dst);

    unsigned TShape0 = TileDataD::Rows;
    unsigned TShape1 = TileDataD::Cols;
    unsigned idx_stride = TileDataS1::Cols;
    constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(typename TileDataD::DType);
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(typename TileDataD::DType);
    constexpr unsigned stride = TileDataD::RowStride;
    unsigned numRepeatPerLine = validCol / elementsPerRepeat;
    unsigned numRemainPerLine = validCol % elementsPerRepeat;
    unsigned numLoop = numRepeatPerLine / REPEAT_MAX;
    unsigned remainAfterLoop = numRepeatPerLine % REPEAT_MAX;

    if constexpr (std::is_same_v<typename TileDataS0::DType, float> ||
                  std::is_same_v<typename TileDataS0::DType, int32_t> ||
                  std::is_same_v<typename TileDataS0::DType, uint32_t>) {
        // 64 element per VL
        // counter mode
        set_mask_count();
        for (int i = 0; i < validRow; i++) {
            set_vector_mask(0, validCol);
            vmuls((__ubuf__ int32_t *)(dstPtr + i * TShape1), (__ubuf__ int32_t *)(src1Ptr + i * idx_stride),
                sizeof(typename TileDataD::DType), 1, 1, 1, 8, 8);
            pipe_barrier(PIPE_V);
            vgather((__ubuf__ uint32_t *)(dstPtr + i * TShape1), (__ubuf__ uint32_t *)(dstPtr + i * TShape1),
                (uintptr_t)src0Ptr, 8, 1);
        }

        set_mask_norm();
        set_vector_mask(-1, -1);
    } else {
        // 128 element per VL
        // counter mode
        set_mask_count();
        for (int i = 0; i < validRow; i++) {
            set_vector_mask(0, validCol);
            vmuls((__ubuf__ int32_t *)(dstPtr + i * TShape1), (__ubuf__ int32_t *)(src1Ptr + i * idx_stride),
                sizeof(typename TileDataD::DType), 1, 1, 1, 8, 8);
            pipe_barrier(PIPE_V);
            vgather((__ubuf__ uint16_t *)(dstPtr + i * TShape1), (__ubuf__ uint32_t *)(dstPtr + i * TShape1),
                (uintptr_t)src0Ptr, 8, 1);
        }
        set_mask_norm();
        set_vector_mask(-1, -1);
    }
}

template <typename TileDataD, typename TileDataS0, typename TileDataS1>
PTO_INTERNAL void TGATHER_IMPL(TileDataD &dst, TileDataS0 &src0, TileDataS1 &src1)
{
    CheckValid<TileDataD, TileDataS0, TileDataS1>();

    unsigned validCol = dst.GetValidCol();
    unsigned validRow = dst.GetValidRow();

    TGather<TileDataD, TileDataS0, TileDataS1>(dst.data(), src0.data(), src1.data(), validCol, validRow);
}

template <typename DstTileData, typename SrcTileData, MaskPattern maskPattern>
__tf__ AICORE void TGather(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    unsigned validRow, unsigned validCol)
{
    using T = typename SrcTileData::DType;
    using U = std::conditional_t<sizeof(T) == sizeof(uint32_t), uint32_t, uint16_t>;

    constexpr unsigned srcRepeatStride = SrcTileData::Cols * sizeof(T) / BLOCK_BYTE_SIZE;

    __ubuf__ typename DstTileData::DType *dstPtr = (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(dst);
    __ubuf__ typename SrcTileData::DType *srcPtr = (__ubuf__ typename SrcTileData::DType *)__cce_get_tile_ptr(src);

    set_mask_count();
    set_vector_mask(0, validCol);
    vreducev2(reinterpret_cast<__ubuf__ U *>(dstPtr), reinterpret_cast<__ubuf__ U *>(srcPtr),
        reinterpret_cast<__ubuf__ U *>(srcPtr), validRow, 1, maskPattern, srcRepeatStride, 0);
    set_mask_norm();
    set_vector_mask(-1, -1);
}

template <typename DstTileData, typename SrcTileData, MaskPattern maskPattern>
PTO_INTERNAL void TGATHER_IMPL(DstTileData &dst, SrcTileData &src)
{
    using T = typename SrcTileData::DType;
    static_assert(sizeof(T) == 2 || sizeof(T) == 4, "Fix: TGATHER src element type must be 16 or 32-bit wide");
    static_assert((DstTileData::Loc == TileType::Vec) && (SrcTileData::Loc == TileType::Vec),
        "Fix: TGATHER expect vec TileType");
    static_assert((DstTileData::isRowMajor && SrcTileData::isRowMajor),
        "Fix: TGATHER expect row major");
    static_assert((sizeof(typename DstTileData::DType) == sizeof(T)),
        "Fix: TGATHER expect same type size for dst and src");
    PTO_ASSERT(dst.GetValidCol() == DstTileData::Cols, "Fix: TGATHER expect continuous memory for dst.");
    TGather<DstTileData, SrcTileData, maskPattern>(dst.data(), src.data(), src.GetValidRow(), src.GetValidCol());
}
} // namespace pto
#endif
