#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LANE="pin"
RUN_ID=""
REPORT="$ROOT/docs/bringup/gates/latest.json"
LINX_BRINGUP_PROFILE="${LINX_BRINGUP_PROFILE:-release-strict}" # dev|release-strict
TRACE_SCHEMA_VERSION="${TRACE_SCHEMA_VERSION:-1.0}"
EXTERNAL_ROOT="${EXTERNAL_ROOT:-$HOME}"
LINUX_ROOT="${LINUX_ROOT:-$HOME/linux}"
OUT_BASE="$ROOT/docs/bringup/gates/logs"
QEMU_TIMEOUT="${QEMU_TIMEOUT:-10}"
MUSL_TIMEOUT="${MUSL_TIMEOUT:-90}"
LINX_DISABLE_TIMER_IRQ="${LINX_DISABLE_TIMER_IRQ:-1}"
LINX_EMU_DISABLE_TIMER_IRQ="${LINX_EMU_DISABLE_TIMER_IRQ:-0}"
RUN_GLIBC_G1B="${RUN_GLIBC_G1B-}"
GLIBC_G1B_ALLOW_BLOCKED="${GLIBC_G1B_ALLOW_BLOCKED-}"
RUN_MODEL_DIFF="${RUN_MODEL_DIFF-}"
RUN_CPP_GATES="${RUN_CPP_GATES-}" # 0|1
CPP_MODE="${CPP_MODE:-phase-b}"
STRICT_CROSS_ALLOW_G1_BLOCKED="${STRICT_CROSS_ALLOW_G1_BLOCKED-}"
if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  [[ -n "$RUN_GLIBC_G1B" ]] || RUN_GLIBC_G1B=1
  [[ -n "$GLIBC_G1B_ALLOW_BLOCKED" ]] || GLIBC_G1B_ALLOW_BLOCKED=0
  [[ -n "$RUN_MODEL_DIFF" ]] || RUN_MODEL_DIFF=1
  [[ -n "$RUN_CPP_GATES" ]] || RUN_CPP_GATES=0
  [[ -n "$STRICT_CROSS_ALLOW_G1_BLOCKED" ]] || STRICT_CROSS_ALLOW_G1_BLOCKED=0
else
  [[ -n "$RUN_GLIBC_G1B" ]] || RUN_GLIBC_G1B=1
  [[ -n "$GLIBC_G1B_ALLOW_BLOCKED" ]] || GLIBC_G1B_ALLOW_BLOCKED=1
  [[ -n "$RUN_MODEL_DIFF" ]] || RUN_MODEL_DIFF=0
  [[ -n "$RUN_CPP_GATES" ]] || RUN_CPP_GATES=0
  [[ -n "$STRICT_CROSS_ALLOW_G1_BLOCKED" ]] || STRICT_CROSS_ALLOW_G1_BLOCKED=1
fi

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  if [[ "$GLIBC_G1B_ALLOW_BLOCKED" != "0" ]]; then
    echo "error: release-strict forbids GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED" >&2
    exit 1
  fi
  if [[ "$STRICT_CROSS_ALLOW_G1_BLOCKED" != "0" ]]; then
    echo "error: release-strict forbids STRICT_CROSS_ALLOW_G1_BLOCKED=$STRICT_CROSS_ALLOW_G1_BLOCKED" >&2
    exit 1
  fi
  if [[ "$RUN_MODEL_DIFF" != "1" ]]; then
    echo "error: release-strict requires RUN_MODEL_DIFF=1" >&2
    exit 1
  fi
  if [[ "$RUN_GLIBC_G1B" != "1" ]]; then
    echo "error: release-strict requires RUN_GLIBC_G1B=1" >&2
    exit 1
  fi
fi
SKIP_LINUX=0
SKIP_STRICT_CROSS=0

