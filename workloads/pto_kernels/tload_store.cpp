#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

constexpr int kRows = PTO_QEMU_SMOKE ? 32 : 1024;
constexpr int kCols = PTO_QEMU_SMOKE ? 32 : 1024;
using tile_vec_i32 = Tile<Location::Vec, int, 32, 32, BLayout::RowMajor>;

static_assert(tile_vec_i32::Rows * tile_vec_i32::Cols *
                      static_cast<int>(sizeof(int)) ==
                  4096,
              "tile must be exactly 4KB");
static_assert(kRows % tile_vec_i32::Rows == 0 &&
                  kCols % tile_vec_i32::Cols == 0,
              "global tensor must be divisible by tile shape");

using gmSrc = global_tensor<int, RowMajor<kRows, kCols>>;
using gmDst = global_tensor<int, RowMajor<kRows, kCols>>;

using itSrc = global_iterator<gmSrc, tile_vec_i32>;
using itDst = global_iterator<gmDst, tile_vec_i32>;

} // namespace

extern "C" void tload_store_i32(int *src_ptr, int *dst_ptr) {
  itSrc gSrc(src_ptr);
  itDst gDst(dst_ptr);

  constexpr int kRowTiles = kRows / tile_vec_i32::Rows;
  constexpr int kColTiles = kCols / tile_vec_i32::Cols;

  for (int tr = 0; tr < kRowTiles; ++tr) {
    for (int tc = 0; tc < kColTiles; ++tc) {
      tile_vec_i32 tile;
      TLOAD(tile, gSrc(tr, tc));
      TSTORE(gDst(tr, tc), tile);
    }
  }
}
