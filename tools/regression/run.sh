#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "== Linx regression =="

echo
echo "-- ISA golden checks"
python3 "$ROOT/tools/isa/lint_no_cjk.py"
python3 "$ROOT/tools/isa/build_golden.py" --in "$ROOT/isa/golden/v0.2" --out "$ROOT/isa/spec/current/linxisa-v0.2.json" --check
python3 "$ROOT/tools/isa/validate_spec.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json"
python3 "$ROOT/tools/isa/build_golden.py" --profile v0.3 --check
python3 "$ROOT/tools/isa/validate_spec.py" --profile v0.3
LINUX_ROOT="${LINUX_ROOT:-$HOME/linux}"
QEMU_ROOT_CHECK="${QEMU_ROOT_CHECK:-$HOME/qemu}"
LLVM_ROOT="${LLVM_ROOT:-$HOME/llvm-project}"
LEGACY_SCAN_ARGS=()
[[ -d "$LINUX_ROOT" ]] && LEGACY_SCAN_ARGS+=(--extra-root "$LINUX_ROOT")
[[ -d "$QEMU_ROOT_CHECK" ]] && LEGACY_SCAN_ARGS+=(--extra-root "$QEMU_ROOT_CHECK")
[[ -d "$LLVM_ROOT" ]] && LEGACY_SCAN_ARGS+=(--extra-root "$LLVM_ROOT")
python3 "$ROOT/tools/isa/check_no_legacy_v02.py" --root "$ROOT" "${LEGACY_SCAN_ARGS[@]}"
python3 "$ROOT/tools/isa/check_no_legacy_v03.py" --root "$ROOT" "${LEGACY_SCAN_ARGS[@]}"
python3 "$ROOT/tools/isa/report_encoding_space.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json" --out "$ROOT/docs/reference/encoding_space_report.md" --check
python3 "$ROOT/tools/isa/gen_qemu_codec.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json" --out-dir "$ROOT/isa/generated/codecs" --check
python3 "$ROOT/tools/isa/gen_c_codec.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json" --out-dir "$ROOT/isa/generated/codecs" --check
python3 "$ROOT/tools/isa/gen_manual_adoc.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json" --out-dir "$ROOT/docs/architecture/isa-manual/src/generated" --check
python3 "$ROOT/tools/isa/gen_ssr_adoc.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json" --out-dir "$ROOT/docs/architecture/isa-manual/src/generated" --check
python3 "$ROOT/tools/isa/sail_coverage.py" --spec "$ROOT/isa/spec/current/linxisa-v0.2.json" --implemented "$ROOT/isa/sail/implemented_mnemonics.txt" --out "$ROOT/isa/sail/coverage.json" --check

# Allow callers to override tool locations.
CLANG="${CLANG:-}"
LLD="${LLD:-}"
QEMU="${QEMU:-}"

if [[ -z "$CLANG" ]]; then
  CAND="$HOME/llvm-project/build-linxisa-clang/bin/clang"
  if [[ -x "$CAND" ]]; then
    CLANG="$CAND"
  fi
fi
if [[ -z "$LLD" && -n "$CLANG" ]]; then
  CAND="$(cd "$(dirname "$CLANG")" && pwd)/ld.lld"
  if [[ -x "$CAND" ]]; then
    LLD="$CAND"
  fi
fi
if [[ -z "$QEMU" ]]; then
  CAND="$HOME/qemu/build-tci/qemu-system-linx64"
  if [[ -x "$CAND" ]]; then
    QEMU="$CAND"
  else
    CAND="$HOME/qemu/build/qemu-system-linx64"
    if [[ -x "$CAND" ]]; then
      QEMU="$CAND"
    fi
  fi
fi

if [[ -z "$CLANG" || ! -x "$CLANG" ]]; then
  echo "error: CLANG not found; set CLANG=/path/to/clang" >&2
  exit 1
fi
if [[ -z "$LLD" || ! -x "$LLD" ]]; then
  echo "error: LLD not found; set LLD=/path/to/ld.lld" >&2
  exit 1
fi
if [[ -z "$QEMU" || ! -x "$QEMU" ]]; then
  echo "error: QEMU not found; set QEMU=/path/to/qemu-system-linx64" >&2
  exit 1
fi

echo
echo "-- Compiler compile-only tests (linx64)"
(cd "$ROOT/compiler/llvm/tests" && CLANG="$CLANG" TARGET="linx64-linx-none-elf" OUT_DIR="$ROOT/compiler/llvm/tests/out-linx64" ./run.sh)

echo
echo "-- Compiler coverage report (linx64)"
python3 "$ROOT/compiler/llvm/tests/analyze_coverage.py" --out-dir "$ROOT/compiler/llvm/tests/out-linx64" --fail-under "${COVERAGE_FAIL_UNDER:-100}"

echo
echo "-- Compiler compile-only tests (linx32)"
(cd "$ROOT/compiler/llvm/tests" && CLANG="$CLANG" TARGET="linx32-linx-none-elf" OUT_DIR="$ROOT/compiler/llvm/tests/out-linx32" ./run.sh)

echo
echo "-- Compiler coverage report (linx32)"
python3 "$ROOT/compiler/llvm/tests/analyze_coverage.py" --out-dir "$ROOT/compiler/llvm/tests/out-linx32" --fail-under "${COVERAGE_FAIL_UNDER:-100}"

echo
echo "-- QEMU strict system gate (ACR/IRQ/exception coverage + noise check)"
(cd "$ROOT/tests/qemu" && CLANG="$CLANG" LLD="$LLD" QEMU="$QEMU" ./check_system_strict.sh)

echo
echo "-- QEMU runtime tests"
(cd "$ROOT/tests/qemu" && CLANG="$CLANG" LLD="$LLD" QEMU="$QEMU" ./run_tests.sh --all --timeout 10)

echo
echo "-- ctuning Milepost codelets (optional)"
CTUNING_ROOT="${CTUNING_ROOT:-$HOME/ctuning-programs}"
CTUNING_LIMIT="${CTUNING_LIMIT:-5}"
if [[ -d "$CTUNING_ROOT/program" ]]; then
  python3 "$ROOT/tools/ctuning/run_milepost_codelets.py" \
    --ctuning-root "$CTUNING_ROOT" \
    --clang "$CLANG" \
    --lld "$LLD" \
    --qemu "$QEMU" \
    --target linx64-linx-none-elf \
    --run \
    --limit "$CTUNING_LIMIT"
else
  echo "note: skipping ctuning (not found at $CTUNING_ROOT)"
fi

echo
echo "ok: regression complete"