usage() {
  cat <<'USAGE'
Usage: tools/bringup/run_runtime_convergence.sh [options]

Options:
  --lane pin|external          Lane to evaluate (default: pin)
  --run-id ID                  Run identifier (default: YYYY-MM-DD-r1-<lane>)
  --report PATH                Gate report JSON path
  --external-root PATH         External workspace root (default: $HOME)
  --linux-root PATH            External linux root (default: $HOME/linux)
  --qemu-timeout SEC           run_tests.sh timeout (default: 10)
  --musl-timeout SEC           musl smoke timeout (default: 90)
  --skip-glibc-g1b             Skip glibc G1b shared libc.so gate
  --strict-glibc-g1b           Treat G1b blocked status as failure
  --skip-linux                 Skip smoke.py/full_boot.py gates
  --skip-strict-cross          Skip tools/regression/strict_cross_repo.sh gate

Environment:
  LINX_BRINGUP_PROFILE=dev|release-strict   (default: release-strict)
  TRACE_SCHEMA_VERSION=MAJOR.MINOR          (default: 1.0)
  RUN_MODEL_DIFF=0|1                        (default: 1 in release-strict)
  RUN_CPP_GATES=0|1                         (default: 0)
  CPP_MODE=phase-b|...                      (default: phase-b)
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --lane)
      LANE="$2"
      shift 2
      ;;
    --run-id)
      RUN_ID="$2"
      shift 2
      ;;
    --report)
      REPORT="$2"
      shift 2
      ;;
    --external-root)
      EXTERNAL_ROOT="$2"
      shift 2
      ;;
    --linux-root)
      LINUX_ROOT="$2"
      shift 2
      ;;
    --qemu-timeout)
      QEMU_TIMEOUT="$2"
      shift 2
      ;;
    --musl-timeout)
      MUSL_TIMEOUT="$2"
      shift 2
      ;;
    --skip-glibc-g1b)
      RUN_GLIBC_G1B=0
      shift
      ;;
    --strict-glibc-g1b)
      GLIBC_G1B_ALLOW_BLOCKED=0
      shift
      ;;
    --skip-linux)
      SKIP_LINUX=1
      shift
      ;;
    --skip-strict-cross)
      SKIP_STRICT_CROSS=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "error: unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ "$LANE" != "pin" && "$LANE" != "external" ]]; then
  echo "error: --lane must be pin or external (got: $LANE)" >&2
  exit 2
fi

if [[ -z "$RUN_ID" ]]; then
  RUN_ID="$(date -u +%Y-%m-%d)-r1-${LANE}"
fi

resolve_clang() {
  if [[ -n "${CLANG:-}" && -x "${CLANG}" ]]; then
    echo "${CLANG}"
    return
  fi
  local cands=(
    "$HOME/llvm-project/build-linxisa-clang/bin/clang"
    "$ROOT/compiler/llvm/build-linxisa-clang/bin/clang"
  )
  local c
  for c in "${cands[@]}"; do
    if [[ -x "$c" ]]; then
      echo "$c"
      return
    fi
  done
  echo ""
}

resolve_lld() {
  local clang="$1"
  if [[ -n "${LLD:-}" && -x "${LLD}" ]]; then
    echo "${LLD}"
    return
  fi
  local cand
  cand="$(cd "$(dirname "$clang")" && pwd)/ld.lld"
  if [[ -x "$cand" ]]; then
    echo "$cand"
    return
  fi
  echo ""
}

resolve_qemu() {
  if [[ -n "${QEMU:-}" && -x "${QEMU}" ]]; then
    echo "${QEMU}"
    return
  fi
  local cands=()
  if [[ "$LANE" == "pin" ]]; then
    cands=(
      "$ROOT/emulator/qemu/build/qemu-system-linx64"
      "$ROOT/emulator/qemu/build-tci/qemu-system-linx64"
    )
  else
    cands=(
      "$EXTERNAL_ROOT/qemu/build/qemu-system-linx64"
      "$EXTERNAL_ROOT/qemu/build-tci/qemu-system-linx64"
    )
  fi
  local c
  for c in "${cands[@]}"; do
    if [[ -x "$c" ]]; then
      echo "$c"
      return
    fi
  done
  echo ""
}

