/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TREMS_HPP
#define TREMS_HPP

#include <pto/common/constants.hpp>
#include <pto/npu/a2a3/TExpandS.hpp>

#define SRC1_INDEX 7

namespace pto {
template <typename T>
struct RemSOp {
  PTO_INTERNAL static void RemSF32Instr(__ubuf__ float *dst, __ubuf__ float *src, __ubuf__ float *src1,
                                        __ubuf__ float *tmp) {
    pipe_barrier(PIPE_V);
    vector_dup(src1, (float)41, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);

    // tmporary buffer size: validCols*sizeof(float)
    __ubuf__ int32_t *tmpPtr = (__ubuf__ int32_t *)tmp;
    // qf = s0 / s1
    pipe_barrier(PIPE_V);
    vdiv(dst, src, src1, 1, 1, 1, 1, 8, 8, 8);
    pipe_barrier(PIPE_V);
    // float32 path: convert float quotient -> int32 (truncate), then back to float
    // Convert float -> int32 with truncation
    vconv_f322s32z(tmpPtr, dst, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);
    // convert int32 back to float
    vconv_s322f32(dst, tmpPtr, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);
    // prod = qf * s1
    vmul(dst, dst, src1, 1, 1, 1, 1, 8, 8, 8);
    pipe_barrier(PIPE_V);
    // dst = s0 - prod
    vsub(dst, src, dst, 1, 1, 1, 1, 8, 8, 8);
    pipe_barrier(PIPE_V);
  }

  PTO_INTERNAL static void RemSF16Instr(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *src1, __ubuf__ T *tmp,
                                        unsigned rowStride) {
    // tmporary buffer size: validCols*sizeof(float)*4
    __ubuf__ float *tmpPtr = (__ubuf__ float *)tmp;
    __ubuf__ float *tmpSrc = tmpPtr + rowStride;
    __ubuf__ float *tmpSrc1 = tmpPtr + rowStride * 2;
    __ubuf__ float *tmpShare = tmpPtr + rowStride * 3;
    pipe_barrier(PIPE_V);
    vconv_f162f32(tmpSrc, src, 1, 1, 1, 8, 8);
    vconv_f162f32(tmpSrc1, src1, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);
    RemSF32Instr(tmpPtr, tmpSrc, tmpSrc1, tmpShare);
    pipe_barrier(PIPE_V);
    vconv_f322f16(dst, tmpPtr, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);
  }

  PTO_INTERNAL static void RemSInt32Instr(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *src1, __ubuf__ T *tmpPtr,
                                          unsigned rowStride) {
    // tmporary buffer size: validCols*sizeof(float)*7
    constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
    __ubuf__ float *src_f = reinterpret_cast<__ubuf__ float *>(tmpPtr);
    __ubuf__ float *src1_f = src_f + rowStride;
    __ubuf__ float *qf = src1_f + rowStride;
    __ubuf__ float *prod = qf + rowStride;
    __ubuf__ int32_t *qf_int = reinterpret_cast<__ubuf__ int32_t *>(prod + rowStride); // reuse prod buffer
    __ubuf__ float *qf_trunc_f = reinterpret_cast<__ubuf__ float *>(qf_int + rowStride);
    __ubuf__ float *rem_f = prod + rowStride;
    // int->float
    pipe_barrier(PIPE_V);
    vconv_s322f32(src_f, (__ubuf__ int32_t *)src, 1, 1, 1, 8, 8);
    vconv_s322f32(src1_f, (__ubuf__ int32_t *)src1, 1, 1, 1, 8, 8);
    // 2. qf = src_f / src1_f
    pipe_barrier(PIPE_V);
    vdiv(qf, src_f, src1_f, 1, 1, 1, 1, 8, 8, 8);
    // 3. qf_int = trunc(qf)
    pipe_barrier(PIPE_V);
    vconv_f322s32z(qf_int, qf, 1, 1, 1, 8, 8);
    // 4. qf_trunc_f = float(qf_int)
    pipe_barrier(PIPE_V);
    vconv_s322f32(qf_trunc_f, qf_int, 1, 1, 1, 8, 8);
    // 5. prod = qf_trunc_f * src1_f
    pipe_barrier(PIPE_V);
    vmul(prod, qf_trunc_f, src1_f, 1, 1, 1, 1, 8, 8, 8);
    // 6. rem_f = src_f - prod
    pipe_barrier(PIPE_V);
    vsub(rem_f, src_f, prod, 1, 1, 1, 1, 8, 8, 8);
    // 7. float->int
    pipe_barrier(PIPE_V);
    vconv_f322s32z((__ubuf__ int32_t *)dst, rem_f, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);
  }

