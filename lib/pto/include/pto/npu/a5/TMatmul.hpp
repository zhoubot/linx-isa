/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TMATMUL_HPP
#define TMATMUL_HPP

namespace pto {
template <typename TileLeft>
PTO_INTERNAL constexpr bool GetGemvCtrl()
{
    return TileLeft::Rows != 1;
}

template <typename TileRes, typename TileLeft, typename TileRight, bool cmatrixSource, bool cmatrixInitVal>
__tf__ AICORE void TMatmul(typename TileRes::TileDType __out__ cMatrix, typename TileLeft::TileDType __in__ aMatrix,
    typename TileRight::TileDType __in__ bMatrix, uint16_t m, uint16_t k, uint16_t n)
{
    constexpr bool gemvCtrl = GetGemvCtrl<TileLeft>();

    __cc__ typename TileRes::DType *c = (__cc__ typename TileRes::DType *)__cce_get_tile_ptr(cMatrix);
    __ca__ typename TileLeft::DType *a = (__ca__ typename TileLeft::DType *)__cce_get_tile_ptr(aMatrix);
    __cb__ typename TileRight::DType *b = (__cb__ typename TileRight::DType *)__cce_get_tile_ptr(bMatrix);

    mad(c, a, b, m, k, n, 0, gemvCtrl, cmatrixSource, cmatrixInitVal);
}

template <typename TileRes, typename TileLeft, typename TileRight, bool cmatrixSource, bool cmatrixInitVal>
__tf__ AICORE void TMatmulBias(typename TileRes::TileDType __out__ cMatrix, typename TileLeft::TileDType __in__ aMatrix,
    typename TileRight::TileDType __in__ bMatrix, uint64_t bias, uint16_t m, uint16_t k, uint16_t n)
{
    constexpr bool gemvCtrl = GetGemvCtrl<TileLeft>();

    __cc__ typename TileRes::DType *c = (__cc__ typename TileRes::DType *)__cce_get_tile_ptr(cMatrix);
    __ca__ typename TileLeft::DType *a = (__ca__ typename TileLeft::DType *)__cce_get_tile_ptr(aMatrix);
    __cb__ typename TileRight::DType *b = (__cb__ typename TileRight::DType *)__cce_get_tile_ptr(bMatrix);
    uint64_t xd = ((uint64_t)c) & 0xffffffffULL | ((bias & 0xffffffffULL) << 32);
    c = (__cc__ typename TileRes::DType *)xd;

    mad(c, a, b, m, k, n, 0, gemvCtrl, cmatrixSource, cmatrixInitVal);
}

template <typename TileRes, typename TileLeft, typename TileRight, bool biasBufferCtrl, bool cmatrixInitVal>
__tf__ AICORE void TMatmulMx(typename TileRes::TileDType __out__ cMatrix, typename TileLeft::TileDType __in__ aMatrix,
    typename TileRight::TileDType __in__ bMatrix, uint16_t m, uint16_t k, uint16_t n)
{
    // CmatrixInitVal Indicates the initial matrix, 1: the number in C matrix is 0, 0：use the real number in C matrix
    constexpr bool gemvCtrl = GetGemvCtrl<TileLeft>();

    __cc__ typename TileRes::DType *c = (__cc__ typename TileRes::DType *)__cce_get_tile_ptr(cMatrix);
    __ca__ typename TileLeft::DType *a = (__ca__ typename TileLeft::DType *)__cce_get_tile_ptr(aMatrix);
    __cb__ typename TileRight::DType *b = (__cb__ typename TileRight::DType *)__cce_get_tile_ptr(bMatrix);

    mad_mx(c, a, b, m, k, n, 0, gemvCtrl, biasBufferCtrl, cmatrixInitVal);
}

template <typename TileRes, typename TileLeft, typename TileRight, bool biasBufferCtrl, bool cmatrixInitVal>
__tf__ AICORE void TMatmulMxBias(typename TileRes::TileDType __out__ cMatrix,
    typename TileLeft::TileDType __in__ aMatrix, typename TileRight::TileDType __in__ bMatrix, uint64_t bias,
    uint16_t m, uint16_t k, uint16_t n)
{
    constexpr bool gemvCtrl = GetGemvCtrl<TileLeft>();

    __cc__ typename TileRes::DType *c = (__cc__ typename TileRes::DType *)__cce_get_tile_ptr(cMatrix);
    __ca__ typename TileLeft::DType *a = (__ca__ typename TileLeft::DType *)__cce_get_tile_ptr(aMatrix);
    __cb__ typename TileRight::DType *b = (__cb__ typename TileRight::DType *)__cce_get_tile_ptr(bMatrix);
    uint64_t xd = ((uint64_t)c) & 0xffffffffULL | ((bias & 0xffffffffULL) << 32);
    c = (__cc__ typename TileRes::DType *)xd;

    mad_mx(c, a, b, m, k, n, 0, gemvCtrl, biasBufferCtrl, cmatrixInitVal);
}

template<typename A, typename B>
constexpr bool isSupportedFp4Combo = 
    (std::is_same_v<A, float4_e1m2x2_t> && std::is_same_v<B, float4_e1m2x2_t>) ||
    (std::is_same_v<A, float4_e1m2x2_t> && std::is_same_v<B, float4_e2m1x2_t>) ||
    (std::is_same_v<A, float4_e2m1x2_t> && std::is_same_v<B, float4_e2m1x2_t>) ||
    (std::is_same_v<A, float4_e2m1x2_t> && std::is_same_v<B, float4_e1m2x2_t>);

template<typename A, typename B>
constexpr bool isSupportedFp8Combo = 
    (std::is_same_v<A, float8_e4m3_t> && std::is_same_v<B, float8_e4m3_t>) ||
    (std::is_same_v<A, float8_e4m3_t> && std::is_same_v<B, float8_e5m2_t>) ||
    (std::is_same_v<A, float8_e5m2_t> && std::is_same_v<B, float8_e4m3_t>) ||
    (std::is_same_v<A, float8_e5m2_t> && std::is_same_v<B, float8_e5m2_t>);

template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
PTO_INTERNAL void CheckMadMxValid()
{
    constexpr const int BASEK = 64;
    using AType = typename TileLeft::DType;
    using BType = typename TileRight::DType;
    using CType = typename TileRes::DType;
    constexpr bool isFp4 = isSupportedFp4Combo<AType, BType>;
    constexpr bool isFp8 = isSupportedFp8Combo<AType, BType>;

    static_assert((isFp4 || isFp8) && std::is_same_v<CType, float>, "TMatmulMX:No supported data type combination.");
    static_assert((TileLeft::Cols % BASEK == 0), "TMatmulMX:k must be a multiple of 64.");
    if constexpr (isFp4) {
        static_assert((TileLeft::Cols % 2 == 0), "TMatmulMX:For FP4 data types, k must be an even number.");
    }
    static_assert(
        ((TileLeft::Loc == TileType::Left) && (!TileLeft::isRowMajor) && (TileLeft::SFractal == SLayout::RowMajor)) &&
            ((TileRight::Loc == TileType::Right) && (TileRight::isRowMajor) &&
                (TileRight::SFractal == SLayout::ColMajor)) &&
            ((TileRes::Loc == TileType::Acc) && (!TileRes::isRowMajor) && (TileRes::SFractal == SLayout::RowMajor)),
        "TMatmulMX:Non-conforming matrix fractal");
}

PTO_INTERNAL void CheckDynamicMmad(uint16_t aMatrixRow, uint16_t aMatrixCol, uint16_t bMatrixCol)
{
    constexpr uint16_t elementSize = 4095;
    PTO_ASSERT(aMatrixRow >= 1 && aMatrixRow <= elementSize, "ERROR: The range of valid aMatrixRow is [1, 4095].");
    PTO_ASSERT(aMatrixCol >= 1 && aMatrixCol <= elementSize, "ERROR: The range of valid aMatrixCol is [1, 4095].");
    PTO_ASSERT(bMatrixCol >= 1 && bMatrixCol <= elementSize, "ERROR: The range of valid bMatrixCol is [1, 4095].");
}


template <typename TileRes, typename TileLeft, typename TileRight>
PTO_INTERNAL void CheckMadValid()
{
    using AType = typename TileLeft::DType;
    using BType = typename TileRight::DType;
    using CType = typename TileRes::DType;
    static_assert(std::is_same_v<CType, int32_t> || std::is_same_v<CType, float>, "Acc Type support int32_t or float.");
    if constexpr (std::is_same_v<CType, int32_t>) {
        static_assert(std::is_same_v<AType, int8_t> && std::is_same_v<BType, int8_t>,
            "Left Type and Rigth Type must be int8_t when Acc Type is int32_t.");
    } else if constexpr (std::is_same_v<CType, float>) {
        static_assert((std::is_same_v<AType, half> && std::is_same_v<BType, half>) ||
                          (std::is_same_v<AType, bfloat16_t> && std::is_same_v<BType, bfloat16_t>) ||
                          (std::is_same_v<AType, float> && std::is_same_v<BType, float>) ||
                          (std::is_same_v<AType, float8_e4m3_t> && std::is_same_v<BType, float8_e4m3_t>) ||
                          (std::is_same_v<AType, float8_e4m3_t> && std::is_same_v<BType, float8_e5m2_t>) ||
                          (std::is_same_v<AType, float8_e5m2_t> && std::is_same_v<BType, float8_e4m3_t>) ||
                          (std::is_same_v<AType, float8_e5m2_t> && std::is_same_v<BType, float8_e5m2_t>) ||
                          (std::is_same_v<AType, hifloat8_t> && std::is_same_v<BType, hifloat8_t>),
            "No supported data type when Acc Type is float.");
    }
    static_assert(
        ((TileLeft::Loc == TileType::Left) && (!TileLeft::isRowMajor) && (TileLeft::SFractal == SLayout::RowMajor)) &&
            ((TileRight::Loc == TileType::Right) && (TileRight::isRowMajor) &&
                (TileRight::SFractal == SLayout::ColMajor)) &&
            ((TileRes::Loc == TileType::Acc) && (!TileRes::isRowMajor) && (TileRes::SFractal == SLayout::RowMajor)),
        "Non-conforming matrix fractal.");
}

template <typename TileRes, typename TileLeft, typename TileRight>
PTO_INTERNAL void TMATMUL_IMPL(TileRes &cMatrix, TileLeft &aMatrix, TileRight &bMatrix)
{
    // cmatrixInitVal Indicates the initial matrix, 1: the number in C matrix is 0, 0：use the real number in C matrix
    CheckMadValid<TileRes, TileLeft, TileRight>();

    uint16_t m = aMatrix.GetValidRow();
    uint16_t k = aMatrix.GetValidCol();
    uint16_t n = bMatrix.GetValidCol();
    CheckDynamicMmad(m, k, n);

    TMatmul<TileRes, TileLeft, TileRight, false, true>(cMatrix.data(), aMatrix.data(), bMatrix.data(), m, k, n);
}

template <typename TileRes, typename TileLeft, typename TileRight>
PTO_INTERNAL void TMATMUL_ACC_IMPL(TileRes &cOutMatrix, TileRes &cInMatrix, TileLeft &aMatrix, TileRight &bMatrix)
{
    // cmatrixInitVal Indicates the initial matrix, 1: the number in C matrix is 0, 0：use the real number in C matrix
    CheckMadValid<TileRes, TileLeft, TileRight>();

    uint16_t m = aMatrix.GetValidRow();
    uint16_t k = aMatrix.GetValidCol();
    uint16_t n = bMatrix.GetValidCol();
    CheckDynamicMmad(m, k, n);

    TMatmul<TileRes, TileLeft, TileRight, false, false>(cOutMatrix.data(), aMatrix.data(), bMatrix.data(), m, k, n);
}

template <typename TileRes, typename TileLeft, typename TileRight, typename TileBias>
PTO_INTERNAL void TMATMUL_BIAS_IMPL(TileRes &cMatrix, TileLeft &aMatrix, TileRight &bMatrix, TileBias &biasData)
{
    // cmatrixSource control matrix source, 0: C matrix is in L0C, 1: C matrix is in C2
    // cmatrixInitVal Indicates the initial matrix, 1: the number in C matrix is 0, 0：use the real number in C matrix
    CheckMadValid<TileRes, TileLeft, TileRight>();
    static_assert(std::is_same_v<typename TileRes::DType, typename TileBias::DType>, "No supported bias data type.");
    static_assert((TileBias::Loc == TileType::Bias) && (TileBias::Rows == 1) && (TileBias::isRowMajor),
        "Non-conforming bias fractal.");

    uint16_t m = aMatrix.GetValidRow();
    uint16_t k = aMatrix.GetValidCol();
    uint16_t n = bMatrix.GetValidCol();
    CheckDynamicMmad(m, k, n);

    TMatmulBias<TileRes, TileLeft, TileRight, true, false>(
        cMatrix.data(), aMatrix.data(), bMatrix.data(), biasData.data(), m, k, n);
}

template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
PTO_INTERNAL void TMATMUL_MX_IMPL(
    TileRes &cMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix, TileRight &bMatrix, TileRightScale &bScaleMatrix)
{
    using AType = typename TileLeft::DType;
    using BType = typename TileRight::DType;
    constexpr bool isMx = isSupportedFp4Combo<AType, BType> || isSupportedFp8Combo<AType, BType>;

    if constexpr (isMx) {
        uint16_t m = aMatrix.GetValidRow();
        uint16_t k = aMatrix.GetValidCol();
        uint16_t n = bMatrix.GetValidCol();
        CheckDynamicMmad(m, k, n);

        CheckMadMxValid<TileRes, TileLeft, TileLeftScale, TileRight, TileRightScale>();

        TMatmulMx<TileRes, TileLeft, TileRight, false, true>(cMatrix.data(), aMatrix.data(), bMatrix.data(), m, k, n);
    } else {
        (void)aScaleMatrix;
        (void)bScaleMatrix;
        TMATMUL_IMPL(cMatrix, aMatrix, bMatrix);
    }
}

template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
PTO_INTERNAL void TMATMUL_MX_IMPL(TileRes &cOutMatrix, TileRes &cInMatrix, TileLeft &aMatrix,
    TileLeftScale &aScaleMatrix, TileRight &bMatrix, TileRightScale &bScaleMatrix)
{
    using AType = typename TileLeft::DType;
    using BType = typename TileRight::DType;
    constexpr bool isMx = isSupportedFp4Combo<AType, BType> || isSupportedFp8Combo<AType, BType>;

    if constexpr (isMx) {
        uint16_t m = aMatrix.GetValidRow();
        uint16_t k = aMatrix.GetValidCol();
        uint16_t n = bMatrix.GetValidCol();
        CheckDynamicMmad(m, k, n);

        CheckMadMxValid<TileRes, TileLeft, TileLeftScale, TileRight, TileRightScale>();

        TMatmulMx<TileRes, TileLeft, TileRight, false, false>(cOutMatrix.data(), aMatrix.data(), bMatrix.data(), m, k, n);
    } else {
        (void)aScaleMatrix;
        (void)bScaleMatrix;
        TMATMUL_ACC_IMPL(cOutMatrix, cInMatrix, aMatrix, bMatrix);
    }
}

template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale,
    typename TileBias>
PTO_INTERNAL void TMATMUL_MX_IMPL(TileRes &cMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix, TileRight &bMatrix,
    TileRightScale &bScaleMatrix, TileBias &biasData)
{
    using AType = typename TileLeft::DType;
    using BType = typename TileRight::DType;
    constexpr bool isMx = isSupportedFp4Combo<AType, BType> || isSupportedFp8Combo<AType, BType>;

    if constexpr (isMx) {
        CheckMadMxValid<TileRes, TileLeft, TileLeftScale, TileRight, TileRightScale>();
        static_assert(std::is_same_v<typename TileBias::DType, float>, "TMatmulMX:No supported bias data type.");
        static_assert((TileBias::Loc == TileType::Bias) && (TileBias::Rows == 1), "TMatmulMX:TileBias must be single row.");

        uint16_t m = aMatrix.GetValidRow();
        uint16_t k = aMatrix.GetValidCol();
        uint16_t n = bMatrix.GetValidCol();
        CheckDynamicMmad(m, k, n);

        TMatmulMxBias<TileRes, TileLeft, TileRight, true, false>(
            cMatrix.data(), aMatrix.data(), bMatrix.data(), biasData.data(), m, k, n);
    } else {
        (void)aScaleMatrix;
        (void)bScaleMatrix;
        TMATMUL_BIAS_IMPL(cMatrix, aMatrix, bMatrix, biasData);
    }
}
} // namespace pto
#endif
