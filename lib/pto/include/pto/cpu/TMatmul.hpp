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

#include "pto/cpu/tile_offsets.hpp"
#include "pto/cpu/parallel.hpp"

namespace pto {
    template <typename TileAcc, typename TileLeft, typename TileRight>
    void TMatmulNzZn(typename TileAcc::TileDType dst,
                       typename TileAcc::TileDType acc,
                       typename TileLeft::TileDType src0,
                       typename TileRight::TileDType src1,
                       uint16_t M, uint16_t N, uint16_t K)
    {
        cpu::parallel_for_1d(0, M, static_cast<std::size_t>(M) * N * K, [&](std::size_t i) {
            for (uint16_t j = 0; j < N; j++) {
                typename TileAcc::DType mul_acc = 0;

                PTO_CPU_VECTORIZE_LOOP
                for (uint16_t k = 0; k < K; k++) {
                    size_t src0Idx = GetTileElementOffset<TileLeft>(i, k);
                    size_t src1Idx = GetTileElementOffset<TileRight>(k, j);
                    mul_acc += static_cast<typename TileAcc::DType>(src0[src0Idx]) *
                               static_cast<typename TileAcc::DType>(src1[src1Idx]);
                }

                size_t dstIdx = GetTileElementOffset<TileAcc>(i, j);
                dst[dstIdx] = acc ? acc[dstIdx] + mul_acc : mul_acc;
            }
        });
    }

    template <typename TileAcc, typename TileLeft, typename TileRight>
    PTO_INTERNAL void CheckMadValid()
    {
        using AType = typename TileLeft::DType;
        using BType = typename TileRight::DType;
        using CType = typename TileAcc::DType;
        static_assert(
            (std::is_same_v<AType, int8_t> && std::is_same_v<BType, int8_t> && std::is_same_v<CType, int32_t>) ||  // s8
                (std::is_same_v<AType, int32_t> && std::is_same_v<BType, int32_t> &&
                    std::is_same_v<CType, int32_t>) ||                                                               // s322s32
                (std::is_same_v<AType, half> && std::is_same_v<BType, half> && std::is_same_v<CType, float>) ||  // f162f32
                (std::is_same_v<AType, float> && std::is_same_v<BType, float> &&
                    std::is_same_v<CType, float>)  // f322f32
            , "Not supported data type");
        static_assert(
            (TileLeft::Rows == TileAcc::Rows) && (TileLeft::Cols == TileRight::Rows) && (TileRight::Cols == TileAcc::Cols),
            "Inconsistent number of m, k, n");
        static_assert(
            (TileLeft::Loc == TileType::Left) && (TileRight::Loc == TileType::Right) && (TileAcc::Loc == TileType::Acc),
            "Non-conforming matrix loc");
        // CPU reference implements TMATMUL via element-wise offsets, so we intentionally accept
        // a broader set of tile layouts than the strict NPU hardware constraints.
    }

    template <typename TileAcc, typename TileBias>
    PTO_INTERNAL void CheckBiasValid()
    {
        using CType = typename TileAcc::DType;
        using BiasType = typename TileBias::DType;
        static_assert(std::is_same_v<CType, BiasType>, "No supported bias data type");
        static_assert((TileBias::Loc == TileType::Bias) && (TileBias::Rows == 1) && (TileBias::isRowMajor),
            "Non-conforming bias fractal");
    }

    template <typename TileAcc, typename TileLeft, typename TileRight>
    PTO_INTERNAL void TMATMUL_IMPL(TileAcc &cMatrix, TileLeft &aMatrix, TileRight &bMatrix)
    {
        CheckMadValid<TileAcc, TileLeft, TileRight>();

        uint16_t m = aMatrix.GetValidRow();
        uint16_t k = aMatrix.GetValidCol();
        uint16_t n = bMatrix.GetValidCol();

        TMatmulNzZn<TileAcc, TileLeft, TileRight>(cMatrix.data(), nullptr, aMatrix.data(), bMatrix.data(), m, n, k);
    }

    template <typename TileAcc, typename TileLeft, typename TileRight>
    PTO_INTERNAL void TMATMUL_ACC_IMPL(TileAcc &cOutMatrix, TileAcc &cInMatrix, TileLeft &aMatrix, TileRight &bMatrix)
    {
        CheckMadValid<TileAcc, TileLeft, TileRight>();

        uint16_t m = aMatrix.GetValidRow();
        uint16_t k = aMatrix.GetValidCol();
        uint16_t n = bMatrix.GetValidCol();

        TMatmulNzZn<TileAcc, TileLeft, TileRight>(cOutMatrix.data(), cInMatrix.data(), aMatrix.data(), bMatrix.data(), m, n, k);
    }

    template <typename TileAcc, typename TileLeft, typename TileRight, typename TileBias>
    PTO_INTERNAL void TMATMUL_BIAS_IMPL(TileAcc &cMatrix, TileLeft &aMatrix, TileRight &bMatrix, TileBias &biasMatrix)
    {
        CheckMadValid<TileAcc, TileLeft, TileRight>();
        CheckBiasValid<TileAcc, TileBias>();

        uint16_t m = aMatrix.GetValidRow();
        uint16_t k = aMatrix.GetValidCol();
        uint16_t n = bMatrix.GetValidCol();

        TMatmulNzZn<TileAcc, TileLeft, TileRight>(cMatrix.data(), nullptr, aMatrix.data(), bMatrix.data(), m, n, k);
        for(size_t c=0; c<n; c++) {
            for(size_t r=0; r<m; r++) {
                size_t out_idx = GetTileElementOffset<TileAcc>(r,c);
                size_t bias_idx = GetTileElementOffset<TileBias>(0,c);
                cMatrix.data()[out_idx] += biasMatrix.data()[bias_idx];
            }
        }
    }

    // Keep TMATMUL_MX available in the CPU reference backend by treating it as
    // a normal matmul/matmul_acc/matmul_bias. Scale tiles are accepted but are
    // currently ignored (matching the A2/A3 fallback behavior).
    template <typename TileAcc, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
    PTO_INTERNAL void TMATMUL_MX_IMPL(
        TileAcc &cMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix, TileRight &bMatrix, TileRightScale &bScaleMatrix)
    {
        (void)aScaleMatrix;
        (void)bScaleMatrix;
        TMATMUL_IMPL(cMatrix, aMatrix, bMatrix);
    }

    template <typename TileAcc, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale>
    PTO_INTERNAL void TMATMUL_MX_IMPL(TileAcc &cOutMatrix, TileAcc &cInMatrix, TileLeft &aMatrix,
        TileLeftScale &aScaleMatrix, TileRight &bMatrix, TileRightScale &bScaleMatrix)
    {
        (void)aScaleMatrix;
        (void)bScaleMatrix;
        TMATMUL_ACC_IMPL(cOutMatrix, cInMatrix, aMatrix, bMatrix);
    }

    template <typename TileAcc, typename TileLeft, typename TileLeftScale, typename TileRight, typename TileRightScale,
        typename TileBias>
    PTO_INTERNAL void TMATMUL_MX_IMPL(TileAcc &cMatrix, TileLeft &aMatrix, TileLeftScale &aScaleMatrix, TileRight &bMatrix,
        TileRightScale &bScaleMatrix, TileBias &biasMatrix)
    {
        (void)aScaleMatrix;
        (void)bScaleMatrix;
        TMATMUL_BIAS_IMPL(cMatrix, aMatrix, bMatrix, biasMatrix);
    }
}
#endif
