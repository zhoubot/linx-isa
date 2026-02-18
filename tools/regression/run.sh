#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LINX_BRINGUP_PROFILE="${LINX_BRINGUP_PROFILE:-release-strict}" # dev|release-strict

# Normalize working directory so helper scripts that use repo-relative defaults
# behave consistently regardless of caller cwd.
cd "$ROOT"

echo "== Linx regression =="
echo "profile: $LINX_BRINGUP_PROFILE"

echo
echo "-- ISA golden checks"
python3 "$ROOT/tools/isa/lint_no_cjk.py" --repo-root "$ROOT"
python3 "$ROOT/tools/isa/build_golden.py" --profile v0.3 --check
python3 "$ROOT/tools/isa/validate_spec.py" --profile v0.3
python3 "$ROOT/tools/bringup/check26_contract.py" --root "$ROOT"
python3 "$ROOT/tools/bringup/check_avs_matrix_status.py" --matrix "$ROOT/avs/linx_avs_v1_test_matrix.yaml" --status "$ROOT/avs/linx_avs_v1_test_matrix_status.json"
python3 "$ROOT/tools/bringup/check_check26_coverage.py" \
  --matrix "$ROOT/avs/linx_avs_v1_test_matrix.yaml" \
  --contract "$ROOT/docs/bringup/check26_contract.yaml" \
  --status "$ROOT/avs/linx_avs_v1_test_matrix_status.json" \
  --profile "$LINX_BRINGUP_PROFILE" \
  --report-out "$ROOT/docs/bringup/gates/check26_coverage_${LINX_BRINGUP_PROFILE}.json"

LEGACY_SCAN_ARGS=()
if [[ "${ENABLE_CROSS_REPO_SCAN:-0}" == "1" ]]; then
  DEFAULT_LINUX_ROOT="$ROOT/kernel/linux"
  [[ -d "$DEFAULT_LINUX_ROOT" ]] || DEFAULT_LINUX_ROOT="$HOME/linux"
  DEFAULT_QEMU_ROOT="$ROOT/emulator/qemu"
  [[ -d "$DEFAULT_QEMU_ROOT" ]] || DEFAULT_QEMU_ROOT="$HOME/qemu"
  DEFAULT_LLVM_ROOT="$ROOT/compiler/llvm"
  [[ -d "$DEFAULT_LLVM_ROOT" ]] || DEFAULT_LLVM_ROOT="$HOME/llvm-project"

  LINUX_ROOT="${LINUX_ROOT:-$DEFAULT_LINUX_ROOT}"
  QEMU_ROOT_CHECK="${QEMU_ROOT_CHECK:-$DEFAULT_QEMU_ROOT}"
  LLVM_ROOT="${LLVM_ROOT:-$DEFAULT_LLVM_ROOT}"
  [[ -d "$LINUX_ROOT" ]] && LEGACY_SCAN_ARGS+=(--extra-root "$LINUX_ROOT")
  [[ -d "$QEMU_ROOT_CHECK" ]] && LEGACY_SCAN_ARGS+=(--extra-root "$QEMU_ROOT_CHECK")
  [[ -d "$LLVM_ROOT" ]] && LEGACY_SCAN_ARGS+=(--extra-root "$LLVM_ROOT")