CLANG_BIN="$(resolve_clang)"
if [[ -z "$CLANG_BIN" ]]; then
  echo "error: clang not found; set CLANG or build toolchain first" >&2
  exit 1
fi
LLD_BIN="$(resolve_lld "$CLANG_BIN")"
if [[ -z "$LLD_BIN" ]]; then
  echo "error: ld.lld not found; set LLD or build toolchain first" >&2
  exit 1
fi
QEMU_BIN="$(resolve_qemu)"
if [[ -z "$QEMU_BIN" ]]; then
  echo "error: qemu-system-linx64 not found for lane '$LANE'; set QEMU=..." >&2
  exit 1
fi
if [[ ! -d "$LINUX_ROOT/tools/linxisa/initramfs" ]]; then
  echo "error: linux initramfs tooling missing: $LINUX_ROOT/tools/linxisa/initramfs" >&2
  exit 1
fi

RUN_LOG_DIR="$OUT_BASE/$RUN_ID/$LANE"
mkdir -p "$RUN_LOG_DIR"

python3 "$ROOT/tools/bringup/gate_report.py" capture-sha \
  --report "$REPORT" \
  --root "$ROOT" \
  --external-root "$EXTERNAL_ROOT" \
  --lane "$LANE" \
  --run-id "$RUN_ID" \
  --profile "$LINX_BRINGUP_PROFILE" \
  --lane-policy "external+pin-required" \
  --trace-schema-version "$TRACE_SCHEMA_VERSION"
python3 "$ROOT/tools/bringup/gate_report.py" reset-run \
  --report "$REPORT" \
  --lane "$LANE" \
  --run-id "$RUN_ID"

echo "info: lane=$LANE run_id=$RUN_ID"
echo "info: profile=$LINX_BRINGUP_PROFILE trace_schema_version=$TRACE_SCHEMA_VERSION"
echo "info: clang=$CLANG_BIN"
echo "info: lld=$LLD_BIN"
echo "info: qemu=$QEMU_BIN"
echo "info: Linux runtime IRQ policy LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ"
echo "info: Emulator/system IRQ policy LINX_EMU_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ"
echo "info: glibc G1b gate RUN_GLIBC_G1B=$RUN_GLIBC_G1B GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED"
echo "info: C++ gates RUN_CPP_GATES=$RUN_CPP_GATES CPP_MODE=$CPP_MODE"
echo "info: logs=$RUN_LOG_DIR"

FAIL_COUNT=0

record_gate() {
  local domain="$1"
  local gate="$2"
  local command="$3"
  local status="$4"
  local classification="$5"
  local evidence="$6"
  local required="${7:-yes}"
  local waived="${8:-no}"
  local owner="${9:-bringup}"
  local evidence_type="${10:-log}"
  if [[ "$waived" == "yes" ]]; then
    python3 "$ROOT/tools/bringup/gate_report.py" upsert-gate \
      --report "$REPORT" \
      --lane "$LANE" \
      --run-id "$RUN_ID" \
      --domain "$domain" \
      --gate "$gate" \
      --command "$command" \
      --status "$status" \
      --classification "$classification" \
      --required "$required" \
      --owner "$owner" \
      --evidence-type "$evidence_type" \
      --waived \
      --evidence "$evidence"
  else
    python3 "$ROOT/tools/bringup/gate_report.py" upsert-gate \
      --report "$REPORT" \
      --lane "$LANE" \
      --run-id "$RUN_ID" \
      --domain "$domain" \
      --gate "$gate" \
      --command "$command" \
      --status "$status" \
      --classification "$classification" \
      --required "$required" \
      --owner "$owner" \
      --evidence-type "$evidence_type" \
      --evidence "$evidence"
  fi
}

run_gate() {
  local domain="$1"
  local gate="$2"
  local command="$3"
  local pass_class="$4"
  local fail_class="$5"
  local slug="$6"

  local log="$RUN_LOG_DIR/${slug}.log"
  echo
  echo "== [$domain] $gate"
  echo "cmd: $command"
  set +e
  bash -lc "$command" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    record_gate "$domain" "$gate" "$command" "pass" "$pass_class" "log:$log"
    echo "ok: $gate (log: $log)"
  else
    record_gate "$domain" "$gate" "$command" "fail" "$fail_class" "log:$log"
    echo "error: $gate failed (rc=$rc, log: $log)" >&2
    tail -n 40 "$log" >&2 || true
    FAIL_COUNT=$((FAIL_COUNT + 1))
  fi
}

