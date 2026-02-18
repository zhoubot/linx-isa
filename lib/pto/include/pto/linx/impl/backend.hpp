#ifndef PTO_LINX_IMPL_BACKEND_HPP
#define PTO_LINX_IMPL_BACKEND_HPP

#include <stdint.h>
#if defined(PTO_HOST_SIM)
#include <math.h>
#include <string.h>
#endif

namespace pto {
namespace linx {
namespace detail {

template <typename... Ts>
struct dependent_false {
  static constexpr bool value = false;
};

template <typename T>
struct is_arithmetic {
  static constexpr bool value = false;
};

template <>
struct is_arithmetic<int> {
  static constexpr bool value = true;
};

template <>
struct is_arithmetic<unsigned> {
  static constexpr bool value = true;
};

template <>
struct is_arithmetic<long long> {
  static constexpr bool value = true;
};

template <>
struct is_arithmetic<unsigned long long> {
  static constexpr bool value = true;
};

template <>
struct is_arithmetic<float> {
  static constexpr bool value = true;
};

template <>
struct is_arithmetic<double> {
  static constexpr bool value = true;
};

template <typename T>
struct is_floating_point {
  static constexpr bool value = false;
};

template <>
struct is_floating_point<float> {
  static constexpr bool value = true;
};

template <>
struct is_floating_point<double> {
  static constexpr bool value = true;
};

template <typename T>
struct DTypeCode {
  static_assert(dependent_false<T>::value,
                "PTO Linx strict-v0.3: unsupported tile dtype");
};

template <>
struct DTypeCode<int> { static constexpr unsigned value = 17u; };

template <>
struct DTypeCode<unsigned> { static constexpr unsigned value = 25u; };

template <>
struct DTypeCode<float> { static constexpr unsigned value = 1u; };

template <>
struct DTypeCode<signed char> { static constexpr unsigned value = 19u; };

template <>
struct DTypeCode<unsigned char> { static constexpr unsigned value = 27u; };

template <>
struct DTypeCode<short> { static constexpr unsigned value = 18u; };

template <>
struct DTypeCode<unsigned short> { static constexpr unsigned value = 26u; };

template <>
struct DTypeCode<long long> { static constexpr unsigned value = 16u; };

template <>
struct DTypeCode<unsigned long long> { static constexpr unsigned value = 24u; };

template <>
struct DTypeCode<double> { static constexpr unsigned value = 0u; };

constexpr unsigned kMinTileBytes = 512u;
constexpr unsigned kMaxTileBytes = 4096u;
constexpr unsigned kTileWords = kMaxTileBytes / sizeof(uint32_t);

#if defined(PTO_HOST_SIM)
struct RawTile {
  alignas(64) uint32_t words[kTileWords];
};
#else
using RawTile = int __attribute__((vector_size(4096)));
#endif

constexpr unsigned clampTileBytes(unsigned bytes) {
  return bytes < kMinTileBytes ? kMinTileBytes
                               : (bytes > kMaxTileBytes ? kMaxTileBytes : bytes);
}

constexpr unsigned nextPow2(unsigned value) {
  unsigned p = 1u;
  while (p < value && p < kMaxTileBytes)
    p <<= 1u;
  return p;
}

constexpr unsigned sizeCodeFromBytes(unsigned bytes) {
  const unsigned clipped = clampTileBytes(bytes);
  const unsigned p2 = nextPow2(clipped);
  unsigned code = 0u;
  while ((1u << (code + 4u)) < p2)
    ++code;
  if (code < 5u)
    code = 5u;
  if (code > 8u)
    code = 8u;
  return code;
}

constexpr unsigned dtypeElemBits(unsigned dtype) {
  switch (dtype & 0x1fu) {
  case 0u:  // FP64
  case 16u: // INT64
  case 24u: // UINT64
    return 64u;
  case 1u:  // FP32
  case 17u: // INT32
  case 25u: // UINT32
    return 32u;
  case 2u:  // FP16
  case 6u:  // BF16
  case 18u: // INT16
  case 26u: // UINT16
    return 16u;
  case 3u:  // FP8
  case 7u:  // FPL8
  case 19u: // INT8
  case 27u: // UINT8
    return 8u;
  case 11u: // FP4
  case 12u: // FPL4
  case 20u: // INT4
  case 28u: // UINT4
    return 4u;
  default:
    return 32u;
  }
}

constexpr unsigned dtypeElemBytesForStorage(unsigned dtype) {
  const unsigned bits = dtypeElemBits(dtype);
  return (bits + 7u) / 8u;
}

constexpr unsigned dtypeElemCountForBytes(uint64_t bytes, unsigned dtype) {
  const unsigned bits = dtypeElemBits(dtype);
  if (bits == 0u)
    return 0u;
  const uint64_t total_bits = bytes * 8u;
  return static_cast<unsigned>(total_bits / bits);
}

template <typename Scalar>
inline long long encodeScalar(Scalar value) {
  static_assert(is_arithmetic<Scalar>::value,
                "PTO Linx strict-v0.3: scalar operand must be arithmetic");
  if constexpr (is_floating_point<Scalar>::value) {
    if constexpr (sizeof(Scalar) == sizeof(uint32_t)) {
      union {
        Scalar f;
        uint32_t u;
      } cvt = {value};
      return static_cast<long long>(cvt.u);
    } else if constexpr (sizeof(Scalar) == sizeof(uint64_t)) {
      union {
        Scalar f;
        uint64_t u;
      } cvt = {value};
      return static_cast<long long>(cvt.u);
    } else {
      return static_cast<long long>(value);
    }
  }
  return static_cast<long long>(value);
}

#if defined(PTO_HOST_SIM)

inline uint64_t sizeBytesFromCode(unsigned size_code) {
  return (size_code < 60u) ? (1ull << (size_code + 4u)) : 0ull;
}

template <typename T>
inline uint32_t bitCastToU32(T value) {
  static_assert(sizeof(T) == sizeof(uint32_t), "bitCastToU32 requires 32-bit type");
  uint32_t out = 0;
  memcpy(&out, &value, sizeof(uint32_t));
  return out;
}

template <typename T>
inline T bitCastFromU32(uint32_t bits) {
  static_assert(sizeof(T) == sizeof(uint32_t), "bitCastFromU32 requires 32-bit type");
  T out{};
  memcpy(&out, &bits, sizeof(uint32_t));
  return out;
}

inline float scalarAsF32(long long scalar_bits) {
  uint32_t bits = static_cast<uint32_t>(scalar_bits & 0xffffffffull);
  return bitCastFromU32<float>(bits);
}

inline int32_t scalarAsI32(long long scalar_bits) {
  return static_cast<int32_t>(scalar_bits & 0xffffffffull);
}

template <unsigned TileOp10>
inline RawTile teplUnaryHost(const RawTile &src, unsigned elems) {
  RawTile out{};
  for (unsigned i = 0; i < kTileWords; ++i)
    out.words[i] = 0u;

  switch (TileOp10 & 0x3ffu) {
  case 0x00fu: // TCVT
    for (unsigned i = 0; i < elems && i < kTileWords; ++i)
      out.words[i] = src.words[i];
    break;
  case 0x020u: // TROWMAX (fallback: identity under host backend)
  case 0x022u: // TROWSUM (fallback: identity under host backend)
  case 0x027u: // TCOLEXPAND (fallback: identity under host backend)
    for (unsigned i = 0; i < elems && i < kTileWords; ++i)
      out.words[i] = src.words[i];
    break;
  case 0x040u: // TEXP
    for (unsigned i = 0; i < elems && i < kTileWords; ++i) {
      float f = bitCastFromU32<float>(src.words[i]);
      out.words[i] = bitCastToU32<float>(expf(f));
    }
    break;
  case 0x044u: // TRECIP
    for (unsigned i = 0; i < elems && i < kTileWords; ++i) {
      float f = bitCastFromU32<float>(src.words[i]);
      float inv = (f == 0.0f) ? 0.0f : (1.0f / f);
      out.words[i] = bitCastToU32<float>(inv);
    }
    break;
  default:
    // Unsupported op in host backend: keep destination zeroed.
    break;
  }
  return out;
}

template <unsigned TileOp10>
inline RawTile teplBinaryHost(const RawTile &lhs, const RawTile &rhs, unsigned elems) {
  RawTile out{};
  for (unsigned i = 0; i < kTileWords; ++i)
    out.words[i] = 0u;

  switch (TileOp10 & 0x3ffu) {
  case 0x000u: // TADD
    for (unsigned i = 0; i < elems && i < kTileWords; ++i)
      out.words[i] = lhs.words[i] + rhs.words[i];
    break;
  case 0x001u: // TSUB
    for (unsigned i = 0; i < elems && i < kTileWords; ++i)
      out.words[i] = lhs.words[i] - rhs.words[i];
    break;
  case 0x002u: { // TMUL
    for (unsigned i = 0; i < elems && i < kTileWords; ++i) {
      float a = bitCastFromU32<float>(lhs.words[i]);
      float b = bitCastFromU32<float>(rhs.words[i]);
      out.words[i] = bitCastToU32<float>(a * b);
    }
    break;
  }
  case 0x004u: { // TMAX
    for (unsigned i = 0; i < elems && i < kTileWords; ++i) {
      float a = bitCastFromU32<float>(lhs.words[i]);
      float b = bitCastFromU32<float>(rhs.words[i]);
      out.words[i] = bitCastToU32<float>(a > b ? a : b);
    }
    break;
  }
  default:
    break;
  }
  return out;
}

#endif

template <unsigned SizeCode, unsigned DType, long long Layout, long long LB0,
          long long LB1, long long StrideBytes>
inline RawTile tileTLoad(const void *base) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
#if defined(PTO_HOST_SIM)
  (void)Layout;
  RawTile out{};
  for (unsigned i = 0; i < kTileWords; ++i)
    out.words[i] = 0u;

