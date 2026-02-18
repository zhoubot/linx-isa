#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

constexpr int kS = PTO_QEMU_SMOKE ? 16 : 256;
constexpr int kQD = 4;
constexpr int kVD = 16;
constexpr int kTm = 16;
constexpr int kTk = 4;

static_assert(kTm * kTk * kQD * static_cast<int>(sizeof(int)) <= 4096,
              "QK matmul footprint must fit <=4KB");
static_assert(kTm * kVD * kTk * static_cast<int>(sizeof(int)) <= 4096,
              "WV matmul footprint must fit <=4KB");
static_assert(kS % kTm == 0 && kS % kTk == 0,
              "global sequence shape must be divisible by tile shape");

using gmQ = global_tensor<int, RowMajor<kS, kQD>>;
using gmK = global_tensor<int, ColMajor<kQD, kS>>;
using gmV = global_tensor<int, ColMajor<kS, kVD>>;
using gmO = global_tensor<int, RowMajor<kS, kVD>>;

using tileQ = TileLeft<int, kTm, kQD>;
using tileK = TileRight<int, kQD, kTk>;
using tileV = TileRight<int, kTk, kVD>;
using tileScoreAcc = TileAcc<int, kTm, kTk>;
using tileScoreVec = Tile<Location::Vec, int, kTm, kTk, BLayout::RowMajor>;
using tileScoreLeft = TileLeft<int, kTm, kTk>;
using tileOutAcc = TileAcc<int, kTm, kVD>;
using tileOutVec = Tile<Location::Vec, int, kTm, kVD, BLayout::RowMajor>;

using itQ = global_iterator<gmQ, tileQ>;
using itK = global_iterator<gmK, tileK>;
using itV = global_iterator<gmV, tileV>;
using itO = global_iterator<gmO, tileOutVec>;

} // namespace

extern "C" void flash_attention_i32(int *q_ptr, int *k_ptr, int *v_ptr,
                                       int *out_ptr) {
  itQ gQ(q_ptr);
  itK gK(k_ptr);
  itV gV(v_ptr);
  itO gO(out_ptr);

  constexpr int kQTiles = kS / kTm;
  constexpr int kKTiles = kS / kTk;

  for (int qi = 0; qi < kQTiles; ++qi) {
    tileQ q;
    TLOAD(q, gQ(qi, 0));

    tileK k0;
    tileV v0;
    TLOAD(k0, gK(0, 0));
    TLOAD(v0, gV(0, 0));

    tileScoreAcc sAcc0;
    tileScoreVec sVec0;
    tileScoreLeft sLeft0;
    TMATMUL(sAcc0, q, k0);
    TCVT(sVec0, sAcc0);
    TCVT(sLeft0, sVec0);

    tileOutAcc outAcc;
    TMATMUL(outAcc, sLeft0, v0);

    for (int kj = 1; kj < kKTiles; ++kj) {
      tileK k;
      tileV v;
      TLOAD(k, gK(0, kj));
      TLOAD(v, gV(kj, 0));

      tileScoreAcc sAcc;
      tileScoreVec sVec;
      tileScoreLeft sLeft;
      TMATMUL(sAcc, q, k);
      TCVT(sVec, sAcc);
      TCVT(sLeft, sVec);

      tileOutAcc pieceAcc;
      tileOutVec outVec;
      tileOutVec pieceVec;
      tileOutVec merged;
      TMATMUL(pieceAcc, sLeft, v);
      TCVT(outVec, outAcc);
      TCVT(pieceVec, pieceAcc);
      TADD(merged, outVec, pieceVec);
      TCVT(outAcc, merged);
    }

    tileOutVec out;
    TCVT(out, outAcc);
    TSTORE(gO(qi, 0), out);
  }
}