run_glibc_g1b_gate() {
  local domain="Library"
  local gate="glibc G1b shared libc.so"
  local command="cd $ROOT && GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED bash lib/glibc/tools/linx/build_linx64_glibc_g1b.sh"
  local log="$RUN_LOG_DIR/lib_glibc_g1b.log"
  local summary="$ROOT/out/libc/glibc/logs/g1b-summary.txt"

  echo
  echo "== [$domain] $gate"
  echo "cmd: $command"
  set +e
  bash -lc "$command" >"$log" 2>&1
  local rc=$?
  set -e

  local status="pass"
  local waived="no"
  local classification="glibc_g1b_summary_missing"

  if [[ $rc -ne 0 ]]; then
    status="fail"
    classification="glibc_g1b_wrapper_fail"
  elif [[ -f "$summary" ]]; then
    local g1b_status=""
    local g1b_class=""
    local g1b_class_safe=""
    g1b_status="$(awk -F': *' '/^\[G1b\] status:/{s=$2} END{print s}' "$summary" | tr -d '\r')"
    g1b_class="$(awk -F': *' '/^\[G1b\] classification:/{s=$2} END{print s}' "$summary" | tr -d '\r')"
    g1b_class_safe="$(printf '%s' "$g1b_class" | tr -c '[:alnum:]_' '_')"
    if [[ -z "$g1b_class_safe" ]]; then
      g1b_class_safe="unknown"
    fi

    if [[ "$g1b_status" == "pass" ]]; then
      classification="glibc_g1b_pass_${g1b_class_safe}"
    elif [[ "$g1b_status" == "blocked" ]]; then
      if [[ "$GLIBC_G1B_ALLOW_BLOCKED" == "1" ]]; then
        status="pass"
        waived="yes"
        classification="glibc_g1b_blocked_allowed_${g1b_class_safe}"
      else
        status="fail"
        classification="glibc_g1b_blocked_${g1b_class_safe}"
      fi
    else
      classification="glibc_g1b_unknown_status"
    fi
  fi

  record_gate "$domain" "$gate" "$command" "$status" "$classification" "log:$log,summary:$summary" "yes" "$waived" "glibc" "log"
  if [[ "$status" == "pass" ]]; then
    echo "ok: $gate (log: $log)"
  else
    echo "error: $gate failed (rc=$rc, log: $log)" >&2
    tail -n 60 "$log" >&2 || true
    FAIL_COUNT=$((FAIL_COUNT + 1))
  fi
}

run_gate \
  "ISA" \
  "check26 contract" \
  "python3 $ROOT/tools/bringup/check26_contract.py --root $ROOT" \
  "contract_ok" \
  "contract_fail" \
  "isa_check26"

run_gate \
  "Compiler" \
  "AVS compile suites (linx64)" \
  "cd $ROOT/avs/compiler/linx-llvm/tests && CLANG=$CLANG_BIN TARGET=linx64-linx-none-elf OUT_DIR=$ROOT/avs/compiler/linx-llvm/tests/out-linx64 ./run.sh" \
  "compile_pass_linx64" \
  "compile_fail_linx64" \
  "compiler_linx64"

run_gate \
  "Compiler" \
  "Coverage 100% (linx64)" \
  "python3 $ROOT/avs/compiler/linx-llvm/tests/analyze_coverage.py --out-dir $ROOT/avs/compiler/linx-llvm/tests/out-linx64 --fail-under 100" \
  "mnemonic_coverage_100_linx64" \
  "mnemonic_coverage_under_100_linx64" \
  "compiler_cov_linx64"

