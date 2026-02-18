#include <common/pto_tileop.hpp>

using namespace pto;

extern "C" void pto_tload_store_i32(const int *src, int *dst) {
  using tile_full_i32 = Tile<Location::Vec, int, 32, 32, BLayout::RowMajor>;

  tile_full_i32 tile;
  TLOAD(tile, src);
  TSTORE(dst, tile);
}