  const uint64_t bytes64 = sizeBytesFromCode(SizeCode);
  const unsigned elem_bytes = dtypeElemBytesForStorage(DType);
  const unsigned elem_bits = dtypeElemBits(DType);
  if (bytes64 == 0 || bytes64 > kMaxTileBytes || elem_bits == 0u ||
      (bytes64 % elem_bytes) != 0u)
    return out;

  const unsigned max_elems = dtypeElemCountForBytes(bytes64, DType);
  const uint64_t cols = (LB0 > 0) ? static_cast<uint64_t>(LB0)
                                  : static_cast<uint64_t>(max_elems);
  const uint64_t rows = (LB1 > 0) ? static_cast<uint64_t>(LB1) : 1u;
  if (rows == 0u || cols == 0u)
    return out;
  if (rows > (UINT64_MAX / cols))
    return out;
  const uint64_t active = rows * cols;
  if (active > max_elems)
    return out;

  const uint64_t row_span_bits = cols * elem_bits;
  const uint64_t row_span_bytes = (row_span_bits + 7u) / 8u;
  const uint64_t stride_bytes =
      (StrideBytes > 0) ? static_cast<uint64_t>(StrideBytes) : row_span_bytes;
  if (stride_bytes < row_span_bytes ||
      (elem_bytes != 0u && (stride_bytes % elem_bytes) != 0u)) {
    return out;
  }