  PTO_INTERNAL static void RemSInt16Instr(__ubuf__ T *dst, __ubuf__ T *src, __ubuf__ T *src1, __ubuf__ T *tmpPtr,
                                          unsigned rowStride) {
    // tmporary buffer size: validCols*sizeof(float)*6
    __ubuf__ half *src_f = reinterpret_cast<__ubuf__ half *>(tmpPtr);
    __ubuf__ half *src1_f = src_f + rowStride;
    __ubuf__ half *qf = src1_f + rowStride;
    __ubuf__ half *prod = qf + rowStride;
    __ubuf__ int16_t *qf_int = reinterpret_cast<__ubuf__ int16_t *>(prod + rowStride);
    __ubuf__ half *qf_trunc_f = reinterpret_cast<__ubuf__ half *>(qf_int + rowStride);
    __ubuf__ half *rem_f = prod + rowStride;
    // need tmporary buffer
    pipe_barrier(PIPE_V);
    vconv_s162f16(src_f, (__ubuf__ int16_t *)src, 1, 1, 1, 8, 8);
    vconv_s162f16(src1_f, (__ubuf__ int16_t *)src1, 1, 1, 1, 8, 8);
    // 2. qf = src_f / src1_f
    pipe_barrier(PIPE_V);
    vdiv(qf, src_f, src1_f, 1, 1, 1, 1, 8, 8, 8);
    // 3. qf_int = trunc(qf)
    pipe_barrier(PIPE_V);
    vconv_f162s16z(qf_int, qf, 1, 1, 1, 8, 8);
    // 4. qf_trunc_f = half(qf_int)
    pipe_barrier(PIPE_V);
    vconv_s162f16(qf_trunc_f, qf_int, 1, 1, 1, 8, 8);
    // 5. prod = qf_trunc_f * src1_f
    pipe_barrier(PIPE_V);
    vmul(prod, qf_trunc_f, src1_f, 1, 1, 1, 1, 8, 8, 8);
    // 6. rem_f = src_f - prod
    pipe_barrier(PIPE_V);
    vsub(rem_f, src_f, prod, 1, 1, 1, 1, 8, 8, 8);
    // 7. half->int16
    pipe_barrier(PIPE_V);
    vconv_f162s16z((__ubuf__ int16_t *)dst, rem_f, 1, 1, 1, 8, 8);
    pipe_barrier(PIPE_V);
  }
};

template <typename TileData, unsigned elementsPerRepeat, unsigned blockSizeElem, unsigned dstRowStride,
          unsigned srcRowStride, unsigned tmpRowStride>
__tf__ PTO_INTERNAL void TRemS(typename TileData::TileDType __out__ dst, typename TileData::TileDType __in__ src,
                               typename TileData::TileDType __in__ tmp, unsigned validRows, unsigned validCols) {
  using T = typename TileData::DType;

  __ubuf__ T *dstPtr = (__ubuf__ T *)__cce_get_tile_ptr(dst);
  __ubuf__ T *srcPtr = (__ubuf__ T *)__cce_get_tile_ptr(src);
  __ubuf__ T *tmpPtr = (__ubuf__ T *)__cce_get_tile_ptr(tmp); // tmp buffer

  set_mask_count();
  set_vector_mask(0, validCols);
  __ubuf__ T *s1Next = tmpPtr + SRC1_INDEX * tmpRowStride;
  for (int i = 0; i < validRows; ++i) {
    __ubuf__ T *dstNext = dstPtr + i * dstRowStride;
    __ubuf__ T *s0Next = srcPtr + i * srcRowStride;
    if constexpr (std::is_same_v<T, float> || std::is_same_v<T, float32_t>) {
      RemSOp<T>::RemSF32Instr(dstNext, s0Next, s1Next, tmpPtr);
    } else if constexpr (std::is_same_v<T, half> || std::is_same_v<T, float16_t>) {
      RemSOp<T>::RemSF16Instr(dstNext, s0Next, s1Next, tmpPtr, tmpRowStride);
    } else if constexpr (std::is_same_v<T, int32_t>) {
      RemSOp<T>::RemSInt32Instr(dstNext, s0Next, s1Next, tmpPtr, tmpRowStride);
    } else if constexpr (std::is_same_v<T, int16_t>) {
      RemSOp<T>::RemSInt16Instr(dstNext, s0Next, s1Next, tmpPtr, tmpRowStride);
    } else {
      static_assert(sizeof(T) == 0, "TREMS: Unsupported tile DType.");
    }
  }
  set_mask_norm();
  set_vector_mask(-1, -1);
}

template <typename TileDataDst, typename TileDataSrc, typename TileDataTmp>
PTO_INTERNAL void TREMS_IMPL(TileDataDst &dst, TileDataSrc &src, typename TileDataSrc::DType scalar, TileDataTmp &tmp) {
  using T = typename TileDataDst::DType;

  // static assertions
  static_assert(std::is_same_v<T, typename TileDataSrc::DType> && std::is_same_v<T, typename TileDataTmp::DType>,
                "TREMS: The data types of dst, src and tmp must be the same.");
  static_assert(std::is_same<T, int32_t>::value || std::is_same<T, int>::value || std::is_same<T, int16_t>::value ||
                    std::is_same<T, half>::value || std::is_same<T, float16_t>::value ||
                    std::is_same<T, float>::value || std::is_same<T, float32_t>::value,
                "TREMS: Invalid data type");
  static_assert(TileDataSrc::Loc == TileType::Vec && TileDataDst::Loc == TileType::Vec &&
                    TileDataTmp::Loc == TileType::Vec,
                "TREMS: TileType of src and dst tiles must be TileType::Vec.");
  static_assert(TileDataDst::isRowMajor && TileDataSrc::isRowMajor && TileDataTmp::isRowMajor,
                "TREMS: Only support row major layout.");

  // dynamic checks
  PTO_ASSERT(tmp.GetValidRow() >= 8, "TREMS: Number of valid rows of tmp tile must be at least 8.");
  PTO_ASSERT(dst.GetValidRow() == src.GetValidRow() && dst.GetValidRow() > 0,
             "TREMS: Number of valid rows of src and dst must be the same, and both greater than 0.");
  PTO_ASSERT(dst.GetValidCol() == src.GetValidCol() && dst.GetValidCol() == tmp.GetValidCol(),
             "TREMS: Number of valid columns of src, dst and tmp must be the same, and all greater than 0.");

  TEXPANDS_IMPL(tmp, scalar);

  constexpr unsigned blockSizeElem = BLOCK_BYTE_SIZE / sizeof(T);
  constexpr unsigned elementsPerRepeat = REPEAT_BYTE / sizeof(T);
  constexpr unsigned dstRowStride = TileDataDst::RowStride;
  constexpr unsigned srcRowStride = TileDataSrc::RowStride;
  constexpr unsigned TmpRowStride = TileDataTmp::RowStride;
  TRemS<TileDataDst, elementsPerRepeat, blockSizeElem, dstRowStride, srcRowStride, TmpRowStride>(
      dst.data(), src.data(), tmp.data(), dst.GetValidRow(), dst.GetValidCol());
}
} // namespace pto

#endif