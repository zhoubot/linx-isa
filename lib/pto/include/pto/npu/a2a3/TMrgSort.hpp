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

#include <pto/common/constants.hpp>

namespace pto {
constexpr const int STRUCTSIZE = 8;
constexpr const int STRUCT_SIZE_SHIFT = 3;
constexpr const int COL_SIZE = 49152; // UBSIZE / ELEMSIZE = 192 * 1024 B / 4B
constexpr const int UB_SIZE = 196608;  // 192*1024 B
constexpr const int BLOCK_NUM = 4;
constexpr const int ONE_ROW = 1;
constexpr const int EMPTY_LIST_SIZE = 0;
constexpr const int LIST_NUM_1 = 1;
constexpr const int LIST_NUM_2 = 2;
constexpr const int LIST_NUM_3 = 3;
constexpr const int LIST_NUM_4 = 4;
constexpr const int TMRGSORT_BLOCK_LEN = 64;
constexpr const int REPEAT_ONE_TIME = 1;
constexpr const int MAX_REPEAT_TIMES = 255;

struct MrgSortExecutedNumList {
    uint16_t mrgSortList0;
    uint16_t mrgSortList1;
    uint16_t mrgSortList2;
    uint16_t mrgSortList3;
};

template <bool exhausted>
PTO_INTERNAL void GetExhaustedData(
    uint16_t &mrgSortList0, uint16_t &mrgSortList1, uint16_t &mrgSortList2, uint16_t &mrgSortList3)
{
    if constexpr (exhausted) {
        PtoSetWaitFlag<PIPE_V, PIPE_S>();
        int64_t mrgSortResult = get_vms4_sr();
        // VMS4_SR[15:0], number of finished region proposals in list0
        mrgSortList0 = static_cast<uint64_t>(mrgSortResult) & 0xFFFF;
        // VMS4_SR[31:16], number of finished region proposals in list1
        mrgSortList1 = (static_cast<uint64_t>(mrgSortResult) >> 16) & 0xFFFF;
        // VMS4_SR[47:32], number of finished region proposals in list2
        mrgSortList2 = (static_cast<uint64_t>(mrgSortResult) >> 32) & 0xFFFF;
        // VMS4_SR[63:48], number of finished region proposals in list3
        mrgSortList3 = (static_cast<uint64_t>(mrgSortResult) >> 48) & 0xFFFF;
    }
}

template <bool exhausted>
PTO_INTERNAL uint64_t InitConfig()
{
    uint64_t config = uint64_t(REPEAT_ONE_TIME); // Xt[7:0]: repeat time
    if constexpr (exhausted) {
        config |= (uint64_t(0b1) << 12); // Xt[12]: 1-enable input list exhausted suspension
    } else {
        config |= (uint64_t(0b0) << 12); // Xt[12]: 0-disable input list exhausted suspension
    }
    return config;
}

template <typename DstTileData>
PTO_INTERNAL void MovUb2Ub(
    __ubuf__ typename DstTileData::DType *dstPtr, __ubuf__ typename DstTileData::DType *tmpPtr, unsigned dstCol)
{
    pipe_barrier(PIPE_V);
    unsigned lenBurst = (dstCol * sizeof(typename DstTileData::DType) + BLOCK_BYTE_SIZE - 1) / BLOCK_BYTE_SIZE;
    copy_ubuf_to_ubuf((__ubuf__ void *)dstPtr, (__ubuf__ void *)tmpPtr, 0, 1, lenBurst, 0, 0);
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, typename Src3TileData, bool exhausted, unsigned listNum>
__tf__ AICORE void TMrgsort(typename DstTileData::TileDType __out__ dst,
    typename TmpTileData::TileDType __out__ tmp, typename Src0TileData::TileDType __in__ src0,
    typename Src1TileData::TileDType __in__ src1, typename Src2TileData::TileDType __in__ src2,
    typename Src3TileData::TileDType __in__ src3, unsigned dstCol, uint16_t &mrgSortList0, uint16_t &mrgSortList1,
    uint16_t &mrgSortList2, uint16_t &mrgSortList3, unsigned src0Col, unsigned src1Col, unsigned src2Col,
    unsigned src3Col)
{
    __ubuf__ typename DstTileData::DType *dstPtr = (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(dst);
    __ubuf__ typename DstTileData::DType *tmpPtr = (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(tmp);
    __ubuf__ typename DstTileData::DType *src0Ptr = (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(src0);
    __ubuf__ typename DstTileData::DType *src1Ptr = (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(src1);

    uint64_t config = InitConfig<exhausted>();

    uint64_t count = uint64_t(src0Col); // VMS4_SR[15:0], number of finished region proposals in list0
    count |= (uint64_t(src1Col) << 16);   // VMS4_SR[31:16], number of finished region proposals in list1

    if constexpr (listNum == LIST_NUM_2) {
        config |= (uint64_t(0b0011) << 8); // Xt[11:8]: 4-bit mask signal

        __ubuf__ typename DstTileData::DType *addrArray[LIST_NUM_2] = {
            (__ubuf__ typename DstTileData::DType *)(src0Ptr), (__ubuf__ typename DstTileData::DType *)(src1Ptr)};

        vmrgsort4(tmpPtr, addrArray, count, config);
    } else if constexpr (listNum == LIST_NUM_3) {
        __ubuf__ typename DstTileData::DType *src2Ptr =
            (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(src2);

        config |= (uint64_t(0b0111) << 8); // Xt[11:8]: 4-bit mask signal

        count |= (uint64_t(src2Col) << 32); // VMS4_SR[47:32], number of finished region proposals in list2

        __ubuf__
            typename DstTileData::DType *addrArray[LIST_NUM_3] = {(__ubuf__ typename DstTileData::DType *)(src0Ptr),
                (__ubuf__ typename DstTileData::DType *)(src1Ptr), (__ubuf__ typename DstTileData::DType *)(src2Ptr)};

        vmrgsort4(tmpPtr, addrArray, count, config);
    } else if constexpr (listNum == LIST_NUM_4) {
        __ubuf__ typename DstTileData::DType *src2Ptr =
            (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(src2);
        __ubuf__ typename DstTileData::DType *src3Ptr =
            (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(src3);

        config |= (uint64_t(0b1111) << 8); // Xt[11:8]: 4-bit mask signal

        count |= (uint64_t(src2Col) << 32); // VMS4_SR[47:32], number of finished region proposals in list2
        count |= (uint64_t(src3Col) << 48); // VMS4_SR[63:48], number of finished region proposals in list3

        __ubuf__ typename DstTileData::DType *addrArray[LIST_NUM_4] = {
            (__ubuf__ typename DstTileData::DType *)(src0Ptr), (__ubuf__ typename DstTileData::DType *)(src1Ptr),
            (__ubuf__ typename DstTileData::DType *)(src2Ptr), (__ubuf__ typename DstTileData::DType *)(src3Ptr)};

        vmrgsort4(tmpPtr, addrArray, count, config);
    }

    GetExhaustedData<exhausted>(mrgSortList0, mrgSortList1, mrgSortList2, mrgSortList3);
    MovUb2Ub<DstTileData>(dstPtr, tmpPtr, dstCol);
}

template <typename DstTileData, typename SrcTileData>
__tf__ AICORE void TMrgsort(typename DstTileData::TileDType __out__ dst, typename SrcTileData::TileDType __in__ src,
    uint32_t numStrcutures, uint8_t repeatTimes)
{
    __ubuf__ typename DstTileData::DType *dstPtr = (__ubuf__ typename DstTileData::DType *)__cce_get_tile_ptr(dst);
    __ubuf__ typename SrcTileData::DType *srcPtr = (__ubuf__ typename SrcTileData::DType *)__cce_get_tile_ptr(src);

    uint64_t config = uint64_t(repeatTimes); // Xt[7:0]: repeat time
    config |= (uint64_t(0b1111) << 8);       // Xt[11:8]: 4-bit mask signal
    config |= (uint64_t(0b0) << 12);         // Xt[12]: 1-enable input list exhausted suspension

    uint64_t count = (uint64_t(numStrcutures));  // VMS4_SR[15:0], length of block0 in the list
    count |= (uint64_t(numStrcutures) << 16);    // VMS4_SR[31:16], length of block1 in the list
    count |= (uint64_t(numStrcutures) << 32);    // VMS4_SR[47:32], length of block2 in the list
    count |= (uint64_t(numStrcutures) << 48);    // VMS4_SR[63:48], length of block3 in the list

    constexpr const int BLOCK3_INDEX = 2;
    constexpr const int BLOCK4_INDEX = 3;
    unsigned offset = numStrcutures * STRUCTSIZE / sizeof(typename DstTileData::DType);

    __ubuf__ typename SrcTileData::DType *addrArray[BLOCK_NUM] = {(__ubuf__ typename SrcTileData::DType *)(srcPtr),
        (__ubuf__ typename SrcTileData::DType *)(srcPtr + offset),
        (__ubuf__ typename SrcTileData::DType *)(srcPtr + offset * BLOCK3_INDEX),
        (__ubuf__ typename SrcTileData::DType *)(srcPtr + offset * BLOCK4_INDEX)};
    vmrgsort4(dstPtr, addrArray, count, config);
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, typename Src3TileData, unsigned listNum>
PTO_INTERNAL void CheckOverMemory()
{
    constexpr size_t elemSize = sizeof(typename DstTileData::DType);
    constexpr size_t tmpSize = (listNum == LIST_NUM_1) ? DstTileData::Cols * elemSize : TmpTileData::Cols * elemSize;
    static_assert((tmpSize + Src0TileData::Cols * elemSize) <= UB_SIZE,
        "ERROR: memory usage exceeds UB limit!");
    if constexpr (listNum >= LIST_NUM_2) {
        static_assert(Src1TileData::Cols * elemSize <= UB_SIZE, "ERROR: src1 memory usage exceeds UB limit!");
    }
    if constexpr (listNum >= LIST_NUM_3) {
        static_assert(Src2TileData::Cols * elemSize <= UB_SIZE, "ERROR: src2 memory usage exceeds UB limit!");
    }
    if constexpr (listNum >= LIST_NUM_4) {
        static_assert(Src3TileData::Cols * elemSize <= UB_SIZE, "ERROR: src3 memory usage exceeds UB limit!");
    }
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, typename Src3TileData>
PTO_INTERNAL void CheckStatic()
{
    using DstType = typename DstTileData::DType;
    static_assert((std::is_same<DstType, half>::value) || (std::is_same<DstType, float>::value),
        "TMrgsort: Unsupported data type! Supported types is half/float");
    static_assert((std::is_same<DstType, typename TmpTileData::DType>::value) &&
                      (std::is_same<DstType, typename Src0TileData::DType>::value) &&
                      (std::is_same<DstType, typename Src1TileData::DType>::value) &&
                      (std::is_same<DstType, typename Src2TileData::DType>::value) &&
                      (std::is_same<DstType, typename Src3TileData::DType>::value),
        "TMrgsort: Destination and Source tile data types must be the same.");
    static_assert((DstTileData::Loc == TileType::Vec) && (TmpTileData::Loc == TileType::Vec) &&
                      (Src0TileData::Loc == TileType::Vec) && (Src1TileData::Loc == TileType::Vec) &&
                      (Src2TileData::Loc == TileType::Vec) && (Src3TileData::Loc == TileType::Vec),
        "TMrgsort: the TileType of Destination and Source tile must be Vec.");
    static_assert((DstTileData::Rows == ONE_ROW) && (TmpTileData::Rows == ONE_ROW) && (Src0TileData::Rows == ONE_ROW) &&
                      (Src1TileData::Rows == ONE_ROW) && (Src2TileData::Rows == ONE_ROW),
        "TMrgsort: the row of Destination and Source tile must be 1.");
    static_assert((DstTileData::isRowMajor && TmpTileData::isRowMajor && Src0TileData::isRowMajor &&
                      Src1TileData::isRowMajor && Src2TileData::isRowMajor && Src3TileData::isRowMajor),
        "TMrgsort: the BLayout of Destination and Source tile must be RowMajor.");
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, typename Src3TileData, bool exhausted>
PTO_INTERNAL void TMRGSORT_IMPL(DstTileData &dst, MrgSortExecutedNumList &executedNumList, TmpTileData &tmp,
    Src0TileData &src0, Src1TileData &src1, Src2TileData &src2, Src3TileData &src3)
{
    CheckStatic<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src2TileData, Src3TileData>();
    CheckOverMemory<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src2TileData, Src3TileData, LIST_NUM_4>();
    unsigned dstCol = dst.GetValidCol();
    PTO_ASSERT((src0.GetValidCol() + src1.GetValidCol() + src2.GetValidCol() + src3.GetValidCol() + tmp.GetValidCol()) *
                       sizeof(typename DstTileData::DType) <
                   UB_SIZE,
        "ERROR: Total memory usage exceeds UB limit!");
    constexpr unsigned ELE_NUM_SHIFT = (std::is_same<typename DstTileData::DType, float>::value) ? 1 : 2;
    unsigned src0Col = src0.GetValidCol() >> ELE_NUM_SHIFT;
    unsigned src1Col = src1.GetValidCol() >> ELE_NUM_SHIFT;
    unsigned src2Col = src2.GetValidCol() >> ELE_NUM_SHIFT;
    unsigned src3Col = src3.GetValidCol() >> ELE_NUM_SHIFT;
    TMrgsort<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src2TileData, Src3TileData, exhausted, LIST_NUM_4>(
        dst.data(), tmp.data(), src0.data(), src1.data(), src2.data(), src3.data(), dstCol,
        executedNumList.mrgSortList0, executedNumList.mrgSortList1, executedNumList.mrgSortList2,
        executedNumList.mrgSortList3, src0Col, src1Col, src2Col, src3Col);
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData,
    typename Src2TileData, bool exhausted>
PTO_INTERNAL void TMRGSORT_IMPL(DstTileData &dst, MrgSortExecutedNumList &executedNumList, TmpTileData &tmp,
    Src0TileData &src0, Src1TileData &src1, Src2TileData &src2)
{
    CheckStatic<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src2TileData, Src0TileData>();
    CheckOverMemory<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src2TileData, Src0TileData, LIST_NUM_3>();
    unsigned dstCol = dst.GetValidCol();
    PTO_ASSERT((src0.GetValidCol() + src1.GetValidCol() + src2.GetValidCol() + tmp.GetValidCol()) *
                       sizeof(typename DstTileData::DType) <
                   UB_SIZE,
        "ERROR: Total memory usage exceeds UB limit!");
    constexpr unsigned ELE_NUM_SHIFT = (std::is_same<typename DstTileData::DType, float>::value) ? 1 : 2;
    unsigned src0Col = src0.GetValidCol() >> ELE_NUM_SHIFT;
    unsigned src1Col = src1.GetValidCol() >> ELE_NUM_SHIFT;
    unsigned src2Col = src2.GetValidCol() >> ELE_NUM_SHIFT;
    TMrgsort<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src2TileData, Src2TileData, exhausted, LIST_NUM_3>(
        dst.data(), tmp.data(), src0.data(), src1.data(), src2.data(), nullptr, dstCol, executedNumList.mrgSortList0,
        executedNumList.mrgSortList1, executedNumList.mrgSortList2, executedNumList.mrgSortList3, src0Col, src1Col,
        src2Col, EMPTY_LIST_SIZE);
}

template <typename DstTileData, typename TmpTileData, typename Src0TileData, typename Src1TileData, bool exhausted>
PTO_INTERNAL void TMRGSORT_IMPL(
    DstTileData &dst, MrgSortExecutedNumList &executedNumList, TmpTileData &tmp, Src0TileData &src0, Src1TileData &src1)
{
    CheckStatic<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src0TileData, Src0TileData>();
    CheckOverMemory<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src0TileData, Src0TileData, LIST_NUM_2>();
    unsigned dstCol = dst.GetValidCol();
    PTO_ASSERT(
        (src0.GetValidCol() + src1.GetValidCol() + tmp.GetValidCol()) * sizeof(typename DstTileData::DType) < UB_SIZE,
        "ERROR: Total memory usage exceeds UB limit!");
    constexpr unsigned ELE_NUM_SHIFT = (std::is_same<typename DstTileData::DType, float>::value) ? 1 : 2;
    unsigned src0Col = src0.GetValidCol() >> ELE_NUM_SHIFT;
    unsigned src1Col = src1.GetValidCol() >> ELE_NUM_SHIFT;
    TMrgsort<DstTileData, TmpTileData, Src0TileData, Src1TileData, Src1TileData, Src1TileData, exhausted, LIST_NUM_2>(
        dst.data(), tmp.data(), src0.data(), src1.data(), nullptr, nullptr, dstCol, executedNumList.mrgSortList0,
        executedNumList.mrgSortList1, executedNumList.mrgSortList2, executedNumList.mrgSortList3, src0Col, src1Col,
        EMPTY_LIST_SIZE, EMPTY_LIST_SIZE);
}

// The blockLen size includes values and indexes, such as 32 values and indexes: blockLen=64
template <typename DstTileData, typename SrcTileData>
PTO_INTERNAL void TMRGSORT_IMPL(DstTileData &dst, SrcTileData &src, uint32_t blockLen)
{
    CheckStatic<DstTileData, DstTileData, SrcTileData, SrcTileData, SrcTileData, SrcTileData>();
    CheckOverMemory<DstTileData, DstTileData, SrcTileData, SrcTileData, SrcTileData, SrcTileData, LIST_NUM_1>();
    uint32_t dstCol = dst.GetValidCol();
    uint32_t srcCol = src.GetValidCol();
    PTO_ASSERT((srcCol + dstCol) * sizeof(typename DstTileData::DType) < UB_SIZE,
        "ERROR: Total memory usage exceeds UB limit!");
    // A struct is 8 bytes
    uint32_t numStrcutures = blockLen * sizeof(typename SrcTileData::DType) >> STRUCT_SIZE_SHIFT;
    PTO_ASSERT(blockLen % TMRGSORT_BLOCK_LEN == 0, "blockLen is a multiple of 64");
    PTO_ASSERT(srcCol % (blockLen * BLOCK_NUM) == 0,
        "ERROR: The input Tile Valid size requirement is an integer multiple of blockLen * 4.");
    uint8_t repeatTimes = srcCol / (blockLen * BLOCK_NUM);
    PTO_ASSERT(repeatTimes >= REPEAT_ONE_TIME && repeatTimes <= MAX_REPEAT_TIMES,
        "ERROR: The range of Tile Valid divided by blockLen is [1,255].");
    TMrgsort<DstTileData, SrcTileData>(dst.data(), src.data(), numStrcutures, repeatTimes);
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
#endif