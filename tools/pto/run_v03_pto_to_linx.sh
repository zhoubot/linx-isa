#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${OUT_DIR:-$ROOT/tools/pto/out}"
mkdir -p "$OUT_DIR"

CXX="${CLANGXX:-${CLANG:-}}"
if [[ -z "$CXX" ]]; then
  CAND="$HOME/llvm-project/build-linxisa-clang/bin/clang++"
  if [[ -x "$CAND" ]]; then
    CXX="$CAND"
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
  -I"$ROOT/impl/toolchain/pto/include"
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
    /BSTART\.(TMA|PAR)[[:space:]]+T(LOAD|STORE)/ {
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

compile_one "$ROOT/tools/pto/examples/pto_tload_store.cpp" "$OUT_DIR/pto_tload_store.s"
compile_one "$ROOT/tools/pto/examples/pto_mamulb.cpp" "$OUT_DIR/pto_mamulb.s"
compile_one "$ROOT/tools/pto/examples/pto_tmatmul_acc.cpp" "$OUT_DIR/pto_tmatmul_acc.s"
compile_one "$ROOT/tools/pto/examples/pto_gemm_auto.cpp" "$OUT_DIR/pto_gemm_auto.s"
compile_one "$ROOT/tools/pto/examples/pto_flash_attention_auto.cpp" "$OUT_DIR/pto_flash_attention_auto.s"

for asm in "$OUT_DIR"/pto_*.s; do
  check_no_forbidden_tokens "$asm"
  check_tma_descriptor_headers "$asm"
done

grep -qE "BSTART\\.(TMA|PAR)[[:space:]]+TLOAD" "$OUT_DIR/pto_tload_store.s"
grep -qE "BSTART\\.(TMA|PAR)[[:space:]]+TSTORE" "$OUT_DIR/pto_tload_store.s"
grep -qE "BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB," "$OUT_DIR/pto_mamulb.s"
grep -qE "BSTART\\.(CUBE|PAR)[[:space:]]+ACCCVT," "$OUT_DIR/pto_mamulb.s"
grep -qE "BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB\\.ACC," "$OUT_DIR/pto_tmatmul_acc.s"
grep -qE "BSTART\\.(CUBE|PAR)[[:space:]]+ACCCVT," "$OUT_DIR/pto_tmatmul_acc.s"
grep -qE "BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB," "$OUT_DIR/pto_gemm_auto.s"
grep -qE "BSTART\\.(CUBE|PAR)[[:space:]]+MAMULB," "$OUT_DIR/pto_flash_attention_auto.s"
check_tile_group_coverage "$OUT_DIR/pto_gemm_auto.s"
check_tile_group_coverage "$OUT_DIR/pto_flash_attention_auto.s"

if [[ "${RUN_QEMU_TILE:-1}" == "1" ]]; then
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
  CLANG="$CLANG_C" CLANGXX="$CXX" python3 "$ROOT/tests/qemu/run_tests.py" \
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

echo "ok: generated PTO->Linx v0.3 assembly in $OUT_DIR"
