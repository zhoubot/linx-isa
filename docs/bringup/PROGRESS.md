# Bring-up Progress (v0.4 workspace)

Last updated: 2026-02-17

## Phase status

| Phase | Status | Evidence |
| --- | --- | --- |
| 1. Contract freeze (26 checks) | ✅ Passed | `python3 tools/bringup/check26_contract.py --root .` |
| 2. linxisa v0.3 cutover | ✅ Passed | `bash tools/regression/run.sh` |
| 3. LLVM MC/CodeGen alignment | ✅ Passed | `llvm-lit llvm/test/MC/LinxISA llvm/test/CodeGen/LinxISA` |
| 4. QEMU runtime/system alignment | ✅ Passed | `avs/qemu/check_system_strict.sh`; `avs/qemu/run_tests.sh --all` |
| 5. Linux userspace boot path | ✅ Passed | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py`; `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` |
| 6. pyCircuit + Janus model alignment | ✅ Bring-up scope complete | pyCircuit/Janus run scripts |
| 7. Skills/docs sync + full stack regression | ✅ Passed | `CLANG=/Users/zhoubot/llvm-project/build-linxisa-clang/bin/clang LLD=/Users/zhoubot/llvm-project/build-linxisa-clang/bin/ld.lld LLVM_ROOT=/Users/zhoubot/linx-isa/compiler/llvm QEMU=/Users/zhoubot/qemu/build/qemu-system-linx64 LINUX_ROOT=/Users/zhoubot/linux LINX_DISABLE_TIMER_IRQ=1 LINX_EMU_DISABLE_TIMER_IRQ=0 bash tools/regression/full_stack.sh` |
| 8. musl Linux runtime bring-up | ✅ Phase-B runtime pass | `MODE=phase-b lib/musl/tools/linx/build_linx64_musl.sh`; `python3 avs/qemu/run_musl_smoke.py --mode phase-b` |

## Level 1 closure activation (2026-02-17)

- Release-strict gating now enforces:
  - check26 directed coverage linkage (including `Model` domain tests),
  - QEMU-vs-model differential suite as a required gate,
  - trace schema version compatibility checks,
  - external/pin lane parity checks for required gate sets.
- Current blockers are tracked in `docs/bringup/MATURITY_PLAN.md` immediate backlog.

## Gate snapshot

| Gate | Status | Command |
| --- | --- | --- |
| AVS compile-only (`linx64`/`linx32`) | ✅ | `./avs/compiler/linx-llvm/tests/run.sh` |
| AVS runtime suites | ✅ | `./avs/qemu/run_tests.sh --all` |
| Strict system gate | ✅ | `./avs/qemu/check_system_strict.sh` |
| Main regression | ✅ | `bash tools/regression/run.sh` |
| Linux initramfs smoke/full | ✅ | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py`; `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` |
| glibc `G1a` | ✅ (`configure` + `csu/subdir_lib`) | `bash /Users/zhoubot/linx-isa/lib/glibc/tools/linx/build_linx64_glibc.sh` |
| musl `M1` | ✅ | `MODE=phase-b /Users/zhoubot/linx-isa/lib/musl/tools/linx/build_linx64_musl.sh` |
| musl `M2` | ✅ (phase-b strict) | `out/libc/musl/logs/phase-b-summary.txt` |
| musl `M3` | ✅ (phase-b strict) | `out/libc/musl/logs/phase-b-summary.txt`; `out/libc/musl/logs/phase-b-m3-shared.log` |
| musl runtime `R1` | ✅ | `avs/qemu/out/musl-smoke/compile.log` |
| musl runtime `R2` | ✅ | `avs/qemu/out/musl-smoke/qemu.log` |

## Latest command log

- `MODE=phase-b /Users/zhoubot/linx-isa/lib/musl/tools/linx/build_linx64_musl.sh` ✅ (`M1/M2/M3` pass)
- `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --mode phase-b` ✅ (`runtime_pass`)
- `bash /Users/zhoubot/linx-isa/lib/glibc/tools/linx/build_linx64_glibc.sh` ✅ (`G1a`: configure + `csu/subdir_lib` + `crt1.o`)
- `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py` ✅
- `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` ✅
- `bash /Users/zhoubot/linx-isa/tools/regression/strict_cross_repo.sh` ✅
- `CLANG=/Users/zhoubot/llvm-project/build-linxisa-clang/bin/clang LLD=/Users/zhoubot/llvm-project/build-linxisa-clang/bin/ld.lld LLVM_ROOT=/Users/zhoubot/linx-isa/compiler/llvm QEMU=/Users/zhoubot/qemu/build/qemu-system-linx64 LINUX_ROOT=/Users/zhoubot/linux LINX_DISABLE_TIMER_IRQ=1 LINX_EMU_DISABLE_TIMER_IRQ=0 bash /Users/zhoubot/linx-isa/tools/regression/full_stack.sh` ✅

## Canonical Gate Table

- `docs/bringup/gates/latest.json` is the source-of-truth gate artifact (lane + SHA + command + result).
- `docs/bringup/GATE_STATUS.md` is generated from the JSON gate artifact.
