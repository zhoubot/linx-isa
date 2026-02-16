# Bring-up Gate Status (Canonical)

Last verified: `2026-02-16 18:17:55 CST` (`2026-02-16 10:17:55Z`)

This file is the canonical command/result table for current bring-up gates across library, compiler, and kernel.

| Domain | Gate | Command | Result | Evidence |
| --- | --- | --- | --- | --- |
| Library | glibc `G1` | `bash /Users/zhoubot/linx-isa/lib/glibc/tools/linx/build_linx64_glibc.sh` | ⚠ partial pass (`configure` + `csu/subdir_lib`) | `out/libc/glibc/logs/summary.txt`; `out/libc/glibc/logs/02-configure.log`; `out/libc/glibc/logs/03-make.log` |
| Library | musl `M1/M2/M3` | `MODE=phase-b /Users/zhoubot/linx-isa/lib/musl/tools/linx/build_linx64_musl.sh` | ✅ pass | `out/libc/musl/logs/phase-b-summary.txt` |
| Library | musl runtime `R2-static` | `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --qemu /Users/zhoubot/linx-isa/emulator/qemu/build/qemu-system-linx64 --mode phase-b --link static --sample malloc_printf --timeout 45` | ❌ fail (`malloc_printf_static_runtime_block_trap`) | `avs/qemu/out/musl-smoke/summary.json`; `avs/qemu/out/musl-smoke/qemu_malloc_printf_static.log` |
| Library | musl runtime `R2-shared` | `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --mode phase-b --link shared --timeout 25` | ❌ fail (`shared_runtime_kernel_panic`) | `avs/qemu/out/musl-smoke/summary_shared.json`; `avs/qemu/out/musl-smoke/qemu_shared.log` |
| Library | musl runtime call/ret matrix (`Linux+musl`) | `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --sample callret --link both --timeout 120` | ❌ fail (`callret_static_runtime_block_trap`) | `avs/qemu/out/musl-smoke/summary.json`; `avs/qemu/out/musl-smoke/qemu_callret_static.log` |
| Compiler | AVS compile suites | `OUT_DIR=/tmp/linx-callret-rerun bash /Users/zhoubot/linx-isa/avs/compiler/linx-llvm/tests/run.sh` | ✅ pass | terminal output (`33`-`38` relocation + template checks pass) |
| Compiler+Emulator | freestanding call/ret suite | `python3 /Users/zhoubot/linx-isa/avs/qemu/run_tests.py --suite callret --timeout 20` | ✅ pass | `avs/qemu/out/linx-qemu-tests.o` + terminal output |
| Emulator/QEMU | call/ret contract-negative traps | `python3 /Users/zhoubot/linx-isa/avs/qemu/run_callret_contract.py --timeout 5` | ✅ pass (11-case matrix) | `avs/qemu/out/callret-contract` |
| Emulator/QEMU | strict system gate | `bash /Users/zhoubot/linx-isa/avs/qemu/check_system_strict.sh` | ✅ pass | terminal/log output |
| Kernel (OS) | Linux initramfs smoke | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py` | ✅ pass | terminal/log output |
| Kernel (OS) | Linux initramfs full boot | `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py` | ✅ pass | terminal/log output |
| Kernel+Compiler | call-target marker audit (`cross-stack`) | `bash /Users/zhoubot/linx-isa/tools/ci/check_linx_callret_crossstack.sh` | ✅ pass | terminal output (`PASS: Linx Linux call/ret cross-stack audit passed`) |
| Repository | Layout policy | `bash /Users/zhoubot/linx-isa/tools/ci/check_repo_layout.sh` | ✅ pass | terminal output (`OK: repository layout policy passed`) |

## Notes

- `glibc G1` alias-attribute configure blocker is resolved in the current scripted flow.
- `glibc G1` still needs an explicit full shared-`libc.so` gate in addition to `csu/subdir_lib`.
- freestanding contract negatives now cover eleven cases (bad targets, missing `setc.tgt`, invalid `SETRET` sequencing) and pass on local strict QEMU.
- freestanding call/ret positive suite now includes runtime musttail-indirect rebind and frame-heavy return checks (to catch direct-folded tail paths and stack-return corruption).
- compile-only call/ret suites now enforce both relocation coverage (`R_LINX_*BSTART*` + `R_LINX_*SETRET*`) and template shape (`FENTRY/FRET.STK` vs `FENTRY/FEXIT` for musttail); workspace clang gate is passing.
- musl shared runtime currently fails before `MUSL_SMOKE_PASS` with qemu-reported `invalid branch target 0x0 (not a block start marker)`.
- musl Linux runtime on local strict QEMU still fails with `LINX_EBLOCK_CAUSE_BAD_BRANCH_TARGET`:
  - first observed: `a=0x0000000000083c9c`, `pc=0x0000000000084134`
  - repeated later: `a=0x000000000001b6d4`, `pc=0x000000000001b3da`
- cross-stack audit now accepts resolved local fused call/setret targets by default and keeps strict relocation-only auditing available via `LINX_STRICT_CALLRET_RELOCS=1`.
- workspace LLVM toolchain build with assertions (`compiler/llvm/build-linxisa-clang`) now compiles full AVS C matrix; previous assert-only blockers (`PSEUDO_RET` verifier and softfp truncate assert) are resolved.
- Linux initramfs signal applets still use fallback markers in bring-up (`sigill: ok`, `sigsegv: ok`) while signal-return hardening is completed.