run_gate \
  "Compiler" \
  "AVS compile suites (linx32)" \
  "cd $ROOT/avs/compiler/linx-llvm/tests && CLANG=$CLANG_BIN TARGET=linx32-linx-none-elf OUT_DIR=$ROOT/avs/compiler/linx-llvm/tests/out-linx32 ./run.sh" \
  "compile_pass_linx32" \
  "compile_fail_linx32" \
  "compiler_linx32"

run_gate \
  "Compiler" \
  "Coverage 100% (linx32)" \
  "python3 $ROOT/avs/compiler/linx-llvm/tests/analyze_coverage.py --out-dir $ROOT/avs/compiler/linx-llvm/tests/out-linx32 --fail-under 100" \
  "mnemonic_coverage_100_linx32" \
  "mnemonic_coverage_under_100_linx32" \
  "compiler_cov_linx32"

run_gate \
  "Emulator" \
  "QEMU strict system" \
  "cd $ROOT/avs/qemu && LINX_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ CLANG=$CLANG_BIN LLD=$LLD_BIN QEMU=$QEMU_BIN ./check_system_strict.sh" \
  "strict_system_pass" \
  "strict_system_fail" \
  "emu_strict_system"

run_gate \
  "Emulator" \
  "QEMU all suites" \
  "cd $ROOT/avs/qemu && LINX_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ CLANG=$CLANG_BIN LLD=$LLD_BIN QEMU=$QEMU_BIN ./run_tests.sh --all --timeout $QEMU_TIMEOUT" \
  "all_suites_pass" \
  "all_suites_fail_or_timeout" \
  "emu_all_suites"

if [[ $SKIP_LINUX -eq 0 ]]; then
  run_gate \
    "Kernel" \
    "Linux initramfs smoke" \
    "LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ QEMU=$QEMU_BIN python3 $LINUX_ROOT/tools/linxisa/initramfs/smoke.py" \
    "linux_smoke_pass" \
    "linux_smoke_fail" \
    "kernel_smoke"

  run_gate \
    "Kernel" \
    "Linux initramfs full boot" \
    "LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ QEMU=$QEMU_BIN python3 $LINUX_ROOT/tools/linxisa/initramfs/full_boot.py" \
    "linux_full_boot_pass" \
    "linux_full_boot_fail" \
    "kernel_full_boot"
else
  record_gate \
    "Kernel" \
    "Linux initramfs smoke" \
    "LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ QEMU=$QEMU_BIN python3 $LINUX_ROOT/tools/linxisa/initramfs/smoke.py" \
    "not_run" \
    "skipped_by_flag" \
    "note: --skip-linux" \
    "no" \
    "no" \
    "kernel" \
    "note"
  record_gate \
    "Kernel" \
    "Linux initramfs full boot" \
    "LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ QEMU=$QEMU_BIN python3 $LINUX_ROOT/tools/linxisa/initramfs/full_boot.py" \
    "not_run" \
    "skipped_by_flag" \
    "note: --skip-linux" \
    "no" \
    "no" \
    "kernel" \
    "note"
fi

run_gate \
  "Library" \
  "musl runtime static+shared" \
  "LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ python3 $ROOT/avs/qemu/run_musl_smoke.py --mode phase-b --link both --qemu $QEMU_BIN --timeout $MUSL_TIMEOUT" \
  "runtime_pass" \
  "runtime_mode_failure" \
  "lib_musl_both"

if [[ "$RUN_GLIBC_G1B" == "1" ]]; then
  run_glibc_g1b_gate
else
  record_gate \
    "Library" \
    "glibc G1b shared libc.so" \
    "cd $ROOT && GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED bash lib/glibc/tools/linx/build_linx64_glibc_g1b.sh" \
    "not_run" \
    "skipped_by_flag" \
    "note: --skip-glibc-g1b" \
    "no" \
    "no" \
    "glibc" \
    "note"
fi

