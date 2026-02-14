#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "== LinxISA full-stack regression =="

echo
echo "-- LinxISA local regression (spec/gates + compile-only + QEMU suites)"
bash "$ROOT/tools/regression/run.sh"

echo
echo "-- TSVC full vectorization sweep (optional)"
if [[ "${RUN_TSVC_FULL:-0}" == "1" ]]; then
  CLANG_BIN="${CLANG:-${CLANG_BIN:-}}"
  if [[ -z "${CLANG_BIN:-}" ]]; then
    for cand in "$HOME/llvm-project/build-linxisa-clang/bin/clang" \
                "$HOME/llvm-project/build/bin/clang"; do
      if [[ -x "$cand" ]]; then
        CLANG_BIN="$cand"
        break
      fi
    done
  fi

  LLD_BIN="${LLD:-${LLD_BIN:-}}"
  if [[ -z "${LLD_BIN:-}" && -n "${CLANG_BIN:-}" ]]; then
    cand="$(cd "$(dirname "$CLANG_BIN")" && pwd)/ld.lld"
    if [[ -x "$cand" ]]; then
      LLD_BIN="$cand"
    fi
  fi

  if [[ -z "${QEMU_BIN:-}" ]]; then
    for cand in "$HOME/qemu/build-tci/qemu-system-linx64" \
                "$HOME/qemu/build/qemu-system-linx64" \
                "$HOME/qemu/build-linx/qemu-system-linx64"; do
      if [[ -x "$cand" ]]; then
        QEMU_BIN="$cand"
        break
      fi
    done
  fi

  if [[ -z "${CLANG_BIN:-}" || -z "${LLD_BIN:-}" || -z "${QEMU_BIN:-}" ]]; then
    echo "error: RUN_TSVC_FULL=1 requires CLANG/LLD/QEMU binaries" >&2
    exit 1
  fi

  python3 "$ROOT/workloads/benchmarks/run_tsvc.py" \
    --clang "$CLANG_BIN" \
    --lld "$LLD_BIN" \
    --qemu "$QEMU_BIN" \
    --timeout "${TSVC_TIMEOUT:-180}" \
    --iterations "${TSVC_ITERATIONS:-32}" \
    --len-1d "${TSVC_LEN_1D:-320}" \
    --len-2d "${TSVC_LEN_2D:-16}" \
    --vector-mode "${TSVC_VECTOR_MODE:-all}" \
    --coverage-fail-under "${TSVC_COVERAGE_FAIL_UNDER:-151}"
else
  echo "note: skipping TSVC full sweep (set RUN_TSVC_FULL=1 to enable)"
fi

QEMU_BIN="${QEMU_BIN:-${QEMU:-}}"
if [[ -z "${QEMU_BIN:-}" ]]; then
  for cand in "$HOME/qemu/build-tci/qemu-system-linx64" \
              "$HOME/qemu/build/qemu-system-linx64" \
              "$HOME/qemu/build-linx/qemu-system-linx64"; do
    if [[ -x "$cand" ]]; then
      QEMU_BIN="$cand"
      break
    fi
  done
fi
if [[ -n "${QEMU_BIN:-}" ]]; then
  export QEMU="$QEMU_BIN"
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
  echo "-- Linux initramfs userspace boot (smoke/full/virtio)"
  python3 "$LINUX_ROOT/tools/linxisa/initramfs/smoke.py"
  python3 "$LINUX_ROOT/tools/linxisa/initramfs/full_boot.py"
  python3 "$LINUX_ROOT/tools/linxisa/initramfs/virtio_disk_smoke.py"
else
  echo
  echo "note: skipping Linux boot scripts (LINUX_ROOT missing: $LINUX_ROOT)"
fi

echo
echo "-- PTO auto-mode AI kernels (compile/run/objdump)"
python3 "$ROOT/workloads/benchmarks/run_pto_ai_kernels.py"

PTO_ISA_ROOT="${PTO_ISA_ROOT:-$HOME/pto-isa}"
if [[ -f "$PTO_ISA_ROOT/include/pto/pto-inst.hpp" ]]; then
  echo
  echo "-- PTO CPU sim vs Linx QEMU value match (GEMM + Flash)"
  python3 "$ROOT/workloads/benchmarks/compare_pto_cpu_qemu.py" --pto-repo "$PTO_ISA_ROOT"
else
  echo
  echo "note: skipping PTO CPU sim compare (pto-isa not found at $PTO_ISA_ROOT)"
fi

PYC_ROOT="${PYC_ROOT:-$HOME/pyCircuit}"
if [[ -d "$PYC_ROOT" ]]; then
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
else
  echo
  echo "note: skipping pyCircuit/Janus (PYC_ROOT missing: $PYC_ROOT)"
fi

echo
echo "ok: full-stack regression complete"
