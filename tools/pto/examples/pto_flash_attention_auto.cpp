#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

constexpr int kTileElemsI32 = 1024;
using tile_i32_8x8 = Tile<Location::Vec, int, 8, 8, BLayout::RowMajor>;

template <typename TileT>
inline TileT load_tile(const int *base, int idx) {
  TileT t;
  TLOAD(t, base + idx * kTileElemsI32);
  return t;
}

template <typename TileT>
inline void store_tile(int *base, int idx, TileT &tile) {
  TSTORE(base + idx * kTileElemsI32, tile);
}

} // namespace

extern "C" void pto_flash_attention_auto_i32(const int *query, const int *key,
                                             const int *value, int *dst) {
  auto q0 = load_tile<tile_i32_8x8>(query, 0);
  auto q1 = load_tile<tile_i32_8x8>(query, 1);
  auto q2 = load_tile<tile_i32_8x8>(query, 2);
  auto q3 = load_tile<tile_i32_8x8>(query, 3);
  auto q4 = load_tile<tile_i32_8x8>(query, 4);

  auto k0 = load_tile<tile_i32_8x8>(key, 0);
  auto k1 = load_tile<tile_i32_8x8>(key, 1);
  auto k2 = load_tile<tile_i32_8x8>(key, 2);
  auto k3 = load_tile<tile_i32_8x8>(key, 3);
  auto k4 = load_tile<tile_i32_8x8>(key, 4);

  auto v0 = load_tile<tile_i32_8x8>(value, 0);
  auto v1 = load_tile<tile_i32_8x8>(value, 1);
  auto v2 = load_tile<tile_i32_8x8>(value, 2);
  auto v3 = load_tile<tile_i32_8x8>(value, 3);

  tile_i32_8x8 s0;
  tile_i32_8x8 s1;
  tile_i32_8x8 s2;
  tile_i32_8x8 s3;
  tile_i32_8x8 s4;
  tile_i32_8x8 s5;
  tile_i32_8x8 s6;
  tile_i32_8x8 s7;
  tile_i32_8x8 s8;
  TMATMUL(s0, q0, k0);
  TMATMUL(s1, q1, k1);
  TMATMUL(s2, q2, k2);
  TMATMUL(s3, q3, k3);
  TMATMUL(s4, q4, k4);
  TMATMUL(s5, q0, k1);
  TMATMUL(s6, q1, k2);
  TMATMUL(s7, q2, k3);
  TMATMUL(s8, q3, k4);

  tile_i32_8x8 o0;
  tile_i32_8x8 o1;
  tile_i32_8x8 o2;
  tile_i32_8x8 o3;
  tile_i32_8x8 o4;
  tile_i32_8x8 o5;
  tile_i32_8x8 o6;
  tile_i32_8x8 o7;
  tile_i32_8x8 o8;
  TMATMUL(o0, s0, v0);
  TMATMUL(o1, s1, v1);
  TMATMUL(o2, s2, v2);
  TMATMUL(o3, s3, v3);
  TMATMUL(o4, s4, v0);
  TMATMUL(o5, s5, v1);
  TMATMUL(o6, s6, v2);
  TMATMUL(o7, s7, v3);
  TMATMUL(o8, s8, v0);

  store_tile(dst, 0, o0);
  store_tile(dst, 1, o1);
  store_tile(dst, 2, o2);
  store_tile(dst, 3, o3);
  store_tile(dst, 4, o4);
  store_tile(dst, 5, o5);
  store_tile(dst, 6, o6);
  store_tile(dst, 7, o7);
  store_tile(dst, 8, o8);
}
