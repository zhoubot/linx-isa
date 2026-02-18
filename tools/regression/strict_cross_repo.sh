#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LINX_BRINGUP_PROFILE="${LINX_BRINGUP_PROFILE:-release-strict}" # dev|release-strict

echo "== LinxISA strict cross-repo gate =="
echo "profile: $LINX_BRINGUP_PROFILE"

CLANG="${CLANG:-}"
LLD="${LLD:-}"
QEMU="${QEMU:-}"
TOOLCHAIN_LANE="${TOOLCHAIN_LANE:-external}" # external|pin|auto
# Runtime stability baseline defaults to external until pin lane converges.
QEMU_LANE="${QEMU_LANE:-external}" # auto|pin|external
QEMU_ROOT="${QEMU_ROOT:-}"
LINX_DISABLE_TIMER_IRQ="${LINX_DISABLE_TIMER_IRQ:-1}" # Linux runtime stabilization default
LINX_EMU_DISABLE_TIMER_IRQ="${LINX_EMU_DISABLE_TIMER_IRQ:-0}" # strict system/test coverage needs timer IRQ

RUN_GLIBC_G1="${RUN_GLIBC_G1:-1}"
GLIBC_G1_SCRIPT="${GLIBC_G1_SCRIPT:-$ROOT/lib/glibc/tools/linx/build_linx64_glibc.sh}"
RUN_GLIBC_G1B="${RUN_GLIBC_G1B-}"
GLIBC_G1B_ALLOW_BLOCKED="${GLIBC_G1B_ALLOW_BLOCKED-}"
GLIBC_G1B_SCRIPT="${GLIBC_G1B_SCRIPT:-$ROOT/lib/glibc/tools/linx/build_linx64_glibc_g1b.sh}"
ALLOW_GLIBC_G1_BLOCKED="${ALLOW_GLIBC_G1_BLOCKED-}"
RUN_MODEL_DIFF="${RUN_MODEL_DIFF-}"
RUN_CPP_GATES="${RUN_CPP_GATES-}" # 0|1
CPP_MODE="${CPP_MODE:-phase-b}"
RUN_CONSISTENCY_CHECKS="${RUN_CONSISTENCY_CHECKS:-1}" # 0|1 (nested runtime-convergence calls set 0)

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  [[ -n "$RUN_GLIBC_G1B" ]] || RUN_GLIBC_G1B=1
  [[ -n "$GLIBC_G1B_ALLOW_BLOCKED" ]] || GLIBC_G1B_ALLOW_BLOCKED=0
  [[ -n "$ALLOW_GLIBC_G1_BLOCKED" ]] || ALLOW_GLIBC_G1_BLOCKED=0
  [[ -n "$RUN_MODEL_DIFF" ]] || RUN_MODEL_DIFF=0
  [[ -n "$RUN_CPP_GATES" ]] || RUN_CPP_GATES=0
else
  [[ -n "$RUN_GLIBC_G1B" ]] || RUN_GLIBC_G1B=0
  [[ -n "$GLIBC_G1B_ALLOW_BLOCKED" ]] || GLIBC_G1B_ALLOW_BLOCKED=1
  [[ -n "$ALLOW_GLIBC_G1_BLOCKED" ]] || ALLOW_GLIBC_G1_BLOCKED=0
  [[ -n "$RUN_MODEL_DIFF" ]] || RUN_MODEL_DIFF=0
  [[ -n "$RUN_CPP_GATES" ]] || RUN_CPP_GATES=0
fi

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  if [[ "$ALLOW_GLIBC_G1_BLOCKED" != "0" ]]; then
    echo "error: release-strict forbids ALLOW_GLIBC_G1_BLOCKED=$ALLOW_GLIBC_G1_BLOCKED" >&2
    exit 1
  fi
  if [[ "$GLIBC_G1B_ALLOW_BLOCKED" != "0" ]]; then
    echo "error: release-strict forbids GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED" >&2
    exit 1
  fi
  if [[ "$RUN_GLIBC_G1B" != "1" ]]; then
    echo "error: release-strict requires RUN_GLIBC_G1B=1" >&2
    exit 1
  fi
fi

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
      echo "error: invalid TOOLCHAIN_LANE='$TOOLCHAIN_LANE' (expected: external|pin|auto)" >&2
      exit 1
      ;;
  esac
  for cand in "${clang_candidates[@]}"; do
    if [[ -x "$cand" ]]; then
      CLANG="$cand"
      break
    fi
  done
fi
if [[ -z "$LLD" && -n "$CLANG" ]]; then
  cand="$(cd "$(dirname "$CLANG")" && pwd)/ld.lld"
  if [[ -x "$cand" ]]; then
    LLD="$cand"
  fi
