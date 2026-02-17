#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "== LinxISA strict cross-repo gate =="

CLANG="${CLANG:-}"
LLD="${LLD:-}"
QEMU="${QEMU:-}"
TOOLCHAIN_LANE="${TOOLCHAIN_LANE:-external}" # external|pin|auto
# Runtime stability baseline defaults to external until pin lane converges.
QEMU_LANE="${QEMU_LANE:-external}" # auto|pin|external
QEMU_ROOT="${QEMU_ROOT:-}"

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

echo
echo "-- Compiler AVS gate"
(cd "$ROOT/avs/compiler/linx-llvm/tests" && CLANG="$CLANG" ./run.sh)

echo
echo "-- QEMU strict system gate"
(cd "$ROOT/avs/qemu" && CLANG="$CLANG" LLD="$LLD" QEMU="$QEMU" ./check_system_strict.sh)

LINUX_ROOT="${LINUX_ROOT:-$HOME/linux}"
if [[ ! -d "$LINUX_ROOT/tools/linxisa/initramfs" ]]; then
  echo "error: Linux initramfs tooling not found at $LINUX_ROOT/tools/linxisa/initramfs" >&2
  exit 1
fi

echo
echo "-- Linux initramfs smoke/full"
QEMU="$QEMU" python3 "$LINUX_ROOT/tools/linxisa/initramfs/smoke.py"
QEMU="$QEMU" python3 "$LINUX_ROOT/tools/linxisa/initramfs/full_boot.py"

echo
echo "-- musl runtime smoke (phase-b, static+shared)"
MUSL_MODE="${MUSL_MODE:-phase-b}"
MUSL_SUMMARY_DIR="${MUSL_SUMMARY_DIR:-$ROOT/avs/qemu/out/musl-smoke}"
set +e
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

RUN_GLIBC_G1="${RUN_GLIBC_G1:-1}"
GLIBC_G1_SCRIPT="${GLIBC_G1_SCRIPT:-$ROOT/lib/glibc/tools/linx/build_linx64_glibc.sh}"
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
  if [[ "${ALLOW_GLIBC_G1_BLOCKED:-0}" == "1" ]]; then
    echo "note: glibc G1 is blocked (ALLOW_GLIBC_G1_BLOCKED=1 set)." >&2
  else
    echo "error: glibc G1 is blocked; strict gate failed." >&2
    exit 1
  fi
fi

echo
echo "ok: strict cross-repo gate passed"
