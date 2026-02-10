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
  -I"$ROOT/toolchain/pto/include"
)

compile_one() {
  local src="$1"
  local out="$2"
  "$CXX" "${COMMON_FLAGS[@]}" "$src" -o "$out"
}

compile_one "$ROOT/tools/pto/examples/pto_tload_store.cpp" "$OUT_DIR/pto_tload_store.s"
compile_one "$ROOT/tools/pto/examples/pto_mamulb.cpp" "$OUT_DIR/pto_mamulb.s"
compile_one "$ROOT/tools/pto/examples/pto_tmatmul_acc.cpp" "$OUT_DIR/pto_tmatmul_acc.s"

for asm in "$OUT_DIR"/pto_*.s; do
  if grep -q "BSTART.PAR" "$asm"; then
    echo "error: legacy BSTART.PAR found in $asm" >&2
    exit 1
  fi
  if grep -q "L\\." "$asm"; then
    echo "error: legacy L.* mnemonic found in $asm" >&2
    exit 1
  fi
done

grep -q "BSTART.TMA" "$OUT_DIR/pto_tload_store.s"
grep -q "BSTART.CUBE" "$OUT_DIR/pto_mamulb.s"
grep -q "BSTART.CUBE" "$OUT_DIR/pto_tmatmul_acc.s"

if [[ "${RUN_QEMU_TILE:-1}" == "1" ]]; then
  CLANG_C="${QEMU_CLANG:-$(cd "$(dirname "$CXX")" && pwd)/clang}"
  if [[ ! -x "$CLANG_C" ]]; then
    CLANG_C="$CXX"
  fi
  CLANG="$CLANG_C" CLANGXX="$CXX" python3 "$ROOT/tests/qemu/run_tests.py" \
    --suite tile --timeout "${QEMU_TIMEOUT:-30}" \
    --require-test-id 0x000A0001 \
    --require-test-id 0x000A0002 \
    --require-test-id 0x000A0003
fi

echo "ok: generated PTO->Linx v0.3 assembly in $OUT_DIR"
