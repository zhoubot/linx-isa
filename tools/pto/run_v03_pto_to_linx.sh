#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${OUT_DIR:-$ROOT/tools/pto/out}"
mkdir -p "$OUT_DIR"
rm -f "$OUT_DIR"/*.s

CXX="${CLANGXX:-${CLANG:-}}"
if [[ -z "$CXX" ]]; then
  CAND="$ROOT/compiler/llvm/build-linxisa-clang/bin/clang++"
  if [[ -x "$CAND" ]]; then
    CXX="$CAND"
  else
    CAND="$HOME/llvm-project/build-linxisa-clang/bin/clang++"
    if [[ -x "$CAND" ]]; then
      CXX="$CAND"
    fi
  fi
fi
if [[ -z "$CXX" || ! -x "$CXX" ]]; then
  echo "error: clang++ not found; set CLANG=/path/to/clang++" >&2
  exit 1
fi

COMMON_FLAGS=(
  -target linx64-linx-none-elf
  -O2
  -S
  -ffreestanding
  -fno-builtin
  -fno-stack-protector
  -fno-exceptions
  -fno-rtti
  -nostdlib
  -I"$ROOT/lib/pto/include"
)

compile_one() {
  local src="$1"
  local out="$2"
  "$CXX" "${COMMON_FLAGS[@]}" "$src" -o "$out"
}

has_tile_range() {
  local asm="$1"
  local lo="$2"
  local hi="$3"
  awk -v lo="$lo" -v hi="$hi" '
    {
      line = $0
      while (match(line, /tile[0-9]+/)) {
        tile = substr(line, RSTART + 4, RLENGTH - 4) + 0
        if (tile >= lo && tile <= hi) {
          found = 1
          exit 0
        }
        line = substr(line, RSTART + RLENGTH)
      }
    }
    END { exit(found ? 0 : 1) }
  ' "$asm"
}

has_tile_hand() {
  local asm="$1"
  local hand="$2"
  grep -Eiq "\\b${hand}#?[0-7]\\b" "$asm"
}

check_no_forbidden_tokens() {
  local asm="$1"
  local forbidden_re='((^|[^A-Za-z0-9_])L\.|set_flag|wait_flag|TSync|B\.SET|B\.WAIT)'
  if grep -Eiq "$forbidden_re" "$asm"; then
    echo "error: forbidden v0.3 or non-auto-mode token found in $asm" >&2
    exit 1
  fi
}

check_tile_group_coverage() {
  local asm="$1"
  if ! has_tile_range "$asm" 0 7 && ! has_tile_hand "$asm" t; then
    echo "error: missing T tile-group usage (tile0..tile7) in $asm" >&2
    exit 1
  fi
  if ! has_tile_range "$asm" 8 15 && ! has_tile_hand "$asm" u; then
    echo "error: missing U tile-group usage (tile8..tile15) in $asm" >&2
    exit 1
  fi
  if ! has_tile_range "$asm" 16 23 && ! has_tile_hand "$asm" m; then
    echo "error: missing M tile-group usage (tile16..tile23) in $asm" >&2
    exit 1
  fi
  if ! has_tile_range "$asm" 24 31 && ! has_tile_hand "$asm" n; then
    echo "error: missing N tile-group usage (tile24..tile31) in $asm" >&2
    exit 1
  fi
}

check_tma_descriptor_headers() {
  local asm="$1"
  awk '
    /BSTART\.T(LOAD|STORE)|BSTART\.(TMA|PAR)[[:space:]]+T(LOAD|STORE)/ {
      inblk = 1
      seen_arg = 0
      seen_ior = 0
      seen_iot = 0
      next
    }
    inblk && /BSTART\./ {
      if (!seen_arg || !seen_ior || !seen_iot) {
        exit 1
      }
      inblk = 0
    }
    inblk {
      if ($0 ~ /B\.ARG/) seen_arg = 1
      if ($0 ~ /B\.IOR/) seen_ior = 1
      if ($0 ~ /B\.IOT/) seen_iot = 1
    }
    END {
      if (inblk && (!seen_arg || !seen_ior || !seen_iot)) {
        exit 1
      }
    }
  ' "$asm" || {
    echo "error: missing B.ARG/B.IOR/B.IOT descriptor in TMA block of $asm" >&2
    exit 1
  }
}

KERNELS=(
  tload_store
  mamulb
  tmatmul_acc
  gemm
  gemm_basic
  gemm_demo
  gemm_performance
  add_custom
  flash_attention
  flash_attention_demo
  flash_attention_masked
  fa_performance
  mla_attention_demo
)

for kernel in "${KERNELS[@]}"; do
  compile_one "$ROOT/workloads/pto_kernels/${kernel}.cpp" "$OUT_DIR/${kernel}.s"
done

for kernel in "${KERNELS[@]}"; do
  asm="$OUT_DIR/${kernel}.s"
  check_no_forbidden_tokens "$asm"
  check_tma_descriptor_headers "$asm"
done

grep -qE "BSTART\\.TLOAD|BSTART\\.(TMA|PAR)[[:space:]]+TLOAD" "$OUT_DIR/tload_store.s"
grep -qE "BSTART\\.TSTORE|BSTART\\.(TMA|PAR)[[:space:]]+TSTORE" "$OUT_DIR/tload_store.s"
grep -qE "BSTART\\.TMATMUL|BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB," "$OUT_DIR/mamulb.s"
grep -qE "BSTART\\.ACCCVT|BSTART\\.(CUBE|PAR)[[:space:]]+ACCCVT," "$OUT_DIR/mamulb.s"
grep -qE "BSTART\\.TMATMUL\\.ACC|BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB\\.ACC," "$OUT_DIR/tmatmul_acc.s"
grep -qE "BSTART\\.ACCCVT|BSTART\\.(CUBE|PAR)[[:space:]]+ACCCVT," "$OUT_DIR/tmatmul_acc.s"
grep -qE "BSTART\\.TMATMUL|BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB," "$OUT_DIR/gemm.s"
grep -qE "BSTART\\.TMATMUL|BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB," "$OUT_DIR/flash_attention.s"
grep -qE "BSTART\\.TEPL|BSTART\\.TEXPANDS|BSTART\\.TCOLEXPAND" "$OUT_DIR/flash_attention_masked.s"

if [[ "${RUN_QEMU_TILE:-0}" == "1" ]]; then
  CLANG_C="${QEMU_CLANG:-$(cd "$(dirname "$CXX")" && pwd)/clang}"
  if [[ ! -x "$CLANG_C" ]]; then
    CLANG_C="$CXX"
  fi
  QEMU_BIN="${QEMU:-}"
  if [[ -z "$QEMU_BIN" ]]; then
    for cand in "$HOME/qemu/build-tci/qemu-system-linx64" \
                "$HOME/qemu/build-linx/qemu-system-linx64"; do
      if [[ -x "$cand" ]]; then
        QEMU_BIN="$cand"
        break
      fi
    done
  fi
  QEMU_ARGS=()
  if [[ -n "$QEMU_BIN" && -x "$QEMU_BIN" ]]; then
    QEMU_ARGS+=(--qemu "$QEMU_BIN")
  fi
  CLANG="$CLANG_C" CLANGXX="$CXX" python3 "$ROOT/avs/qemu/run_tests.py" \
    --suite tile --timeout "${QEMU_TIMEOUT:-60}" \
    "${QEMU_ARGS[@]}" \
    --require-test-id 0x000A0001 \
    --require-test-id 0x000A0002 \
    --require-test-id 0x000A0003 \
    --require-test-id 0x000A0004 \
    --require-test-id 0x000A0005 \
    --require-test-id 0x000A0006 \
    --require-test-id 0x000A0007 \
    --require-test-id 0x000A0008 \
    --require-test-id 0x000A0009 \
    --require-test-id 0x000A000A
fi

if [[ "${RUN_PTO_PARITY:-0}" == "1" ]]; then
  python3 "$ROOT/tools/pto/run_pto_kernel_parity.py" --timeout "${PTO_PARITY_TIMEOUT:-180}"
fi

echo "ok: generated PTO->Linx v0.3 assembly in $OUT_DIR"