fi
if (( ${#LEGACY_SCAN_ARGS[@]} )); then
  python3 "$ROOT/tools/isa/check_no_legacy_v03.py" --root "$ROOT" "${LEGACY_SCAN_ARGS[@]}"
else
  python3 "$ROOT/tools/isa/check_no_legacy_v03.py" --root "$ROOT"
fi

python3 "$ROOT/tools/isa/report_encoding_space.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --out "$ROOT/docs/reference/encoding_space_report.md" --check
python3 "$ROOT/tools/isa/gen_qemu_codec.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --out-dir "$ROOT/isa/generated/codecs" --check
python3 "$ROOT/tools/isa/gen_c_codec.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --out-dir "$ROOT/isa/generated/codecs" --check
python3 "$ROOT/tools/isa/gen_manual_adoc.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --out-dir "$ROOT/docs/architecture/isa-manual/src/generated" --check
python3 "$ROOT/tools/isa/gen_ssr_adoc.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --out-dir "$ROOT/docs/architecture/isa-manual/src/generated" --check
SAIL_COVERAGE_POLICY="${SAIL_COVERAGE_POLICY:-refresh}" # refresh|check
if [[ "$SAIL_COVERAGE_POLICY" == "check" ]]; then
  python3 "$ROOT/tools/isa/sail_coverage.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --implemented "$ROOT/isa/sail/implemented_mnemonics.txt" --out "$ROOT/isa/sail/coverage.json" --check
else
  python3 "$ROOT/tools/isa/sail_coverage.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --implemented "$ROOT/isa/sail/implemented_mnemonics.txt" --out "$ROOT/isa/sail/coverage.json"
  python3 "$ROOT/tools/isa/sail_coverage.py" --spec "$ROOT/isa/v0.3/linxisa-v0.3.json" --implemented "$ROOT/isa/sail/implemented_mnemonics.txt" --out "$ROOT/isa/sail/coverage.json" --check
fi

# Allow callers to override tool locations.
CLANG="${CLANG:-}"
LLD="${LLD:-}"
QEMU="${QEMU:-}"
TOOLCHAIN_LANE="${TOOLCHAIN_LANE:-external}" # external|pin|auto
QEMU_LANE="${QEMU_LANE:-pin}" # external|pin|auto
QEMU_ROOT="${QEMU_ROOT:-}"
LINX_EMU_DISABLE_TIMER_IRQ="${LINX_EMU_DISABLE_TIMER_IRQ:-0}" # keep strict/system timer IRQs enabled by default

if [[ -z "$CLANG" ]]; then
  clang_candidates=()
  case "$TOOLCHAIN_LANE" in
    external)
      clang_candidates=(
        "$HOME/llvm-project/build-linxisa-clang/bin/clang"
        "$ROOT/compiler/llvm/build-linxisa-clang/bin/clang"
      )
      ;;
    pin)
      clang_candidates=(
        "$ROOT/compiler/llvm/build-linxisa-clang/bin/clang"
        "$HOME/llvm-project/build-linxisa-clang/bin/clang"
      )
      ;;
    auto)
      clang_candidates=(
        "$HOME/llvm-project/build-linxisa-clang/bin/clang"
        "$ROOT/compiler/llvm/build-linxisa-clang/bin/clang"
      )
      ;;
    *)
      echo "error: invalid TOOLCHAIN_LANE='$TOOLCHAIN_LANE' (expected external|pin|auto)" >&2
      exit 1
      ;;
  esac
  for CAND in "${clang_candidates[@]}"; do
    if [[ -x "$CAND" ]]; then
      CLANG="$CAND"
      break
    fi
  done
fi
if [[ -z "$LLD" && -n "$CLANG" ]]; then
  CAND="$(cd "$(dirname "$CLANG")" && pwd)/ld.lld"
  if [[ -x "$CAND" ]]; then
    LLD="$CAND"
  fi
fi
if [[ -z "$QEMU" ]]; then
  qemu_candidates=()
  case "$QEMU_LANE" in
    external)
      qemu_candidates=(
        "${QEMU_ROOT:-$HOME/qemu}/build/qemu-system-linx64"
        "${QEMU_ROOT:-$HOME/qemu}/build-tci/qemu-system-linx64"
      )
      ;;
    pin)
      qemu_candidates=(
        "$ROOT/emulator/qemu/build/qemu-system-linx64"
        "$ROOT/emulator/qemu/build-tci/qemu-system-linx64"
      )
      ;;
    auto)
      qemu_candidates=(
        "${QEMU_ROOT:-$HOME/qemu}/build/qemu-system-linx64"
        "${QEMU_ROOT:-$HOME/qemu}/build-tci/qemu-system-linx64"
        "$ROOT/emulator/qemu/build/qemu-system-linx64"
        "$ROOT/emulator/qemu/build-tci/qemu-system-linx64"
      )
      ;;
    *)
      echo "error: invalid QEMU_LANE='$QEMU_LANE' (expected external|pin|auto)" >&2
      exit 1
      ;;
  esac
  for CAND in "${qemu_candidates[@]}"; do
    if [[ -x "$CAND" ]]; then
      QEMU="$CAND"
      break
    fi
  done
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
echo "info: selected TOOLCHAIN lane=$TOOLCHAIN_LANE clang=$CLANG lld=$LLD"
echo "info: selected QEMU lane=$QEMU_LANE qemu=$QEMU"
echo "info: emulator/system IRQ policy LINX_EMU_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ"