if [[ "$RUN_MODEL_DIFF" == "1" ]]; then
  run_gate \
    "Model" \
    "QEMU vs model differential suite" \
    "python3 $ROOT/tools/bringup/run_model_diff_suite.py --root $ROOT --suite $ROOT/avs/model/linx_model_diff_suite.yaml --profile $LINX_BRINGUP_PROFILE --trace-schema-version $TRACE_SCHEMA_VERSION --report-out $RUN_LOG_DIR/model_diff_summary.json" \
    "model_diff_pass" \
    "model_diff_fail" \
    "model_diff_suite"
else
  record_gate \
    "Model" \
    "QEMU vs model differential suite" \
    "python3 $ROOT/tools/bringup/run_model_diff_suite.py --root $ROOT --suite $ROOT/avs/model/linx_model_diff_suite.yaml --profile $LINX_BRINGUP_PROFILE --trace-schema-version $TRACE_SCHEMA_VERSION --report-out $RUN_LOG_DIR/model_diff_summary.json" \
    "not_run" \
    "skipped_by_flag" \
    "note: RUN_MODEL_DIFF=0" \
    "no" \
    "no" \
    "model" \
    "note"
fi

if [[ $SKIP_STRICT_CROSS -eq 0 ]]; then
  QEMU_LANE_VALUE="external"
  if [[ "$LANE" == "pin" ]]; then
    QEMU_LANE_VALUE="pin"
  fi
  run_gate \
    "Regression" \
    "strict_cross_repo.sh" \
    "cd $ROOT && SKIP_BUILD=1 TOOLCHAIN_LANE=external QEMU_LANE=$QEMU_LANE_VALUE QEMU=$QEMU_BIN LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ LINX_EMU_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ RUN_GLIBC_G1=0 RUN_GLIBC_G1B=$RUN_GLIBC_G1B RUN_MODEL_DIFF=$RUN_MODEL_DIFF RUN_CPP_GATES=$RUN_CPP_GATES CPP_MODE=$CPP_MODE RUN_CONSISTENCY_CHECKS=0 ALLOW_GLIBC_G1_BLOCKED=$STRICT_CROSS_ALLOW_G1_BLOCKED GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED bash tools/regression/strict_cross_repo.sh" \
    "strict_cross_repo_pass" \
    "strict_cross_repo_fail" \
    "reg_strict_cross_repo"
else
  record_gate \
    "Regression" \
    "strict_cross_repo.sh" \
    "cd $ROOT && SKIP_BUILD=1 TOOLCHAIN_LANE=external QEMU_LANE=$LANE QEMU=$QEMU_BIN LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ LINX_EMU_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ RUN_GLIBC_G1=0 RUN_GLIBC_G1B=$RUN_GLIBC_G1B RUN_MODEL_DIFF=$RUN_MODEL_DIFF RUN_CPP_GATES=$RUN_CPP_GATES CPP_MODE=$CPP_MODE RUN_CONSISTENCY_CHECKS=0 ALLOW_GLIBC_G1_BLOCKED=$STRICT_CROSS_ALLOW_G1_BLOCKED GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED bash tools/regression/strict_cross_repo.sh" \
    "not_run" \
    "skipped_by_flag" \
    "note: --skip-strict-cross" \
    "no" \
    "no" \
    "regression" \
    "note"
fi

python3 "$ROOT/tools/bringup/gate_report.py" render --report "$REPORT" --out-md "$ROOT/docs/bringup/GATE_STATUS.md"

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  python3 "$ROOT/tools/bringup/check_gate_consistency.py" \
    --report "$REPORT" \
    --progress "$ROOT/docs/bringup/PROGRESS.md" \
    --gate-status "$ROOT/docs/bringup/GATE_STATUS.md" \
    --libc-status "$ROOT/docs/bringup/libc_status.md" \
    --profile "$LINX_BRINGUP_PROFILE" \
    --lane-policy "external+pin-required" \
    --trace-schema-version "$TRACE_SCHEMA_VERSION" \
    --max-age-hours "${LINX_GATE_MAX_AGE_HOURS:-24}"
fi

if [[ $FAIL_COUNT -ne 0 ]]; then
  echo "error: runtime convergence run completed with $FAIL_COUNT failing gate(s)" >&2
  exit 1
fi

echo "ok: runtime convergence run completed with all gates passing"
