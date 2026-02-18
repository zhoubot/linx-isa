/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TMRGSORT_HPP
#define TMRGSORT_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>
#include <pto/common/pto_tile.hpp>
#include "pto/cpu/tile_offsets.hpp"

namespace pto {

struct MrgSortExecutedNumList {
    uint16_t mrgSortList0;
    uint16_t mrgSortList1;
    uint16_t mrgSortList2;
    uint16_t mrgSortList3;
};

constexpr size_t STRUCT_BYTES = 8;
constexpr const int LIST_NUM_1 = 1;
constexpr const int LIST_NUM_2 = 2;
constexpr const int LIST_NUM_3 = 3;
constexpr const int LIST_NUM_4 = 4;
constexpr const int LIST_INDEX_0 = 0;
constexpr const int LIST_INDEX_1 = 1;
constexpr const int LIST_INDEX_2 = 2;
constexpr const int LIST_INDEX_3 = 3;

template <typename T>
PTO_INTERNAL constexpr unsigned StructElemsForType()
{
    static_assert(STRUCT_BYTES % sizeof(T) == 0, "TMRGSORT: invalid struct size.");
    return static_cast<unsigned>(STRUCT_BYTES / sizeof(T));
}

template <typename TileData>
PTO_INTERNAL constexpr void CheckMrgSortTileConstraints()
{
    static_assert(TileData::Loc == TileType::Vec, "TMRGSORT: tile type must be Vec.");
    static_assert(TileData::Rows == 1, "TMRGSORT: tile rows must be 1.");
    static_assert(TileData::isRowMajor, "TMRGSORT: BLayout must be RowMajor.");
}

template <typename TileData, size_t N>
PTO_INTERNAL std::array<typename TileData::DType, N> ReadStruct(
    const typename TileData::TileDType tile, unsigned r, unsigned cBase)
{
    std::array<typename TileData::DType, N> out{};
    for (size_t i = 0; i < N; i++) {
        const size_t idx = GetTileElementOffset<TileData>(r, cBase + static_cast<unsigned>(i));
        out[i] = tile[idx];
    }
    return out;
}

template <typename TileData, size_t N>
PTO_INTERNAL void WriteStruct(
    typename TileData::TileDType tile, unsigned r, unsigned cBase, const std::array<typename TileData::DType, N> &v)
{
    for (size_t i = 0; i < N; i++) {
        const size_t idx = GetTileElementOffset<TileData>(r, cBase + static_cast<unsigned>(i));
        tile[idx] = v[i];
    }
}

PTO_INTERNAL bool ReachExhaused(unsigned i0, unsigned i1, unsigned i2, unsigned i3, unsigned s0Structs,
    unsigned s1Structs, unsigned s2Structs, unsigned s3Structs, unsigned listNum)
{
    if (i0 == s0Structs || i1 == s1Structs || (listNum >= LIST_NUM_3 && i2 == s2Structs) ||
        (listNum >= LIST_NUM_4 && i3 == s3Structs)) {
        return true;
    }
    return false;
}

PTO_INTERNAL void UpdateIndex(int pick, unsigned &i0, unsigned &i1, unsigned &i2, unsigned &i3)
{
    if (pick == LIST_INDEX_0) {
        i0++;
    } else if (pick == LIST_INDEX_1) {
        i1++;
    } else if (pick == LIST_INDEX_2) {
        i2++;
    } else {
        i3++;
    }
}

PTO_INTERNAL void WriteExhaused(unsigned i0, unsigned i1, unsigned i2, unsigned i3, uint16_t &mrgSortList0,
    uint16_t &mrgSortList1, uint16_t &mrgSortList2, uint16_t &mrgSortList3)
{
    mrgSortList0 = static_cast<uint16_t>(i0);
    mrgSortList1 = static_cast<uint16_t>(i1);
    mrgSortList2 = static_cast<uint16_t>(i2);
    mrgSortList3 = static_cast<uint16_t>(i3);
}

template <typename Dtype, typename SrcTileData, size_t N, unsigned LIST_INDEX>
PTO_INTERNAL void CompareAndPick(
    int &pick, std::array<Dtype, N> &vPick, typename SrcTileData::TileDType src, unsigned index)
{
    const auto v = ReadStruct<SrcTileData, N>(src, 0, index * N);
    if (pick < 0 || v[0] > vPick[0]) {
        vPick = v;
        pick = LIST_INDEX;
    }
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, typename Src3TileData, bool exhausted, unsigned listNum, unsigned kElemsPerStruct>
PTO_INTERNAL void TMrgsort(typename DstTileData::TileDType dst, typename TmpTileData::TileDType tmp,
    typename Src0TileData::TileDType src0, typename Src1TileData::TileDType src1, typename Src2TileData::TileDType src2,
    typename Src3TileData::TileDType src3, unsigned outStructs, uint16_t &mrgSortList0, uint16_t &mrgSortList1,
    uint16_t &mrgSortList2, uint16_t &mrgSortList3, unsigned s0Structs, unsigned s1Structs, unsigned s2Structs,
    unsigned s3Structs)
{
    (void)tmp;
    using DType = typename DstTileData::DType;

    unsigned i0 = 0;
    unsigned i1 = 0;
    unsigned i2 = 0;
    unsigned i3 = 0;
    unsigned out = 0;

    using StructT = std::array<DType, kElemsPerStruct>;
    while (out < outStructs) {
        int pick = -1;
        StructT vPick{};

        if (i0 < s0Structs) {
            vPick = ReadStruct<Src0TileData, kElemsPerStruct>(src0, 0, i0 * kElemsPerStruct);
            pick = 0;
        }
        if (i1 < s1Structs) {
            CompareAndPick<DType, Src1TileData, kElemsPerStruct, LIST_INDEX_1>(pick, vPick, src1, i1);
        }
        if constexpr (listNum >= LIST_NUM_3) {
            if (i2 < s2Structs) {
                CompareAndPick<DType, Src2TileData, kElemsPerStruct, LIST_INDEX_2>(pick, vPick, src2, i2);
            }
        }

        if constexpr (listNum == LIST_NUM_4) {
            if (i3 < s3Structs) {
                CompareAndPick<DType, Src3TileData, kElemsPerStruct, LIST_INDEX_3>(pick, vPick, src3, i3);
            }
        }

        if (pick < 0) {
            break;
        }

        WriteStruct<DstTileData, kElemsPerStruct>(dst, 0, out * kElemsPerStruct, vPick);
        out++;

        UpdateIndex(pick, i0, i1, i2, i3);

        if constexpr (exhausted) {
            if (ReachExhaused(i0, i1, i2, i3, s0Structs, s1Structs, s2Structs, s3Structs, listNum)) {
                break;
            }
        }
    }

    WriteExhaused(i0, i1, i2, i3, mrgSortList0, mrgSortList1, mrgSortList2, mrgSortList3);
}

// blockLen includes values + indexes/payload, e.g. 32 (value,idx) pairs -> blockLen=64 for float.
template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TMrgsort(
    typename DstTileData::TileDType dst, typename SrcTileData::TileDType src, uint32_t maxCols, uint32_t blockLen)
{
    CheckMrgSortTileConstraints<DstTileData>();
    CheckMrgSortTileConstraints<SrcTileData>();

    using DType = typename DstTileData::DType;
    constexpr unsigned kElemsPerStruct = StructElemsForType<DType>();

    const unsigned blockElems = static_cast<unsigned>(blockLen);
    if (blockElems == 0 || blockElems % kElemsPerStruct != 0) {
        return;
    }

    const unsigned structsPerBlock = blockElems / kElemsPerStruct;
    constexpr unsigned kBlocksPerGroup = 4;
    const unsigned groupElems = blockElems * kBlocksPerGroup;

    using StructT = std::array<DType, kElemsPerStruct>;
    for (unsigned cBase = 0; cBase + groupElems <= maxCols; cBase += groupElems) {
        std::array<unsigned, kBlocksPerGroup> idx{};
        for (unsigned i = 0; i < kBlocksPerGroup; i++) {
            idx[i] = 0;
        }

        for (unsigned out = 0; out < structsPerBlock * kBlocksPerGroup; out++) {
            int pick = -1;
            StructT vPick{};
            for (unsigned b = 0; b < kBlocksPerGroup; b++) {
                if (idx[b] >= structsPerBlock) {
                    continue;
                }
                const unsigned srcC = cBase + b * blockElems + idx[b] * kElemsPerStruct;
                const auto v = ReadStruct<SrcTileData, kElemsPerStruct>(src, 0, srcC);
                if (pick < 0 || v[0] > vPick[0]) {
                    vPick = v;
                    pick = static_cast<int>(b);
                }
            }
            if (pick < 0) {
                break;
            }
            const unsigned dstC = cBase + out * kElemsPerStruct;
            WriteStruct<DstTileData, kElemsPerStruct>(dst, 0, dstC, vPick);
            idx[static_cast<unsigned>(pick)]++;
        }
    }
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, typename Src3TileData, bool exhausted>
PTO_INTERNAL void TMRGSORT_IMPL(DstTileData &dst, MrgSortExecutedNumList &executedNumList, TmpTileData &tmp,
    Src0TileData &src0, Src1TileData &src1, Src2TileData &src2, Src3TileData &src3)
{
    CheckMrgSortTileConstraints<DstTileData>();
    CheckMrgSortTileConstraints<TmpTileData>();
    CheckMrgSortTileConstraints<Src0TileData>();
    CheckMrgSortTileConstraints<Src1TileData>();
    CheckMrgSortTileConstraints<Src2TileData>();
    CheckMrgSortTileConstraints<Src3TileData>();
    constexpr unsigned kElemsPerStruct = StructElemsForType<typename DstTileData::DType>();
    unsigned src0Col = src0.GetValidCol() / kElemsPerStruct;
    unsigned src1Col = src1.GetValidCol() / kElemsPerStruct;
    unsigned src2Col = src2.GetValidCol() / kElemsPerStruct;
    unsigned src3Col = src3.GetValidCol() / kElemsPerStruct;
    unsigned dstCol = dst.GetValidCol() / kElemsPerStruct;

    TMrgsort<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src1TileData, Src1TileData, exhausted, LIST_NUM_4,
        kElemsPerStruct>(dst.data(), tmp.data(), src0.data(), src1.data(), src2.data(), src3.data(), dstCol,
        executedNumList.mrgSortList0, executedNumList.mrgSortList1, executedNumList.mrgSortList2,
        executedNumList.mrgSortList3, src0Col, src1Col, src2Col, src3Col);
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, bool exhausted>
PTO_INTERNAL void TMRGSORT_IMPL(DstTileData &dst, MrgSortExecutedNumList &executedNumList, TmpTileData &tmp,
    Src0TileData &src0, Src1TileData &src1, Src2TileData &src2)
{
    constexpr unsigned kElemsPerStruct = StructElemsForType<typename DstTileData::DType>();
    CheckMrgSortTileConstraints<Src0TileData>();
    CheckMrgSortTileConstraints<Src1TileData>();
    CheckMrgSortTileConstraints<Src2TileData>();
    CheckMrgSortTileConstraints<DstTileData>();
    CheckMrgSortTileConstraints<TmpTileData>();
    unsigned src0Col = src0.GetValidCol() / kElemsPerStruct;
    unsigned src1Col = src1.GetValidCol() / kElemsPerStruct;
    unsigned src2Col = src2.GetValidCol() / kElemsPerStruct;
    unsigned dstCol = dst.GetValidCol() / kElemsPerStruct;

    TMrgsort<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src1TileData, Src1TileData, exhausted, LIST_NUM_3,
        kElemsPerStruct>(dst.data(), tmp.data(), src0.data(), src1.data(), src2.data(), nullptr, dstCol,
        executedNumList.mrgSortList0, executedNumList.mrgSortList1, executedNumList.mrgSortList2,
        executedNumList.mrgSortList3, src0Col, src1Col, src2Col, 0);
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData, bool exhausted>
PTO_INTERNAL void TMRGSORT_IMPL(
    DstTileData &dst, MrgSortExecutedNumList &executedNumList, TmpTileData &tmp, Src0TileData &src0, Src1TileData &src1)
{
    CheckMrgSortTileConstraints<DstTileData>();
    CheckMrgSortTileConstraints<TmpTileData>();
    CheckMrgSortTileConstraints<Src0TileData>();
    CheckMrgSortTileConstraints<Src1TileData>();
    constexpr unsigned kElemsPerStruct = StructElemsForType<typename DstTileData::DType>();
    unsigned src0Col = src0.GetValidCol() / kElemsPerStruct;
    unsigned src1Col = src1.GetValidCol() / kElemsPerStruct;
    unsigned dstCol = dst.GetValidCol() / kElemsPerStruct;

    TMrgsort<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src1TileData, Src1TileData, exhausted, LIST_NUM_2,
        kElemsPerStruct>(dst.data(), tmp.data(), src0.data(), src1.data(), nullptr, nullptr, dstCol,
        executedNumList.mrgSortList0, executedNumList.mrgSortList1, executedNumList.mrgSortList2,
        executedNumList.mrgSortList3, src0Col, src1Col, 0, 0);
}

// The blockLen size includes values and indexes, such as 32 values and indexes: blockLen=64
template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TMRGSORT_IMPL(DstTileData &dst, SrcTileData &src, uint32_t blockLen)
{
    uint32_t dstCol = dst.GetValidCol();
    TMrgsort<DstTileData, SrcTileData>(dst.data(), src.data(), dstCol, blockLen);
}

template <typename Src0TileData, typename Src1TileData, typename Src2TileData, typename Src3TileData>
PTO_INTERNAL constexpr uint32_t GETMRGSORTTMPSIZE()
{
    return Src0TileData::Cols + Src1TileData::Cols + Src2TileData::Cols + Src3TileData::Cols;
}

template <typename Src0TileData, typename Src1TileData, typename Src2TileData>
PTO_INTERNAL constexpr uint32_t GETMRGSORTTMPSIZE()
{
    return Src0TileData::Cols + Src1TileData::Cols + Src2TileData::Cols;
}

template <typename Src0TileData, typename Src1TileData>
PTO_INTERNAL constexpr uint32_t GETMRGSORTTMPSIZE()
{
    return Src0TileData::Cols + Src1TileData::Cols;
}
} // namespace pto

#endif // TMRGSORT_HPP
