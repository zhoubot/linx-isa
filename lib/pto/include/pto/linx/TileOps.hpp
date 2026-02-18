#ifndef PTO_LINX_TILEOPS_HPP
#define PTO_LINX_TILEOPS_HPP

#include <common/pto_tileop.hpp>

namespace pto {
namespace linx {

using TileI32 = detail::RawTile;

#if defined(__GNUC__) || defined(__clang__)
#define PTO_LINX_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define PTO_LINX_ALWAYS_INLINE inline
#endif

enum : unsigned {
  kTMAFmtNorm = 0u,
  kTMAFmtND2NZ = 1u,
  kTMAFmtND2ZN = 2u,
  kTMAFmtDN2NZ = 3u,
  kTMAFmtDN2ZN = 4u,
};

enum : unsigned {
  kTMAPadNull = 0u,
  kTMAPadZero = 1u,
  kTMAPadMax = 2u,
  kTMAPadMin = 3u,
};

constexpr unsigned kTileDTypeInt32 = 17u;

constexpr unsigned make_tma_arg(unsigned format, unsigned pad = kTMAPadNull) {
  return ((pad & 0x3u) << 3u) | (format & 0x7u);
}

template <unsigned SizeCode>
constexpr unsigned sizeCodeBytes() {
  return 1u << (SizeCode + 4u);
}

template <unsigned SizeCode>
constexpr unsigned defaultLB0() {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "size code must be in [5,8]");
  return (SizeCode >= 7u) ? 32u : 16u;
}

template <unsigned SizeCode>
constexpr unsigned defaultLB1() {
  constexpr unsigned ElemBytes = sizeof(int32_t);
  constexpr unsigned Elems = sizeCodeBytes<SizeCode>() / ElemBytes;
  constexpr unsigned LB0 = defaultLB0<SizeCode>();
  static_assert((Elems % LB0) == 0u, "default tile dims must be integral");
  return Elems / LB0;
}

template <unsigned SizeCode, unsigned LB0>
constexpr unsigned effectiveLB0() {
  return LB0 ? LB0 : defaultLB0<SizeCode>();
}

template <unsigned SizeCode, unsigned LB1>
constexpr unsigned effectiveLB1() {
  return LB1 ? LB1 : defaultLB1<SizeCode>();
}

template <unsigned SizeCode, unsigned LB0>
constexpr long long effectiveStrideBytes() {
  return static_cast<long long>(effectiveLB0<SizeCode, LB0>() * sizeof(int32_t));
}

template <unsigned SizeCode, unsigned Arg = 0, unsigned LB0 = 0,
          unsigned LB1 = 0, unsigned LB2 = 0>
PTO_LINX_ALWAYS_INLINE TileI32 tload(const void *base) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "tload size_code must be in [5,8]");
  (void)LB2;
  constexpr unsigned Dim0 = effectiveLB0<SizeCode, LB0>();
  constexpr unsigned Dim1 = effectiveLB1<SizeCode, LB1>();
  constexpr long long Stride = effectiveStrideBytes<SizeCode, LB0>();
  return __builtin_linx_tile_tload(base, SizeCode, kTileDTypeInt32, Arg, Dim0,
                                   Dim1, Stride);
}

template <unsigned SizeCode, unsigned Arg = 0, unsigned LB0 = 0,
          unsigned LB1 = 0, unsigned LB2 = 0>
PTO_LINX_ALWAYS_INLINE void tstore(void *base, TileI32 tile) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "tstore size_code must be in [5,8]");
  (void)LB2;
  constexpr unsigned Dim0 = effectiveLB0<SizeCode, LB0>();
  constexpr unsigned Dim1 = effectiveLB1<SizeCode, LB1>();
  constexpr long long Stride = effectiveStrideBytes<SizeCode, LB0>();
  __builtin_linx_tile_tstore(base, tile, SizeCode, kTileDTypeInt32, Arg, Dim0,
                             Dim1, Stride);
}

template <unsigned M, unsigned N, unsigned K>
PTO_LINX_ALWAYS_INLINE TileI32 mamulb(TileI32 lhs, TileI32 rhs) {
  return __builtin_linx_cube_mamulb(lhs, rhs, M, N, K);
}

template <unsigned M, unsigned N, unsigned K>
PTO_LINX_ALWAYS_INLINE TileI32 tmatmul(TileI32 lhs, TileI32 rhs) {
  return mamulb<M, N, K>(lhs, rhs);
}

template <unsigned M, unsigned N, unsigned K>
PTO_LINX_ALWAYS_INLINE TileI32 tmatmul_acc(TileI32 acc, TileI32 lhs, TileI32 rhs) {
  return __builtin_linx_cube_mamulb_acc(acc, lhs, rhs, M, N, K);
}

template <unsigned M, unsigned N, unsigned K>
PTO_LINX_ALWAYS_INLINE TileI32 tmatmul_mx(TileI32 lhs, TileI32 rhs) {
#if defined(PTO_LINX_ENABLE_TMATMUL_MX) && PTO_LINX_ENABLE_TMATMUL_MX
  return mamulb<M, N, K>(lhs, rhs);
#else
  (void)lhs;
  (void)rhs;
  __builtin_trap();
#endif
}

template <unsigned SizeCode>
PTO_LINX_ALWAYS_INLINE TileI32 tadd(TileI32 lhs, TileI32 rhs) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "tadd size_code must be in [5,8]");
  return __builtin_linx_vpar_tadd(lhs, rhs, SizeCode);
}

template <unsigned SizeCode>
PTO_LINX_ALWAYS_INLINE TileI32 tsub(TileI32 lhs, TileI32 rhs) {
  static_assert(SizeCode >= 5u && SizeCode <= 8u,
                "tsub size_code must be in [5,8]");
  return __builtin_linx_vpar_tsub(lhs, rhs, SizeCode);
}

#undef PTO_LINX_ALWAYS_INLINE

} // namespace linx
} // namespace pto

#endif // PTO_LINX_TILEOPS_HPP
