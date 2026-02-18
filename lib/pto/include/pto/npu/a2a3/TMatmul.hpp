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

template <typename TileLeft, typename TileRight>
PTO_INTERNAL bool GetKDirectionAlign(TileLeft &aMatrix, TileRight &bMatrix)
{
    // only for f322f32
    if constexpr (std::is_same<typename TileLeft::DType, float>::value &&
                  std::is_same<typename TileRight::DType, float>::value) {
        if (aMatrix.GetKAligned() || bMatrix.GetKAligned()) {
            return true;
        }
        return false;
    }
    return false;
}
template <typename TileRes, typename TileLeft, typename TileRight, bool cmatrixSource, bool cmatrixInitVal>
__tf__ AICORE void TMatmul(typename TileRes::TileDType __out__ cMatrix, typename TileLeft::TileDType __in__ aMatrix,
    typename TileRight::TileDType __in__ bMatrix, uint16_t m, uint16_t k, uint16_t n, bool kDirectionAlign)
{
    __cc__ typename TileRes::DType *c = (__cc__ typename TileRes::DType *)__cce_get_tile_ptr(cMatrix);
    __ca__ typename TileLeft::DType *a = (__ca__ typename TileLeft::DType *)__cce_get_tile_ptr(aMatrix);
    __cb__ typename TileRight::DType *b = (__cb__ typename TileRight::DType *)__cce_get_tile_ptr(bMatrix);
    if (m == 1) {
        m = 16; // avoid gemv mode, if m is 1, the gemv mode will be used in a3
    }
    mad(c, a, b, m, k, n, 0, kDirectionAlign, cmatrixSource, cmatrixInitVal);
}

template <typename TileRes, typename TileLeft, typename TileRight, bool cmatrixSource, bool cmatrixInitVal>
__tf__ AICORE void TMatmulBias(typename TileRes::TileDType __out__ cMatrix,
    typename TileLeft::TileDType __in__ aMatrix, typename TileRight::TileDType __in__ bMatrix, uint64_t bias,
    uint16_t m, uint16_t k, uint16_t n, bool kDirectionAlign)
{
    __cc__ typename TileRes::DType *c = (__cc__ typename TileRes::DType *)__cce_get_tile_ptr(cMatrix);
    __ca__ typename TileLeft::DType *a = (__ca__ typename TileLeft::DType *)__cce_get_tile_ptr(aMatrix);
    __cb__ typename TileRight::DType *b = (__cb__ typename TileRight::DType *)__cce_get_tile_ptr(bMatrix);
    uint64_t xd = ((uint64_t)c) & 0xffffffffULL | ((bias & 0xffffffffULL) << 32);
    c = (__cc__ typename TileRes::DType *)xd;
    if (m == 1) {
        m = 16; // avoid gemv mode, if m is 1, the gemv mode will be used in a3
    }
    mad(c, a, b, m, k, n, 0, kDirectionAlign, cmatrixSource, cmatrixInitVal);
}

template <typename TileRes, typename TileLeft, typename TileRight>
PTO_INTERNAL void CheckStaticMad()
{
    using AType = typename TileLeft::DType;
    using BType = typename TileRight::DType;
    using CType = typename TileRes::DType;
    static_assert(((std::is_same<CType, int32_t>::value) && (std::is_same<AType, int8_t>::value) &&
                      (std::is_same<BType, int8_t>::value)) ||
                      ((std::is_same<CType, float>::value) && (std::is_same<AType, half>::value) &&
                          (std::is_same<BType, half>::value)) ||
                      ((std::is_same<CType, float>::value) && (std::is_same<AType, float>::value) &&
                          (std::is_same<BType, float>::value)) ||
                      ((std::is_same<CType, float>::value) && (std::is_same<AType, bfloat16_t>::value) &&
                          (std::is_same<BType, bfloat16_t>::value)),
        "The data type is not supported.");

    static_assert(TileLeft::Loc == TileType::Left, "TileLeft TileType must be set to TileType::Left.");
    static_assert(TileRight::Loc == TileType::Right, "TileRight TileType must be set to TileType::Right.");
    static_assert(TileRes::Loc == TileType::Acc, "TileRes TileType must be set to TileType::Acc.");
}

