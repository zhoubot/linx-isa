#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LINX_DISABLE_TIMER_IRQ="${LINX_DISABLE_TIMER_IRQ:-1}"
LINX_EMU_DISABLE_TIMER_IRQ="${LINX_EMU_DISABLE_TIMER_IRQ:-0}"
LINX_BRINGUP_PROFILE="${LINX_BRINGUP_PROFILE:-release-strict}" # dev|release-strict

echo "== LinxISA full-stack regression =="
echo "profile: $LINX_BRINGUP_PROFILE"

echo
echo "-- LinxISA local regression (spec/gates + compile-only + QEMU suites)"
LINX_EMU_DISABLE_TIMER_IRQ="$LINX_EMU_DISABLE_TIMER_IRQ" bash "$ROOT/tools/regression/run.sh"

QEMU_BIN="${QEMU_BIN:-${QEMU:-}}"
if [[ -z "${QEMU_BIN:-}" ]]; then
  for cand in "$HOME/qemu/build/qemu-system-linx64" \
              "$HOME/qemu/build-tci/qemu-system-linx64" \
              "$HOME/qemu/build-linx/qemu-system-linx64"; do
    if [[ -x "$cand" ]]; then
      QEMU_BIN="$cand"
      break
    fi
  done
fi
if [[ -n "${QEMU_BIN:-}" ]]; then
  export QEMU="$QEMU_BIN"
