#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

constexpr int kS = PTO_QEMU_SMOKE ? 16 : 256;
constexpr int kD = 16;
constexpr int kLat = 4;
constexpr int kOut = 16;

constexpr int kTm = 16;
constexpr int kTk = 4;
constexpr float kScale = 0.125f;

static_assert(kTm * kLat * kTk * static_cast<int>(sizeof(float)) <= 4096,
              "projection footprint must fit <=4KB");
static_assert(kTm * kOut * kLat * static_cast<int>(sizeof(float)) <= 4096,
              "output projection footprint must fit <=4KB");
static_assert(kS % kTm == 0 && kD % kTk == 0,
              "global shape must be divisible by tile shape");

using gmQ = global_tensor<float, RowMajor<kS, kD>>;
using gmK = global_tensor<float, RowMajor<kS, kD>>;
using gmV = global_tensor<float, RowMajor<kS, kD>>;
using gmWq = global_tensor<float, ColMajor<kD, kLat>>;
using gmWk = global_tensor<float, ColMajor<kD, kLat>>;
using gmWv = global_tensor<float, ColMajor<kD, kLat>>;
using gmWo = global_tensor<float, ColMajor<kLat, kOut>>;
using gmO = global_tensor<float, RowMajor<kS, kOut>>;

using tileIn = TileLeft<float, kTm, kTk>;
using tileProjW = TileRight<float, kTk, kLat>;
using tileProjAcc = TileAcc<float, kTm, kLat>;
using tileProjVec = Tile<Location::Vec, float, kTm, kLat, BLayout::RowMajor>;
using tileProjLeft = TileLeft<float, kTm, kLat>;
using tileProjRight = TileRight<float, kLat, kLat>;

using tileCtxAcc = TileAcc<float, kTm, kLat>;
using tileCtxVec = Tile<Location::Vec, float, kTm, kLat, BLayout::RowMajor>;
using tileCtxLeft = TileLeft<float, kTm, kLat>;

using tileWo = TileRight<float, kLat, kOut>;
using tileOutAcc = TileAcc<float, kTm, kOut>;
using tileOutVec = Tile<Location::Vec, float, kTm, kOut, BLayout::RowMajor>;

using itQ = global_iterator<gmQ, tileIn>;
using itK = global_iterator<gmK, tileIn>;
using itV = global_iterator<gmV, tileIn>;
using itWq = global_iterator<gmWq, tileProjW>;
using itWk = global_iterator<gmWk, tileProjW>;
using itWv = global_iterator<gmWv, tileProjW>;
using itWo = global_iterator<gmWo, tileWo>;
using itO = global_iterator<gmO, tileOutVec>;

} // namespace

extern "C" void mla_attention_demo_f32(float *out_ptr, float *q_ptr,
                                          float *k_ptr, float *v_ptr,
                                          float *wq_ptr, float *wk_ptr,
                                          float *wv_ptr, float *wo_ptr) {
  itQ gQ(q_ptr);
  itK gK(k_ptr);
  itV gV(v_ptr);
  itWq gWq(wq_ptr);
  itWk gWk(wk_ptr);
  itWv gWv(wv_ptr);
  itWo gWo(wo_ptr);
  itO gO(out_ptr);

  constexpr int kQBlocks = kS / kTm;
  constexpr int kKBlocks = kS / kTm;
  constexpr int kDChunks = kD / kTk;

  tileWo tWo;
  TLOAD(tWo, gWo(0, 0));

  for (int qi = 0; qi < kQBlocks; ++qi) {
    tileIn q0;
    tileProjW wq0;
    TLOAD(q0, gQ(qi, 0));
    TLOAD(wq0, gWq(0, 0));

    tileProjAcc qLatAcc;
    TMATMUL(qLatAcc, q0, wq0);

    for (int dk = 1; dk < kDChunks; ++dk) {
      tileIn q;
      tileProjW wq;
      TLOAD(q, gQ(qi, dk));
      TLOAD(wq, gWq(dk, 0));
      TMATMUL_ACC(qLatAcc, qLatAcc, q, wq);
    }

    tileProjVec qLatVec;
    tileProjLeft qLatLeft;
    TCVT(qLatVec, qLatAcc);
    TCVT(qLatLeft, qLatVec);

    tileCtxVec ctxVec(0.0f);

    for (int kj = 0; kj < kKBlocks; ++kj) {
      tileIn k0;
      tileProjW wk0;
      tileIn v0;
      tileProjW wv0;
      TLOAD(k0, gK(kj, 0));
      TLOAD(wk0, gWk(0, 0));
      TLOAD(v0, gV(kj, 0));
      TLOAD(wv0, gWv(0, 0));

      tileProjAcc kLatAcc;
      tileProjAcc vLatAcc;
      TMATMUL(kLatAcc, k0, wk0);
      TMATMUL(vLatAcc, v0, wv0);

      for (int dk = 1; dk < kDChunks; ++dk) {
        tileIn k;
        tileProjW wk;
        tileIn v;
        tileProjW wv;
        TLOAD(k, gK(kj, dk));
        TLOAD(wk, gWk(dk, 0));
        TLOAD(v, gV(kj, dk));
        TLOAD(wv, gWv(dk, 0));
        TMATMUL_ACC(kLatAcc, kLatAcc, k, wk);
        TMATMUL_ACC(vLatAcc, vLatAcc, v, wv);
      }

      tileProjRight kLatRight;
      tileProjRight vLatRight;
      tileProjAcc scoreAcc;
      tileProjVec scoreVec;
      tileProjVec scoreShifted;
      tileProjVec scoreExp;
      tileProjVec scoreNorm;
      tileProjVec rowMaxExpanded;
      tileProjVec rowSumExpanded;
      tileProjVec ctxPiece;
      tileProjLeft scoreLeft;
      tileCtxVec ctxMerged;
      Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor> rowMax;
      Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor> rowSum;
      Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor> invRowSum;

      TCVT(kLatRight, kLatAcc);
      TCVT(vLatRight, vLatAcc);

      TMATMUL(scoreAcc, qLatLeft, kLatRight);
      TCVT(scoreVec, scoreAcc);
      TMULS(scoreVec, scoreVec, kScale);

      TROWMAX(rowMax, scoreVec);
      TEXPANDCOL(rowMaxExpanded, rowMax);
      TSUB(scoreShifted, scoreVec, rowMaxExpanded);
      TEXP(scoreExp, scoreShifted);

      TROWSUM(rowSum, scoreExp);
      TRECIP(invRowSum, rowSum);
      TEXPANDCOL(rowSumExpanded, invRowSum);
      TMUL(scoreNorm, scoreExp, rowSumExpanded);

      TCVT(scoreLeft, scoreNorm);
      TMATMUL(scoreAcc, scoreLeft, vLatRight);
      TCVT(ctxPiece, scoreAcc);
      TADD(ctxMerged, ctxVec, ctxPiece);
      ctxVec = ctxMerged;
    }

    tileCtxLeft ctxLeft;
    tileOutAcc outAcc;
    tileOutVec outVec;
    TCVT(ctxLeft, ctxVec);
    TMATMUL(outAcc, ctxLeft, tWo);
    TCVT(outVec, outAcc);
    TSTORE(gO(qi, 0), outVec);
  }
}