echo
echo "-- Compiler compile-only tests (linx64)"
(cd "$ROOT/avs/compiler/linx-llvm/tests" && CLANG="$CLANG" TARGET="linx64-linx-none-elf" OUT_DIR="$ROOT/avs/compiler/linx-llvm/tests/out-linx64" ./run.sh)

echo
echo "-- Compiler coverage report (linx64)"
python3 "$ROOT/avs/compiler/linx-llvm/tests/analyze_coverage.py" --out-dir "$ROOT/avs/compiler/linx-llvm/tests/out-linx64" --fail-under "${COVERAGE_FAIL_UNDER:-100}"

echo
echo "-- Compiler compile-only tests (linx32)"
(cd "$ROOT/avs/compiler/linx-llvm/tests" && CLANG="$CLANG" TARGET="linx32-linx-none-elf" OUT_DIR="$ROOT/avs/compiler/linx-llvm/tests/out-linx32" ./run.sh)

echo
echo "-- Compiler coverage report (linx32)"
python3 "$ROOT/avs/compiler/linx-llvm/tests/analyze_coverage.py" --out-dir "$ROOT/avs/compiler/linx-llvm/tests/out-linx32" --fail-under "${COVERAGE_FAIL_UNDER:-100}"

RUN_CPP_GATES="${RUN_CPP_GATES:-0}" # 0|1
CPP_MODE="${CPP_MODE:-phase-b}"
RUN_PTO_PARITY_GATE="${RUN_PTO_PARITY_GATE-}" # 0|1
if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  [[ -n "$RUN_PTO_PARITY_GATE" ]] || RUN_PTO_PARITY_GATE=1
else
  [[ -n "$RUN_PTO_PARITY_GATE" ]] || RUN_PTO_PARITY_GATE=0
fi
if [[ "$RUN_CPP_GATES" == "1" ]]; then
  CLANGXX="$(cd "$(dirname "$CLANG")" && pwd)/clang++"
  if [[ ! -x "$CLANGXX" ]]; then
    echo "error: clang++ not found next to clang: $CLANGXX" >&2
    exit 1
  fi
  echo
  echo "-- C++ runtime overlay build (musl, C++17 no-EH/no-RTTI)"
  CLANG="$CLANG" CLANGXX="$CLANGXX" LLD="$LLD" \
    bash "$ROOT/tools/build_linx_llvm_cpp_runtimes.sh" --mode "$CPP_MODE"

  echo
  echo "-- C++ compile/link gate (musl, C++17 no-EH/no-RTTI)"
  (cd "$ROOT/avs/compiler/linx-llvm/tests" && \
    CLANGXX="$CLANGXX" MODE="$CPP_MODE" TARGET="linx64-unknown-linux-musl" LINK_MODE="both" ./run_cpp.sh)
else
  echo
  echo "note: skipping C++ gates (set RUN_CPP_GATES=1 to enable)"
fi

echo
echo "-- QEMU strict system gate (ACR/IRQ/exception coverage + noise check)"
(cd "$ROOT/avs/qemu" && LINX_DISABLE_TIMER_IRQ="$LINX_EMU_DISABLE_TIMER_IRQ" CLANG="$CLANG" LLD="$LLD" QEMU="$QEMU" ./check_system_strict.sh)

echo
echo "-- QEMU runtime tests"
(cd "$ROOT/avs/qemu" && LINX_DISABLE_TIMER_IRQ="$LINX_EMU_DISABLE_TIMER_IRQ" CLANG="$CLANG" LLD="$LLD" QEMU="$QEMU" ./run_tests.sh --all --timeout 10)

