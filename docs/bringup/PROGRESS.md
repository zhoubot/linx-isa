# Bring-up Progress (v0.4 workspace)

Last updated: 2026-02-15

## Phase status

| Phase | Status | Evidence |
| --- | --- | --- |
| 1. Contract freeze (26 checks) | ✅ Passed | `python3 tools/bringup/check26_contract.py --root .` |
| 2. linxisa v0.3 cutover | ✅ Passed | `bash tools/regression/run.sh` |
| 3. LLVM MC/CodeGen alignment | ✅ Passed | `llvm-lit llvm/test/MC/LinxISA llvm/test/CodeGen/LinxISA` |
| 4. QEMU runtime/system alignment | ✅ Passed | `avs/qemu/check_system_strict.sh`; `avs/qemu/run_tests.sh --all` |
| 5. Linux userspace boot path | ⚠ Regressed | `out/libc/musl/logs/linux-initramfs-smoke.latest.err`; `out/libc/musl/logs/linux-initramfs-full.latest.err` |
| 6. pyCircuit + Janus model alignment | ✅ Bring-up scope complete | pyCircuit/Janus run scripts |
| 7. Skills/docs sync + full stack regression | ⚠ Partial | full-stack strict pass blocked by phase 5 regression |
| 8. musl Linux runtime bring-up | ✅ Phase-B runtime pass | `MODE=phase-b lib/musl/tools/linx/build_linx64_musl.sh`; `python3 avs/qemu/run_musl_smoke.py --mode phase-b` |

## Gate snapshot

| Gate | Status | Command |
| --- | --- | --- |
| AVS compile-only (`linx64`/`linx32`) | ✅ | `./avs/compiler/linx-llvm/tests/run.sh` |
| AVS runtime suites | ✅ | `./avs/qemu/run_tests.sh --all` |
| Strict system gate | ✅ | `./avs/qemu/check_system_strict.sh` |
| Main regression | ✅ | `bash tools/regression/run.sh` |
| Linux initramfs smoke/full | ❌ | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py`; `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` |
| musl `M1` | ✅ | `MODE=phase-b /Users/zhoubot/linx-isa/lib/musl/tools/linx/build_linx64_musl.sh` |
| musl `M2` | ✅ (phase-b strict) | `out/libc/musl/logs/phase-b-summary.txt` |
| musl `M3` | ⚠ blocked (PIC relocations; secondary unresolved runtime symbols) | `out/libc/musl/logs/phase-b-m3-shared.log`; `out/libc/musl/logs/phase-b-m3-shared-notext-probe.log`; `out/libc/musl/logs/phase-b-m3-blockers.md` |
| musl runtime `R1` | ✅ | `avs/qemu/out/musl-smoke/compile.log` |
| musl runtime `R2` | ✅ | `avs/qemu/out/musl-smoke/qemu.log` |

## Latest command log

- `MODE=phase-b /Users/zhoubot/linx-isa/lib/musl/tools/linx/build_linx64_musl.sh` ✅ (`M1/M2` pass, `M3` blocked with classified primary/secondary blockers)
- `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --mode phase-b` ✅ (`runtime_pass`)
- `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py` ❌
- `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` ❌