PTO_INTERNAL void CheckDynamicMad(uint16_t aMatrixRow, uint16_t aMatrixCol, uint16_t bMatrixCol)
{
    constexpr uint16_t elementSize = 4095;
    PTO_ASSERT(aMatrixRow >= 1 && aMatrixRow <= elementSize, "ERROR: The range of valid aMatrixRow is [1, 4095].");
    PTO_ASSERT(aMatrixCol >= 1 && aMatrixCol <= elementSize, "ERROR: The range of valid aMatrixCol is [1, 4095].");
    PTO_ASSERT(bMatrixCol >= 1 && bMatrixCol <= elementSize, "ERROR: The range of valid bMatrixCol is [1, 4095].");
}

template <typename TileRes, typename TileLeft, typename TileRight>
PTO_INTERNAL void TMATMUL_IMPL(TileRes &cMatrix, TileLeft &aMatrix, TileRight &bMatrix)
{
    CheckStaticMad<TileRes, TileLeft, TileRight>();
    uint16_t m = aMatrix.GetValidRow();
    uint16_t k = aMatrix.GetValidCol();
    uint16_t n = bMatrix.GetValidCol();
    bool kDirectionAlign = GetKDirectionAlign(aMatrix, bMatrix);
    CheckDynamicMad(m, k, n);
    TMatmul<TileRes, TileLeft, TileRight, false, true>(cMatrix.data(), aMatrix.data(), bMatrix.data(), m, k, n, kDirectionAlign);
}

template <typename TileRes, typename TileLeft, typename TileRight>
PTO_INTERNAL void TMATMUL_ACC_IMPL(
    TileRes &cOutMatrix, TileRes &cInMatrix, TileLeft &aMatrix, TileRight &bMatrix)
{
    CheckStaticMad<TileRes, TileLeft, TileRight>();
    uint16_t m = aMatrix.GetValidRow();
    uint16_t k = aMatrix.GetValidCol();
    uint16_t n = bMatrix.GetValidCol();
    bool kDirectionAlign = GetKDirectionAlign(aMatrix, bMatrix);
    CheckDynamicMad(m, k, n);
    TMatmul<TileRes, TileLeft, TileRight, false, false>(cOutMatrix.data(), aMatrix.data(), bMatrix.data(), m, k, n, kDirectionAlign);
}

template <typename TileRes, typename TileLeft, typename TileRight, typename TileBias>
PTO_INTERNAL void TMATMUL_BIAS_IMPL(
    TileRes &cMatrix, TileLeft &aMatrix, TileRight &bMatrix, TileBias &biasData)
{
    CheckStaticMad<TileRes, TileLeft, TileRight>();
    static_assert(std::is_same_v<typename TileRes::DType, typename TileBias::DType>, "No supported bias data type.");
    static_assert((TileBias::Loc == TileType::Bias) && (TileBias::Rows == 1), "TileBias must be single row.");
    uint16_t m = aMatrix.GetValidRow();
    uint16_t k = aMatrix.GetValidCol();
    uint16_t n = bMatrix.GetValidCol();
    bool kDirectionAlign = GetKDirectionAlign(aMatrix, bMatrix);
    CheckDynamicMad(m, k, n);

    TMatmulBias<TileRes, TileLeft, TileRight, true, false>(
        cMatrix.data(), aMatrix.data(), bMatrix.data(), biasData.data(), m, k, n, kDirectionAlign);
}

// TMATMUL_MX is not a distinct hardware instruction on A2/A3 today. Keep the public
// PTO ISA surface available by treating it as a regular matmul / matmul_acc / matmul_bias,
// while still accepting (currently unused) scale tiles for forward compatibility.
template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
PTO_INTERNAL void TMATMUL_MX_IMPL(TileRes &cMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix, TileRight &bMatrix,
    TileRightScale &bScaleMatrix)
{
    (void)aScaleMatrix;
    (void)bScaleMatrix;
    TMATMUL_IMPL(cMatrix, aMatrix, bMatrix);
}

template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
PTO_INTERNAL void TMATMUL_MX_IMPL(TileRes &cOutMatrix, TileRes &cInMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix,
    TileRight &bMatrix, TileRightScale &bScaleMatrix)
{
    (void)aScaleMatrix;
    (void)bScaleMatrix;
    TMATMUL_ACC_IMPL(cOutMatrix, cInMatrix, aMatrix, bMatrix);
}

template <typename TileRes, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale,
    typename TileBias>
PTO_INTERNAL void TMATMUL_MX_IMPL(TileRes &cMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix, TileRight &bMatrix,
    TileRightScale &bScaleMatrix, TileBias &biasData)
{
    (void)aScaleMatrix;
    (void)bScaleMatrix;
    TMATMUL_BIAS_IMPL(cMatrix, aMatrix, bMatrix, biasData);
}
} // namespace pto
#endif