if [[ "$RUN_CPP_GATES" == "1" ]]; then
  CLANGXX="$(cd "$(dirname "$CLANG")" && pwd)/clang++"
  if [[ ! -x "$CLANGXX" ]]; then
    echo "error: clang++ not found next to clang: $CLANGXX" >&2
    exit 1
  fi
  CPP_RUNTIME_OUT="${CPP_RUNTIME_OUT:-$ROOT/avs/qemu/out/musl-smoke-cpp}"
  echo
  echo "-- musl C++17 runtime smoke (static+shared)"
  LINX_DISABLE_TIMER_IRQ="$LINX_EMU_DISABLE_TIMER_IRQ" \
    python3 "$ROOT/avs/qemu/run_musl_smoke.py" \
      --mode "$CPP_MODE" \
      --sample cpp17_smoke \
      --link both \
      --clang "$CLANG" \
      --clangxx "$CLANGXX" \
      --lld "$LLD" \
      --qemu "$QEMU" \
      --out-dir "$CPP_RUNTIME_OUT"
fi

if [[ "$RUN_PTO_PARITY_GATE" == "1" ]]; then
  echo
  echo "-- PTO host-vs-QEMU parity gate"
  python3 "$ROOT/tools/pto/run_pto_kernel_parity.py" --timeout "${PTO_PARITY_TIMEOUT:-180}"
else
  echo
  echo "note: skipping PTO parity gate (set RUN_PTO_PARITY_GATE=1 to enable)"
fi

echo
echo "-- ctuning Milepost codelets (optional)"
CTUNING_ROOT="${CTUNING_ROOT:-$HOME/ctuning-programs}"
CTUNING_LIMIT="${CTUNING_LIMIT:-5}"
BENCH_TARGET="${BENCH_TARGET:-linx64-linx-none-elf}"
CTUNING_REQUIRED="${CTUNING_REQUIRED:-0}"
if [[ -d "$CTUNING_ROOT/program" ]]; then
  set +e
  python3 "$ROOT/workloads/ctuning/run_milepost_codelets.py" \
    --ctuning-root "$CTUNING_ROOT" \
    --clang "$CLANG" \
    --lld "$LLD" \
    --qemu "$QEMU" \
    --target "$BENCH_TARGET" \
    --run \
    --limit "$CTUNING_LIMIT"
  CTUNING_RC=$?
  set -e
  if [[ "$CTUNING_RC" -ne 0 ]]; then
    if [[ "$CTUNING_REQUIRED" == "1" ]]; then
      echo "error: ctuning workload gate failed (CTUNING_REQUIRED=1)." >&2
      exit 1
    fi
    echo "note: ctuning workloads failed (rc=$CTUNING_RC); continuing because CTUNING_REQUIRED=0." >&2
  fi
else
  echo "note: skipping ctuning (not found at $CTUNING_ROOT)"
fi

echo
echo "ok: regression complete"

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" && "${RUN_GATE_CONSISTENCY_CHECK:-0}" == "1" ]]; then
  echo
  echo "-- bring-up consistency/freshness checks"
  python3 "$ROOT/tools/bringup/gate_report.py" render \
    --report "$ROOT/docs/bringup/gates/latest.json" \
    --out-md "$ROOT/docs/bringup/GATE_STATUS.md"
  python3 "$ROOT/tools/bringup/check_gate_consistency.py" \
    --report "$ROOT/docs/bringup/gates/latest.json" \
    --progress "$ROOT/docs/bringup/PROGRESS.md" \
    --gate-status "$ROOT/docs/bringup/GATE_STATUS.md" \
    --libc-status "$ROOT/docs/bringup/libc_status.md" \
    --profile "$LINX_BRINGUP_PROFILE" \
    --lane-policy "${LINX_LANE_POLICY:-external+pin-required}" \
    --trace-schema-version "${LINX_TRACE_SCHEMA_VERSION:-1.0}" \
    --max-age-hours "${LINX_GATE_MAX_AGE_HOURS:-24}"
fi
