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

extern "C" void pto_gemm_auto_i32(const int *lhs, const int *rhs, int *dst) {
  auto a0 = load_tile<tile_i32_8x8>(lhs, 0);
  auto a1 = load_tile<tile_i32_8x8>(lhs, 1);
  auto a2 = load_tile<tile_i32_8x8>(lhs, 2);
  auto a3 = load_tile<tile_i32_8x8>(lhs, 3);
  auto a4 = load_tile<tile_i32_8x8>(lhs, 4);
  auto a5 = load_tile<tile_i32_8x8>(lhs, 5);
  auto a6 = load_tile<tile_i32_8x8>(lhs, 6);
  auto a7 = load_tile<tile_i32_8x8>(lhs, 7);
  auto a8 = load_tile<tile_i32_8x8>(lhs, 8);

  auto b0 = load_tile<tile_i32_8x8>(rhs, 0);
  auto b1 = load_tile<tile_i32_8x8>(rhs, 1);
  auto b2 = load_tile<tile_i32_8x8>(rhs, 2);
  auto b3 = load_tile<tile_i32_8x8>(rhs, 3);
  auto b4 = load_tile<tile_i32_8x8>(rhs, 4);
  auto b5 = load_tile<tile_i32_8x8>(rhs, 5);
  auto b6 = load_tile<tile_i32_8x8>(rhs, 6);
  auto b7 = load_tile<tile_i32_8x8>(rhs, 7);

  tile_i32_8x8 c0;
  tile_i32_8x8 c1;
  tile_i32_8x8 c2;
  tile_i32_8x8 c3;
  tile_i32_8x8 c4;
  tile_i32_8x8 c5;
  tile_i32_8x8 c6;
  tile_i32_8x8 c7;
  tile_i32_8x8 c8;
  tile_i32_8x8 c9;
  tile_i32_8x8 c10;
  TMATMUL(c0, a0, b0);
  TMATMUL(c1, a1, b1);
  TMATMUL(c2, a2, b2);
  TMATMUL(c3, a3, b3);
  TMATMUL(c4, a4, b4);
  TMATMUL(c5, a5, b5);
  TMATMUL(c6, a6, b6);
  TMATMUL(c7, a7, b0);
  TMATMUL(c8, a8, b1);
  TMATMUL(c9, a0, b2);
  TMATMUL(c10, a1, b7);

  store_tile(dst, 0, c0);
  store_tile(dst, 1, c1);
  store_tile(dst, 2, c2);
  store_tile(dst, 3, c3);
  store_tile(dst, 4, c4);
  store_tile(dst, 5, c5);
  store_tile(dst, 6, c6);
  store_tile(dst, 7, c7);
  store_tile(dst, 8, c8);
  store_tile(dst, 9, c9);
  store_tile(dst, 10, c10);
}
