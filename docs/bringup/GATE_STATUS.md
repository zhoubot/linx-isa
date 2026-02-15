# Bring-up Gate Status (Canonical)

Last verified: `2026-02-16 02:49:19 CST` (`2026-02-15 18:49:19Z`)

This file is the canonical command/result table for current bring-up gates across library, compiler, and kernel.

| Domain | Gate | Command | Result | Evidence |
| --- | --- | --- | --- | --- |
| Library | glibc `G1` | `bash /Users/zhoubot/linx-isa/lib/glibc/tools/linx/build_linx64_glibc.sh` | ⚠ partial pass (`configure` + `csu/subdir_lib`) | `out/libc/glibc/logs/summary.txt`; `out/libc/glibc/logs/02-configure.log`; `out/libc/glibc/logs/03-make.log` |
| Library | musl `M1/M2/M3` | `MODE=phase-b /Users/zhoubot/linx-isa/lib/musl/tools/linx/build_linx64_musl.sh` | ✅ pass | `out/libc/musl/logs/phase-b-summary.txt` |
| Library | musl runtime `R1/R2` | `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --mode phase-b` | ✅ pass | `avs/qemu/out/musl-smoke/summary.json` |
| Compiler | AVS compile suites | `bash /Users/zhoubot/linx-isa/avs/compiler/linx-llvm/tests/run.sh` | ✅ pass | `avs/compiler/linx-llvm/tests/out` |
| Emulator/QEMU | strict system gate | `bash /Users/zhoubot/linx-isa/avs/qemu/check_system_strict.sh` | ✅ pass | terminal/log output |
| Kernel (OS) | Linux initramfs smoke | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py` | ✅ pass | terminal/log output |
| Kernel (OS) | Linux initramfs full boot | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` | ✅ pass | terminal/log output |

## Notes

- `glibc G1` alias-attribute configure blocker is resolved in the current scripted flow.
- `glibc G1` still needs an explicit full shared-`libc.so` gate in addition to `csu/subdir_lib`.
- Linux initramfs signal applets still use fallback markers in bring-up (`sigill: ok`, `sigsegv: ok`) while signal-return hardening is completed.
