#ifndef PTO_LINX_IMPL_PTO_INSTR_IMPL_HPP
#define PTO_LINX_IMPL_PTO_INSTR_IMPL_HPP

#include <common/pto_tileop.hpp>
#include <pto/common/constants.hpp>

namespace pto {
namespace linx {
namespace impl {

template <typename... Ts>
struct dependent_false {
  static constexpr bool value = false;
};

template <typename... Args>
inline void Unsupported(const char *op_name) {
  (void)op_name;
  static_assert(dependent_false<Args...>::value,
                "PTO Linx strict-v0.3: unsupported PTO op for __LINXISA__ backend");
}

} // namespace impl
} // namespace linx

template <typename Dst, typename Src>
inline void TLOAD_IMPL(Dst &dst, Src &src) {
  TLOAD(dst, src);
}

template <typename TileData, typename GlobalData,
          AtomicType atomicType = AtomicType::AtomicNone,
          ReluPreMode reluPreMode = ReluPreMode::NoRelu>
inline void TSTORE_IMPL(GlobalData &dst, TileData &src) {
  (void)atomicType;
  (void)reluPreMode;
  TSTORE(dst, src);
}

template <typename TileData, typename GlobalData, typename FpTileData,
          AtomicType atomicType = AtomicType::AtomicNone,
          ReluPreMode reluPreMode = ReluPreMode::NoRelu>
inline void TSTORE_IMPL(GlobalData &dst, TileData &src, FpTileData &) {
  (void)atomicType;
  (void)reluPreMode;
  TSTORE(dst, src);
}

template <typename TileData, typename GlobalData,
          AtomicType atomicType = AtomicType::AtomicNone,
          ReluPreMode reluPreMode = ReluPreMode::NoRelu>
inline void TSTORE_IMPL(GlobalData &dst, TileData &src, uint64_t) {
  (void)atomicType;
  (void)reluPreMode;
  TSTORE(dst, src);
}

template <typename TileRes, typename TileLeft, typename TileRight>
inline void TMATMUL_IMPL(TileRes &dst, TileLeft &lhs, TileRight &rhs) {
  TMATMUL(dst, lhs, rhs);
}

template <typename TileRes, typename TileLeft, typename TileRight>
inline void TMATMUL_ACC_IMPL(TileRes &dst, TileRes &acc, TileLeft &lhs,
                             TileRight &rhs) {
  TMATMUL_ACC(dst, acc, lhs, rhs);
}

template <typename Dst, typename Src0, typename Src1>
inline void TADD_IMPL(Dst &dst, Src0 &src0, Src1 &src1) {
  TADD(dst, src0, src1);
}

template <typename Dst, typename Src0, typename Src1>
inline void TSUB_IMPL(Dst &dst, Src0 &src0, Src1 &src1) {
  TSUB(dst, src0, src1);
}

template <typename Dst, typename Src0, typename Src1>
inline void TMUL_IMPL(Dst &dst, Src0 &src0, Src1 &src1) {
  TMUL(dst, src0, src1);
}

template <typename Dst, typename Src0, typename Src1>
inline void TMAX_IMPL(Dst &dst, Src0 &src0, Src1 &src1) {
  TMAX(dst, src0, src1);
}

template <typename Dst, typename Src>
inline void TEXPANDS_IMPL(Dst &dst, typename Dst::DType scalar) {
  TEXPANDS(dst, scalar);
}

template <typename Dst, typename Src>
inline void TEXP_IMPL(Dst &dst, Src &src) {
  TEXP(dst, src);
}

template <typename Dst, typename Src>
inline void TRECIP_IMPL(Dst &dst, Src &src) {
  TRECIP(dst, src);
}

template <typename Dst, typename Src>
inline void TCOLEXPAND_IMPL(Dst &dst, Src &src) {
  TCOLEXPAND(dst, src);
}

template <typename Dst, typename Src>
inline void TCVT_IMPL(Dst &dst, Src &src, RoundMode) {
  TCVT(dst, src);
}

template <typename Dst, typename Src, typename Tmp>
inline void TROWSUM_IMPL(Dst &dst, Src &src, Tmp &) {
  TROWSUM(dst, src);
}

template <typename Dst, typename Src, typename Tmp>
inline void TROWMAX_IMPL(Dst &dst, Src &src, Tmp &) {
  TROWMAX(dst, src);
}

template <typename Dst, typename Src>
inline void TMULS_IMPL(Dst &dst, Src &src, typename Src::DType scalar) {
  TMULS(dst, src, scalar);
}

template <typename Dst, typename Scalar, typename Src>
inline void TDIVS_IMPL(Dst &dst, Scalar, Src &src) {
  TRECIP(dst, src);
}

template <typename... Args>
inline void TAND_IMPL(Args &&...) {
  linx::impl::Unsupported<Args...>("TAND");
}

} // namespace pto

#endif // PTO_LINX_IMPL_PTO_INSTR_IMPL_HPP