elif [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  echo "error: QEMU binary is required in release-strict profile" >&2
  exit 1
fi

LLVM_ROOT="${LLVM_ROOT:-$HOME/llvm-project}"
if [[ -d "$LLVM_ROOT/llvm/test" ]]; then
  echo
  echo "-- LLVM lit (MC + CodeGen)"
  LLVM_LIT_BIN="${LLVM_LIT:-}"
  if [[ -z "${LLVM_LIT_BIN:-}" ]]; then
    for cand in "$LLVM_ROOT/build-linxisa-clang/bin/llvm-lit" \
                "$LLVM_ROOT/build/bin/llvm-lit"; do
      if [[ -x "$cand" ]]; then
        LLVM_LIT_BIN="$cand"
        break
      fi
    done
  fi
  if [[ -z "${LLVM_LIT_BIN:-}" ]]; then
    if command -v llvm-lit >/dev/null 2>&1; then
      LLVM_LIT_BIN="$(command -v llvm-lit)"
    fi
  fi
  if [[ -z "${LLVM_LIT_BIN:-}" ]]; then
    echo "note: skipping LLVM lit (llvm-lit not found). Set LLVM_LIT=/path/to/llvm-lit."
  else
    "$LLVM_LIT_BIN" -sv "$LLVM_ROOT/llvm/test/MC/LinxISA" "$LLVM_ROOT/llvm/test/CodeGen/LinxISA"
  fi
else
  echo
  echo "note: skipping LLVM lit (LLVM_ROOT missing: $LLVM_ROOT)"
fi

LINUX_ROOT="${LINUX_ROOT:-$HOME/linux}"
if [[ -d "$LINUX_ROOT/tools/linxisa/initramfs" ]]; then
  echo
  echo "-- Linux initramfs userspace boot (smoke/full)"
  LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" python3 "$LINUX_ROOT/tools/linxisa/initramfs/smoke.py"
  LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" python3 "$LINUX_ROOT/tools/linxisa/initramfs/full_boot.py"
  if [[ "${RUN_VIRTIO_DISK_SMOKE:-0}" == "1" ]]; then
    echo
    echo "-- Linux initramfs virtio disk smoke"
    LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" python3 "$LINUX_ROOT/tools/linxisa/initramfs/virtio_disk_smoke.py"
  else
    echo "note: skipping virtio disk smoke (set RUN_VIRTIO_DISK_SMOKE=1 to enable)"
  fi
else
  echo
  echo "note: skipping Linux boot scripts (LINUX_ROOT missing: $LINUX_ROOT)"
fi

if [[ "${RUN_MUSL_SMOKE:-0}" == "1" ]]; then
  echo
  echo "-- Linx musl malloc/printf runtime smoke"
  RUN_MUSL_SMOKE_MODE="${RUN_MUSL_SMOKE_MODE:-phase-b}"
  LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" \
    python3 "$ROOT/avs/qemu/run_musl_smoke.py" --mode "$RUN_MUSL_SMOKE_MODE"
fi

if [[ "${RUN_STRICT_CROSS_REPO:-0}" == "1" ]]; then
  echo
  echo "-- Strict cross-repo release gate"
  LINX_DISABLE_TIMER_IRQ="$LINX_DISABLE_TIMER_IRQ" \
  LINX_EMU_DISABLE_TIMER_IRQ="$LINX_EMU_DISABLE_TIMER_IRQ" \
    bash "$ROOT/tools/regression/strict_cross_repo.sh"
fi

PYC_ROOT="${PYC_ROOT:-$ROOT/tools/pyCircuit}"
PYC_AUTO_BUILD="${PYC_AUTO_BUILD-}" # 0|1
if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
  [[ -n "$PYC_AUTO_BUILD" ]] || PYC_AUTO_BUILD=1
else
  [[ -n "$PYC_AUTO_BUILD" ]] || PYC_AUTO_BUILD=0
fi

resolve_pyc_compile_bin() {
  local pyc_root="$1"
  local cand=""

  for cand in "$pyc_root/build/bin/pyc-compile" \
              "$pyc_root/pyc/mlir/build/bin/pyc-compile" \
              "$pyc_root/build-top/bin/pyc-compile"; do
    if [[ -x "$cand" ]]; then
      echo "$cand"
      return 0
    fi
  done

  if command -v pyc-compile >/dev/null 2>&1; then
    command -v pyc-compile
    return 0
  fi

  return 1
}

try_build_pyc_compile() {
  local pyc_root="$1"
  local clang_bin="${2:-}"
  local llvm_config="${LLVM_CONFIG:-}"
  local llvm_dir=""
  local mlir_dir=""
  local cand=""
  local config_root=""

  if [[ -z "$llvm_config" && -n "$clang_bin" ]]; then
    local tool_bin_dir
    tool_bin_dir="$(cd "$(dirname "$clang_bin")" && pwd)"
    if [[ -x "$tool_bin_dir/llvm-config" ]]; then
      llvm_config="$tool_bin_dir/llvm-config"
    fi
  fi

  if [[ -z "$llvm_config" ]]; then
    for cand in "${LLVM_ROOT:-$HOME/llvm-project}/build-linxisa-clang/bin/llvm-config" \
                "${LLVM_ROOT:-$HOME/llvm-project}/build/bin/llvm-config" \
                "$ROOT/compiler/llvm/build-linxisa-clang/bin/llvm-config"; do
      if [[ -x "$cand" ]]; then
        llvm_config="$cand"
        break
      fi
    done
  fi

  if [[ -z "$llvm_config" ]] && command -v llvm-config >/dev/null 2>&1; then
    llvm_config="$(command -v llvm-config)"
  fi

  if [[ -n "$llvm_config" && -x "$llvm_config" ]]; then
    llvm_dir="$("$llvm_config" --cmakedir 2>/dev/null || true)"
    if [[ -n "$llvm_dir" && -f "$llvm_dir/LLVMConfig.cmake" ]]; then
      config_root="$(dirname "$llvm_dir")"
      if [[ -f "$config_root/mlir/MLIRConfig.cmake" ]]; then
        mlir_dir="$config_root/mlir"
      fi
    fi
  fi

  if [[ -z "$mlir_dir" ]]; then
    for cand in "${LLVM_ROOT:-$HOME/llvm-project}/build-linxisa-clang/lib/cmake/mlir" \
                "${LLVM_ROOT:-$HOME/llvm-project}/build/lib/cmake/mlir" \
                "${LLVM_ROOT:-$HOME/llvm-project}/build-make/lib/cmake/mlir" \
                "$ROOT/compiler/llvm/build-linxisa-clang/lib/cmake/mlir" \
                "$ROOT/compiler/llvm/build/lib/cmake/mlir" \
                "$ROOT/compiler/llvm/build-make/lib/cmake/mlir"; do
      if [[ -f "$cand/MLIRConfig.cmake" ]]; then
        mlir_dir="$cand"
        if [[ -f "$(dirname "$cand")/llvm/LLVMConfig.cmake" ]]; then
          llvm_dir="$(dirname "$cand")/llvm"
        fi
        break
      fi
    done
  fi

  if [[ -n "$llvm_dir" && -n "$mlir_dir" && -f "$llvm_dir/LLVMConfig.cmake" && -f "$mlir_dir/MLIRConfig.cmake" ]]; then
    echo "info: building pyc-compile with LLVM_DIR=$llvm_dir MLIR_DIR=$mlir_dir"
    (cd "$pyc_root" && LLVM_DIR="$llvm_dir" MLIR_DIR="$mlir_dir" bash "scripts/pyc" build)
    return 0
  fi

  echo "error: unable to locate compatible LLVM/MLIR CMake configs for pyc-compile bootstrap (set LLVM_CONFIG, LLVM_ROOT, or LLVM_DIR/MLIR_DIR)." >&2
  return 1
}

if [[ -d "$PYC_ROOT" ]]; then
  PYC_COMPILE_BIN="${PYC_COMPILE:-}"
  if [[ -n "${PYC_COMPILE_BIN}" && ! -x "${PYC_COMPILE_BIN}" ]]; then
    echo "warning: ignoring non-executable PYC_COMPILE='$PYC_COMPILE_BIN'" >&2
    PYC_COMPILE_BIN=""
  fi

  if [[ -z "${PYC_COMPILE_BIN}" ]]; then
    if RESOLVED_PYC="$(resolve_pyc_compile_bin "$PYC_ROOT")"; then
      PYC_COMPILE_BIN="$RESOLVED_PYC"
    fi
  fi

  if [[ -z "${PYC_COMPILE_BIN}" && "$PYC_AUTO_BUILD" == "1" ]]; then
    if [[ ! -x "$PYC_ROOT/scripts/pyc" ]]; then
      echo "warning: cannot auto-build pyc-compile (missing: $PYC_ROOT/scripts/pyc)" >&2
    else
      echo
      echo "-- pyCircuit tool bootstrap"
      try_build_pyc_compile "$PYC_ROOT" "${CLANG:-}"
      if RESOLVED_PYC="$(resolve_pyc_compile_bin "$PYC_ROOT")"; then
        PYC_COMPILE_BIN="$RESOLVED_PYC"
      fi
    fi
  fi

  if [[ -z "${PYC_COMPILE_BIN}" || ! -x "${PYC_COMPILE_BIN}" ]]; then
    echo
    if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
      echo "error: pyCircuit/Janus gate required in release-strict profile, but pyc-compile was not found or could not be built (PYC_AUTO_BUILD=$PYC_AUTO_BUILD)" >&2
      exit 1
    fi
    echo "note: skipping pyCircuit/Janus (pyc-compile not found)"
  else
    export PYC_COMPILE="${PYC_COMPILE_BIN}"

    echo
    echo "-- pyCircuit + Janus model runs"
    (cd "$PYC_ROOT" && bash "tools/run_linx_cpu_pyc_cpp.sh")
    (cd "$PYC_ROOT" && bash "janus/tools/run_janus_bcc_pyc_cpp.sh")
    (cd "$PYC_ROOT" && bash "janus/tools/run_janus_bcc_ooo_pyc_cpp.sh")

    if [[ -n "${QEMU_BIN:-}" ]]; then
      echo
      echo "-- QEMU vs pyCircuit trace diff"
      (cd "$PYC_ROOT" && QEMU_BIN="$QEMU_BIN" bash "tools/run_linx_qemu_vs_pyc.sh")
    else
      echo
      echo "note: skipping QEMU vs pyCircuit trace diff (QEMU_BIN not found)"
    fi

    if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" || "${RUN_MODEL_DIFF_SUITE:-0}" == "1" ]]; then
      echo
      echo "-- Model differential suite"
      python3 "$ROOT/tools/bringup/run_model_diff_suite.py" \
        --root "$ROOT" \
        --suite "$ROOT/avs/model/linx_model_diff_suite.yaml" \
        --profile "$LINX_BRINGUP_PROFILE" \
        --trace-schema-version "${LINX_TRACE_SCHEMA_VERSION:-1.0}" \
        --report-out "$ROOT/docs/bringup/gates/model_diff_summary.json"
    fi
  fi
else
  echo
  if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
    echo "error: pyCircuit root is required in release-strict profile (missing: $PYC_ROOT)" >&2
    exit 1
  fi
  echo "note: skipping pyCircuit/Janus (PYC_ROOT missing: $PYC_ROOT)"
fi

echo
echo "ok: full-stack regression complete"

if [[ "$LINX_BRINGUP_PROFILE" == "release-strict" ]]; then
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
