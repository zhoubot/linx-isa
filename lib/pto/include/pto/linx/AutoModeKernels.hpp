#ifndef PTO_LINX_AUTO_MODE_KERNELS_HPP
#define PTO_LINX_AUTO_MODE_KERNELS_HPP

#include <stdint.h>

#include <pto/linx/TileOps.hpp>

namespace pto {
namespace linx {
namespace auto_mode {

constexpr unsigned kTileElemsI32 = 1024u;
constexpr unsigned kFullTileSizeCode = 8u;

inline void gemm_kernel_i32(const int32_t *a, const int32_t *b, int32_t *out) {
  auto a0 = tload<kFullTileSizeCode>(a + 0u * kTileElemsI32);
  auto a1 = tload<kFullTileSizeCode>(a + 1u * kTileElemsI32);
  auto a2 = tload<kFullTileSizeCode>(a + 2u * kTileElemsI32);
  auto a3 = tload<kFullTileSizeCode>(a + 3u * kTileElemsI32);
  auto a4 = tload<kFullTileSizeCode>(a + 4u * kTileElemsI32);
  auto a5 = tload<kFullTileSizeCode>(a + 5u * kTileElemsI32);
  auto a6 = tload<kFullTileSizeCode>(a + 6u * kTileElemsI32);
  auto a7 = tload<kFullTileSizeCode>(a + 7u * kTileElemsI32);
  auto a8 = tload<kFullTileSizeCode>(a + 8u * kTileElemsI32);

  auto b0 = tload<kFullTileSizeCode>(b + 0u * kTileElemsI32);
  auto b1 = tload<kFullTileSizeCode>(b + 1u * kTileElemsI32);
  auto b2 = tload<kFullTileSizeCode>(b + 2u * kTileElemsI32);
  auto b3 = tload<kFullTileSizeCode>(b + 3u * kTileElemsI32);
  auto b4 = tload<kFullTileSizeCode>(b + 4u * kTileElemsI32);
  auto b5 = tload<kFullTileSizeCode>(b + 5u * kTileElemsI32);
  auto b6 = tload<kFullTileSizeCode>(b + 6u * kTileElemsI32);
  auto b7 = tload<kFullTileSizeCode>(b + 7u * kTileElemsI32);

  auto c0 = tmatmul<8, 8, 8>(a0, b0);
  auto c1 = tmatmul<8, 8, 8>(a1, b1);
  auto c2 = tmatmul<8, 8, 8>(a2, b2);
  auto c3 = tmatmul<8, 8, 8>(a3, b3);
  auto c4 = tmatmul<8, 8, 8>(a4, b4);
  auto c5 = tmatmul<8, 8, 8>(a5, b5);
  auto c6 = tmatmul<8, 8, 8>(a6, b6);
  auto c7 = tmatmul<8, 8, 8>(a7, b0);
  auto c8 = tmatmul<8, 8, 8>(a8, b1);
  auto c9 = tmatmul<8, 8, 8>(a0, b2);
  auto c10 = tmatmul<8, 8, 8>(a1, b7);

  tstore<kFullTileSizeCode>(out + 0u * kTileElemsI32, c0);
  tstore<kFullTileSizeCode>(out + 1u * kTileElemsI32, c1);
  tstore<kFullTileSizeCode>(out + 2u * kTileElemsI32, c2);
  tstore<kFullTileSizeCode>(out + 3u * kTileElemsI32, c3);
  tstore<kFullTileSizeCode>(out + 4u * kTileElemsI32, c4);
  tstore<kFullTileSizeCode>(out + 5u * kTileElemsI32, c5);
  tstore<kFullTileSizeCode>(out + 6u * kTileElemsI32, c6);
  tstore<kFullTileSizeCode>(out + 7u * kTileElemsI32, c7);
  tstore<kFullTileSizeCode>(out + 8u * kTileElemsI32, c8);
  tstore<kFullTileSizeCode>(out + 9u * kTileElemsI32, c9);
  tstore<kFullTileSizeCode>(out + 10u * kTileElemsI32, c10);
}

inline void flash_attention_kernel_i32(const int32_t *q, const int32_t *k,
                                       const int32_t *v, int32_t *out) {
  auto q0 = tload<kFullTileSizeCode>(q + 0u * kTileElemsI32);
  auto q1 = tload<kFullTileSizeCode>(q + 1u * kTileElemsI32);
  auto q2 = tload<kFullTileSizeCode>(q + 2u * kTileElemsI32);
  auto q3 = tload<kFullTileSizeCode>(q + 3u * kTileElemsI32);
  auto q4 = tload<kFullTileSizeCode>(q + 4u * kTileElemsI32);

  auto k0 = tload<kFullTileSizeCode>(k + 0u * kTileElemsI32);
  auto k1 = tload<kFullTileSizeCode>(k + 1u * kTileElemsI32);
  auto k2 = tload<kFullTileSizeCode>(k + 2u * kTileElemsI32);
  auto k3 = tload<kFullTileSizeCode>(k + 3u * kTileElemsI32);
  auto k4 = tload<kFullTileSizeCode>(k + 4u * kTileElemsI32);

  auto v0 = tload<kFullTileSizeCode>(v + 0u * kTileElemsI32);
  auto v1 = tload<kFullTileSizeCode>(v + 1u * kTileElemsI32);
  auto v2 = tload<kFullTileSizeCode>(v + 2u * kTileElemsI32);
  auto v3 = tload<kFullTileSizeCode>(v + 3u * kTileElemsI32);

  auto s0 = tmatmul<8, 8, 8>(q0, k0);
  auto s1 = tmatmul<8, 8, 8>(q1, k1);
  auto s2 = tmatmul<8, 8, 8>(q2, k2);
  auto s3 = tmatmul<8, 8, 8>(q3, k3);
  auto s4 = tmatmul<8, 8, 8>(q4, k4);
  auto s5 = tmatmul<8, 8, 8>(q0, k1);
  auto s6 = tmatmul<8, 8, 8>(q1, k2);
  auto s7 = tmatmul<8, 8, 8>(q2, k3);
  auto s8 = tmatmul<8, 8, 8>(q3, k4);

  tstore<kFullTileSizeCode>(out + 0u * kTileElemsI32, tmatmul<8, 8, 8>(s0, v0));
  tstore<kFullTileSizeCode>(out + 1u * kTileElemsI32, tmatmul<8, 8, 8>(s1, v1));
  tstore<kFullTileSizeCode>(out + 2u * kTileElemsI32, tmatmul<8, 8, 8>(s2, v2));
  tstore<kFullTileSizeCode>(out + 3u * kTileElemsI32, tmatmul<8, 8, 8>(s3, v3));
  tstore<kFullTileSizeCode>(out + 4u * kTileElemsI32, tmatmul<8, 8, 8>(s4, v0));
  tstore<kFullTileSizeCode>(out + 5u * kTileElemsI32, tmatmul<8, 8, 8>(s5, v1));
  tstore<kFullTileSizeCode>(out + 6u * kTileElemsI32, tmatmul<8, 8, 8>(s6, v2));
  tstore<kFullTileSizeCode>(out + 7u * kTileElemsI32, tmatmul<8, 8, 8>(s7, v3));
  tstore<kFullTileSizeCode>(out + 8u * kTileElemsI32, tmatmul<8, 8, 8>(s8, v0));
}

} // namespace auto_mode
} // namespace linx
} // namespace pto

#endif // PTO_LINX_AUTO_MODE_KERNELS_HPP
