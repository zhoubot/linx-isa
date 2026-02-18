#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

constexpr int kRows = PTO_QEMU_SMOKE ? 32 : 1024;
constexpr int kCols = PTO_QEMU_SMOKE ? 32 : 1024;
using tile_vec_f32 = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;

static_assert(tile_vec_f32::Rows * tile_vec_f32::Cols *
                      static_cast<int>(sizeof(float)) ==
                  4096,
              "tile must be exactly 4KB");
static_assert(kRows % tile_vec_f32::Rows == 0 &&
                  kCols % tile_vec_f32::Cols == 0,
              "global tensor must be divisible by tile shape");

using gmX = global_tensor<float, RowMajor<kRows, kCols>>;
using gmY = global_tensor<float, RowMajor<kRows, kCols>>;
using gmZ = global_tensor<float, RowMajor<kRows, kCols>>;

using itX = global_iterator<gmX, tile_vec_f32>;
using itY = global_iterator<gmY, tile_vec_f32>;
using itZ = global_iterator<gmZ, tile_vec_f32>;

} // namespace

extern "C" void add_custom_f32(float *x_ptr, float *y_ptr, float *z_ptr) {
  itX gX(x_ptr);
  itY gY(y_ptr);
  itZ gZ(z_ptr);

  constexpr int kRowTiles = kRows / tile_vec_f32::Rows;
  constexpr int kColTiles = kCols / tile_vec_f32::Cols;

  for (int tr = 0; tr < kRowTiles; ++tr) {
    for (int tc = 0; tc < kColTiles; ++tc) {
      tile_vec_f32 tx;
      tile_vec_f32 ty;
      tile_vec_f32 tz;
      TLOAD(tx, gX(tr, tc));
      TLOAD(ty, gY(tr, tc));
      TADD(tz, tx, ty);
      TSTORE(gZ(tr, tc), tz);
    }
  }
}