  const uint8_t *src = reinterpret_cast<const uint8_t *>(base);
  for (uint64_t r = 0; r < rows; ++r) {
    const uint64_t row_base = r * stride_bytes;
    for (uint64_t c = 0; c < cols; ++c) {
      const uint64_t idx64 = r * cols + c;
      if (idx64 >= kTileWords)
        return out;
      const unsigned idx = static_cast<unsigned>(idx64);

      uint32_t value = 0u;
      if (elem_bits == 4u) {
        const uint64_t byte_addr = row_base + (c >> 1u);
        const uint8_t packed = src[byte_addr];
        value = ((c & 1u) == 0u) ? (packed & 0x0fu) : ((packed >> 4u) & 0x0fu);
      } else if (elem_bytes == 1u) {
        value = static_cast<uint32_t>(src[row_base + c]);
      } else if (elem_bytes == 2u) {
        uint16_t v = 0u;
        memcpy(&v, src + row_base + c * 2u, sizeof(v));
        value = static_cast<uint32_t>(v);
      } else if (elem_bytes == 4u) {
        uint32_t v = 0u;
        memcpy(&v, src + row_base + c * 4u, sizeof(v));
        value = v;
      } else if (elem_bytes == 8u) {
        uint64_t v = 0u;
        memcpy(&v, src + row_base + c * 8u, sizeof(v));
        value = static_cast<uint32_t>(v & 0xffffffffu);
      } else {
        return out;
      }
      out.words[idx] = value;
    }
  }
  return out;
#else
  return __builtin_linx_tile_tload(base, SizeCode, DType, Layout, LB0, LB1,
                                   StrideBytes);
#endif
}

template <unsigned SizeCode, unsigned DType, long long Layout, long long LB0,
          long long LB1, long long StrideBytes>
inline void tileTStore(void *base, RawTile tile) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
#if defined(PTO_HOST_SIM)
  (void)Layout;
  const uint64_t bytes64 = sizeBytesFromCode(SizeCode);
  const unsigned elem_bytes = dtypeElemBytesForStorage(DType);
  const unsigned elem_bits = dtypeElemBits(DType);
  if (bytes64 == 0 || bytes64 > kMaxTileBytes || elem_bits == 0u ||
      (bytes64 % elem_bytes) != 0u)
    return;