fi
if [[ -z "$QEMU" ]]; then
  qemu_candidates=()
  case "$QEMU_LANE" in
    pin)
      qemu_candidates=(
        "$ROOT/emulator/qemu/build/qemu-system-linx64"
        "$ROOT/emulator/qemu/build-tci/qemu-system-linx64"
      )
      ;;
    external)
      qemu_candidates=(
        "${QEMU_ROOT:-$HOME/qemu}/build/qemu-system-linx64"
        "${QEMU_ROOT:-$HOME/qemu}/build-tci/qemu-system-linx64"
      )
      ;;
    auto)
      qemu_candidates=(
        "$ROOT/emulator/qemu/build/qemu-system-linx64"
        "$ROOT/emulator/qemu/build-tci/qemu-system-linx64"
        "${QEMU_ROOT:-$HOME/qemu}/build/qemu-system-linx64"
        "${QEMU_ROOT:-$HOME/qemu}/build-tci/qemu-system-linx64"
      )
      ;;
    *)
      echo "error: invalid QEMU_LANE='$QEMU_LANE' (expected: auto|pin|external)" >&2
      exit 1
      ;;
  esac
  for cand in "${qemu_candidates[@]}"; do
    if [[ -x "$cand" ]]; then
      QEMU="$cand"
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
echo "info: Linux runtime IRQ policy LINX_DISABLE_TIMER_IRQ=$LINX_DISABLE_TIMER_IRQ"
echo "info: Emulator/system IRQ policy LINX_EMU_DISABLE_TIMER_IRQ=$LINX_EMU_DISABLE_TIMER_IRQ"
echo "info: release controls RUN_GLIBC_G1B=$RUN_GLIBC_G1B GLIBC_G1B_ALLOW_BLOCKED=$GLIBC_G1B_ALLOW_BLOCKED ALLOW_GLIBC_G1_BLOCKED=$ALLOW_GLIBC_G1_BLOCKED RUN_MODEL_DIFF=$RUN_MODEL_DIFF"
echo "info: C++ controls RUN_CPP_GATES=$RUN_CPP_GATES CPP_MODE=$CPP_MODE"

echo
echo "-- Compiler AVS gate"
(cd "$ROOT/avs/compiler/linx-llvm/tests" && CLANG="$CLANG" ./run.sh)

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
fi

echo
echo "-- QEMU strict system gate"
(cd "$ROOT/avs/qemu" && LINX_DISABLE_TIMER_IRQ="$LINX_EMU_DISABLE_TIMER_IRQ" CLANG="$CLANG" LLD="$LLD" QEMU="$QEMU" ./check_system_strict.sh)

LINUX_ROOT="${LINUX_ROOT:-$HOME/linux}"
if [[ ! -d "$LINUX_ROOT/tools/linxisa/initramfs" ]]; then
  echo "error: Linux initramfs tooling not found at $LINUX_ROOT/tools/linxisa/initramfs" >&2
  exit 1
fi

echo
echo "-- Linux initramfs smoke/full"
LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" QEMU="$QEMU" python3 "$LINUX_ROOT/tools/linxisa/initramfs/smoke.py"
LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" QEMU="$QEMU" python3 "$LINUX_ROOT/tools/linxisa/initramfs/full_boot.py"

echo
echo "-- musl runtime smoke (phase-b, static+shared)"
MUSL_MODE="${MUSL_MODE:-phase-b}"
MUSL_SUMMARY_DIR="${MUSL_SUMMARY_DIR:-$ROOT/avs/qemu/out/musl-smoke}"
set +e
LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" \
  python3 "$ROOT/avs/qemu/run_musl_smoke.py" --mode "$MUSL_MODE" --link both --qemu "$QEMU"
MUSL_COMBINED_RC=$?
set -e
if [[ "$MUSL_COMBINED_RC" -ne 0 ]]; then
  echo "note: combined musl run returned rc=$MUSL_COMBINED_RC; validating per-mode summaries." >&2
fi

for mode in static shared; do
  summary="$MUSL_SUMMARY_DIR/summary_${mode}.json"
  if [[ ! -f "$summary" ]]; then
    echo "error: missing musl ${mode} summary: $summary" >&2
    exit 1
  fi
  python3 - "$summary" "$mode" <<'PY'
import json
import sys
from pathlib import Path