  const unsigned max_elems = dtypeElemCountForBytes(bytes64, DType);
  const uint64_t cols = (LB0 > 0) ? static_cast<uint64_t>(LB0)
                                  : static_cast<uint64_t>(max_elems);
  const uint64_t rows = (LB1 > 0) ? static_cast<uint64_t>(LB1) : 1u;
  if (rows == 0u || cols == 0u)
    return;
  if (rows > (UINT64_MAX / cols))
    return;
  const uint64_t active = rows * cols;
  if (active > max_elems)
    return;

  const uint64_t row_span_bits = cols * elem_bits;
  const uint64_t row_span_bytes = (row_span_bits + 7u) / 8u;
  const uint64_t stride_bytes =
      (StrideBytes > 0) ? static_cast<uint64_t>(StrideBytes) : row_span_bytes;
  if (stride_bytes < row_span_bytes ||
      (elem_bytes != 0u && (stride_bytes % elem_bytes) != 0u)) {
    return;
  }

  uint8_t *dst = reinterpret_cast<uint8_t *>(base);
  for (uint64_t r = 0; r < rows; ++r) {
    const uint64_t row_base = r * stride_bytes;
    for (uint64_t c = 0; c < cols; ++c) {
      const uint64_t idx64 = r * cols + c;
      if (idx64 >= kTileWords)
        return;
      const uint32_t value = tile.words[static_cast<unsigned>(idx64)];

      if (elem_bits == 4u) {
        const uint64_t byte_addr = row_base + (c >> 1u);
        uint8_t packed = dst[byte_addr];
        const uint8_t nibble = static_cast<uint8_t>(value & 0x0fu);
        if ((c & 1u) == 0u)
          packed = static_cast<uint8_t>((packed & 0xf0u) | nibble);
        else
          packed = static_cast<uint8_t>((packed & 0x0fu) | (nibble << 4u));
        dst[byte_addr] = packed;
      } else if (elem_bytes == 1u) {
        dst[row_base + c] = static_cast<uint8_t>(value & 0xffu);
      } else if (elem_bytes == 2u) {
        const uint16_t v = static_cast<uint16_t>(value & 0xffffu);
        memcpy(dst + row_base + c * 2u, &v, sizeof(v));
      } else if (elem_bytes == 4u) {
        memcpy(dst + row_base + c * 4u, &value, sizeof(value));
      } else if (elem_bytes == 8u) {
        const uint64_t v = static_cast<uint64_t>(value);
        memcpy(dst + row_base + c * 8u, &v, sizeof(v));
      } else {
        return;
      }
    }
  }
#else
  __builtin_linx_tile_tstore(base, tile, SizeCode, DType, Layout, LB0, LB1,
                             StrideBytes);
#endif
}

template <unsigned M, unsigned N, unsigned K>
inline RawTile cubeMamulb(RawTile lhs, RawTile rhs) {
  static_assert(M <= 0xffu && N <= 0xffu && K <= 0xffu,
                "PTO Linx strict-v0.3: cube dimensions must fit u8");
#if defined(PTO_HOST_SIM)
  RawTile out{};
  for (unsigned i = 0; i < kTileWords; ++i)
    out.words[i] = 0u;

  for (unsigned i = 0; i < M; ++i) {
    for (unsigned j = 0; j < N; ++j) {
      int64_t acc = 0;
      for (unsigned k = 0; k < K; ++k) {
        const unsigned a_idx = i * K + k;
        const unsigned b_idx = k * N + j;
        if (a_idx >= kTileWords || b_idx >= kTileWords)
          continue;
        const int32_t a = static_cast<int32_t>(lhs.words[a_idx]);
        const int32_t b = static_cast<int32_t>(rhs.words[b_idx]);
        acc += static_cast<int64_t>(a) * static_cast<int64_t>(b);
      }
      const unsigned out_idx = i * N + j;
      if (out_idx < kTileWords)
        out.words[out_idx] = static_cast<uint32_t>(static_cast<int32_t>(acc));
    }
  }
  return out;
#else
  return __builtin_linx_cube_mamulb(lhs, rhs, M, N, K);
#endif
}

template <unsigned M, unsigned N, unsigned K>
inline RawTile cubeMamulbAcc(RawTile acc, RawTile lhs, RawTile rhs) {
  static_assert(M <= 0xffu && N <= 0xffu && K <= 0xffu,
                "PTO Linx strict-v0.3: cube dimensions must fit u8");
#if defined(PTO_HOST_SIM)
  RawTile out = acc;
  for (unsigned i = 0; i < M; ++i) {
    for (unsigned j = 0; j < N; ++j) {
      const unsigned out_idx = i * N + j;
      int64_t sum = (out_idx < kTileWords)
                        ? static_cast<int32_t>(out.words[out_idx])
                        : 0;
      for (unsigned k = 0; k < K; ++k) {
        const unsigned a_idx = i * K + k;
        const unsigned b_idx = k * N + j;
        if (a_idx >= kTileWords || b_idx >= kTileWords)
          continue;
        const int32_t a = static_cast<int32_t>(lhs.words[a_idx]);
        const int32_t b = static_cast<int32_t>(rhs.words[b_idx]);
        sum += static_cast<int64_t>(a) * static_cast<int64_t>(b);
      }
      if (out_idx < kTileWords)
        out.words[out_idx] = static_cast<uint32_t>(static_cast<int32_t>(sum));
    }
  }
  return out;
#else
  return __builtin_linx_cube_mamulb_acc(acc, lhs, rhs, M, N, K);
#endif
}

template <unsigned TileOp10, unsigned SizeCode, unsigned DType>
inline RawTile teplUnary(RawTile src) {
  static_assert(TileOp10 <= 0x3ffu,
                "PTO Linx strict-v0.3: TEPL tileop10 must fit u10");
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
#if defined(PTO_HOST_SIM)
  const uint64_t bytes64 = sizeBytesFromCode(SizeCode);
  const unsigned elem_bytes = dtypeElemBytesForStorage(DType);
  const unsigned elems =
      (bytes64 == 0 || bytes64 > kMaxTileBytes || elem_bytes == 0)
          ? 0u
          : dtypeElemCountForBytes(bytes64, DType);
  return teplUnaryHost<TileOp10>(src, elems);
#else
  return __builtin_linx_tepl_unary(src, TileOp10, SizeCode, DType);
#endif
}

template <unsigned TileOp10, unsigned SizeCode, unsigned DType>
inline RawTile teplBinary(RawTile lhs, RawTile rhs) {
  static_assert(TileOp10 <= 0x3ffu,
                "PTO Linx strict-v0.3: TEPL tileop10 must fit u10");
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
#if defined(PTO_HOST_SIM)
  const uint64_t bytes64 = sizeBytesFromCode(SizeCode);
  const unsigned elem_bytes = dtypeElemBytesForStorage(DType);
  const unsigned elems =
      (bytes64 == 0 || bytes64 > kMaxTileBytes || elem_bytes == 0)
          ? 0u
          : dtypeElemCountForBytes(bytes64, DType);
  return teplBinaryHost<TileOp10>(lhs, rhs, elems);
#else
  return __builtin_linx_tepl_binary(lhs, rhs, TileOp10, SizeCode, DType);
#endif
}

template <unsigned TileOp10, unsigned SizeCode, unsigned DType, unsigned Mode,
          typename Scalar>
inline RawTile teplBinaryScalar(RawTile lhs, Scalar scalar) {
  static_assert(TileOp10 <= 0x3ffu,
                "PTO Linx strict-v0.3: TEPL tileop10 must fit u10");
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
  static_assert(Mode == 1u,
                "PTO Linx strict-v0.3: tepl.binary.scalar requires mode=VS(1)");
#if defined(PTO_HOST_SIM)
  RawTile rhs{};
  const uint64_t bytes64 = sizeBytesFromCode(SizeCode);
  const unsigned elem_bytes = dtypeElemBytesForStorage(DType);
  const unsigned elems =
      (bytes64 == 0 || bytes64 > kMaxTileBytes || elem_bytes == 0)
          ? 0u
          : dtypeElemCountForBytes(bytes64, DType);
  const long long bits = encodeScalar(scalar);
  const uint32_t scalar_word = static_cast<uint32_t>(bits & 0xffffffffull);
  for (unsigned i = 0; i < elems && i < kTileWords; ++i)
    rhs.words[i] = scalar_word;
  return teplBinaryHost<TileOp10>(lhs, rhs, elems);
#else
  return __builtin_linx_tepl_binary_scalar(lhs, encodeScalar(scalar), TileOp10,
                                           SizeCode, DType, Mode);
#endif
}

template <unsigned TileOp10, unsigned SizeCode, unsigned DType, unsigned Mode,
          typename Scalar>
inline RawTile teplSplat(Scalar scalar) {
  static_assert(TileOp10 <= 0x3ffu,
                "PTO Linx strict-v0.3: TEPL tileop10 must fit u10");
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
  static_assert(Mode == 2u,
                "PTO Linx strict-v0.3: tepl.splat requires mode=SV(2)");
#if defined(PTO_HOST_SIM)
  RawTile out{};
  for (unsigned i = 0; i < kTileWords; ++i)
    out.words[i] = 0u;

  const uint64_t bytes64 = sizeBytesFromCode(SizeCode);
  const unsigned elem_bytes = dtypeElemBytesForStorage(DType);
  const unsigned elems =
      (bytes64 == 0 || bytes64 > kMaxTileBytes || elem_bytes == 0)
          ? 0u
          : dtypeElemCountForBytes(bytes64, DType);

  if ((TileOp10 & 0x3ffu) != 0x045u)
    return out;

  const long long bits = encodeScalar(scalar);
  const uint32_t scalar_word = static_cast<uint32_t>(bits & 0xffffffffull);
  for (unsigned i = 0; i < elems && i < kTileWords; ++i)
    out.words[i] = scalar_word;
  return out;
#else
  return __builtin_linx_tepl_splat(encodeScalar(scalar), TileOp10, SizeCode,
                                   DType, Mode);
#endif
}

template <unsigned SizeCode, unsigned DType, long long Layout, unsigned HasLayout,
          unsigned Mode>
inline RawTile tileTMov(RawTile src) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "PTO Linx strict-v0.3: size_code must be in [5,8]");
  static_assert(HasLayout <= 1u, "PTO Linx strict-v0.3: has_layout must be bool");
  static_assert(Mode <= 1u,
                "PTO Linx strict-v0.3: tmov mode must be 0(V2V) or 1(A2V)");
#if defined(PTO_HOST_SIM)
  (void)DType;
  (void)Layout;
  (void)HasLayout;
  (void)Mode;
  return src;
#else
  return __builtin_linx_tile_tmov(src, Mode, SizeCode, DType, Layout, HasLayout);
#endif
}

} // namespace detail
} // namespace linx
} // namespace pto

#endif // PTO_LINX_IMPL_BACKEND_HPP