summary_path = Path(sys.argv[1])
mode = sys.argv[2]
data = json.loads(summary_path.read_text(encoding="utf-8"))
result = data.get("result", {})
ok = bool(result.get("ok", False))
classification = str(result.get("classification", "unknown"))
if not ok:
    print(
        f"error: musl {mode} gate failed ({classification}) from {summary_path}",
        file=sys.stderr,
    )
    raise SystemExit(1)
print(f"ok: musl {mode} gate passed ({summary_path})")
PY
done

if [[ "$RUN_CPP_GATES" == "1" ]]; then
  CLANGXX="$(cd "$(dirname "$CLANG")" && pwd)/clang++"
  CPP_SUMMARY_DIR="${CPP_SUMMARY_DIR:-$ROOT/avs/qemu/out/musl-smoke-cpp}"
  echo
  echo "-- musl C++17 runtime smoke (static+shared)"
  LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" \
    python3 "$ROOT/avs/qemu/run_musl_smoke.py" \
      --mode "$CPP_MODE" \
      --sample cpp17_smoke \
      --link both \
      --clang "$CLANG" \
      --clangxx "$CLANGXX" \
      --lld "$LLD" \
      --qemu "$QEMU" \
      --out-dir "$CPP_SUMMARY_DIR"
fi

if [[ "$RUN_GLIBC_G1" == "1" ]]; then
  if [[ ! -x "$GLIBC_G1_SCRIPT" ]]; then
    echo "error: glibc G1 script not found: $GLIBC_G1_SCRIPT" >&2
    exit 1
  fi
  echo
  echo "-- glibc G1 build gate"
  bash "$GLIBC_G1_SCRIPT"
fi

GLIBC_SUMMARY="${GLIBC_SUMMARY:-$ROOT/out/libc/glibc/logs/summary.txt}"
if [[ ! -f "$GLIBC_SUMMARY" ]]; then
  echo "error: glibc G1 summary not found: $GLIBC_SUMMARY" >&2
  exit 1
fi

echo
echo "-- glibc G1 status"
cat "$GLIBC_SUMMARY"
if grep -Eiq "(blocked|fail|error)" "$GLIBC_SUMMARY"; then
  if [[ "$ALLOW_GLIBC_G1_BLOCKED" == "1" ]]; then
    echo "note: glibc G1 is blocked (ALLOW_GLIBC_G1_BLOCKED=1 set)." >&2
  else
    echo "error: glibc G1 is blocked; strict gate failed." >&2
    exit 1
  fi
fi

if [[ "$RUN_GLIBC_G1B" == "1" ]]; then
  if [[ ! -x "$GLIBC_G1B_SCRIPT" ]]; then
    echo "error: glibc G1b script not found: $GLIBC_G1B_SCRIPT" >&2
    exit 1
  fi
  echo
  echo "-- glibc G1b shared libc.so gate"
  GLIBC_G1B_ALLOW_BLOCKED="$GLIBC_G1B_ALLOW_BLOCKED" bash "$GLIBC_G1B_SCRIPT"

  GLIBC_G1B_SUMMARY="${GLIBC_G1B_SUMMARY:-$ROOT/out/libc/glibc/logs/g1b-summary.txt}"
  if [[ ! -f "$GLIBC_G1B_SUMMARY" ]]; then
    echo "error: glibc G1b summary not found: $GLIBC_G1B_SUMMARY" >&2
    exit 1
  fi
  echo
  echo "-- glibc G1b status"
  cat "$GLIBC_G1B_SUMMARY"

  if grep -Eiq "status:[[:space:]]*blocked" "$GLIBC_G1B_SUMMARY"; then
    if [[ "$GLIBC_G1B_ALLOW_BLOCKED" == "1" ]]; then
      echo "note: glibc G1b is blocked (GLIBC_G1B_ALLOW_BLOCKED=1 set)." >&2
    else
      echo "error: glibc G1b is blocked; strict gate failed." >&2
      exit 1
    fi
  fi
fi

if [[ "$RUN_MODEL_DIFF" == "1" ]]; then
  echo
  echo "-- QEMU vs model differential suite"
  python3 "$ROOT/tools/bringup/run_model_diff_suite.py" \
    --root "$ROOT" \
    --suite "$ROOT/avs/model/linx_model_diff_suite.yaml" \
    --profile "$LINX_BRINGUP_PROFILE" \
    --trace-schema-version "${LINX_TRACE_SCHEMA_VERSION:-1.0}" \
    --report-out "$ROOT/docs/bringup/gates/model_diff_summary.json"
fi

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" && "$RUN_CONSISTENCY_CHECKS" == "1" ]]; then
  echo
  echo "-- bring-up consistency/freshness checks"
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

echo
echo "ok: strict cross-repo gate passed"
