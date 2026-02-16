# Linx libc Bring-up Status

Canonical libc sources:

- `lib/glibc`
- `lib/musl`

## Repositories and pins

- `lib/glibc` @ `3cb936060f896ef88f888682a3159a73db1fe411`
- `lib/musl` @ `b761931e63b6cd6fdbbc7269d07ccd9e4fec20b2`

## Current policy

- Bring-up deltas live in fork history (`LinxISA/glibc`, `LinxISA/musl`).
- This repository provides orchestration, runtime smoke, and status tracking.

## Latest verified state (2026-02-16 18:17:55 CST / 2026-02-16 10:17:55Z)

- glibc `G1`: partial pass.
  - `configure` pass with `lib/glibc/tools/linx/build_linx64_glibc.sh`.
  - `csu/subdir_lib` pass and `out/libc/glibc/build/csu/crt1.o` produced.
  - alias-attribute blocker is resolved in the scripted flow (no `working alias attribute support required`).
  - remaining gap: full shared `libc.so` gate is not yet proven in this script's default target set.
- musl `M1`: pass (`configure` accepts `linx64-unknown-linux-musl`).
- musl `M2`: pass in `phase-b` (strict, no temporary excludes).
- musl `M3`: pass in `phase-b` (shared `lib/libc.so` builds successfully).
  - gate evidence: `out/libc/musl/logs/phase-b-summary.txt` (`m3=pass`)
  - build log: `out/libc/musl/logs/phase-b-m3-shared.log`
- musl `a_cas`/`a_cas_p`: lock-free `LR/SC` CAS loops in `arch/linx64/atomic_arch.h` (global lockword shim removed).
- musl sample compile/link `R1`: pass for both static and shared sample builds via `avs/qemu/run_musl_smoke.py`.
- musl runtime `R2-static` (local strict QEMU): fail (`malloc_printf_static_runtime_block_trap`).
  - command: `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --qemu /Users/zhoubot/linx-isa/emulator/qemu/build/qemu-system-linx64 --link static --sample malloc_printf --timeout 45`
  - evidence: `avs/qemu/out/musl-smoke/summary.json`
  - trap detail: first trap `[linx trap] n=0x5 c=0x1 a=0x0000000000083c9c pc=0x0000000000084134`
- musl runtime `R2-shared`: fail (`shared_runtime_kernel_panic`).
  - evidence: `avs/qemu/out/musl-smoke/summary_shared.json`
  - qemu guest error: `invalid branch target 0x0 (not a block start marker)`
- musl call/ret runtime matrix (`Linux+musl`, `--link both`): fail at static stage (`callret_static_runtime_block_trap`).
  - command: `python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --sample callret --link both --timeout 120`
  - evidence: `avs/qemu/out/musl-smoke/summary.json`
  - trap detail: `[linx trap] n=0x5 c=0x1 a=0x0000000000083c9c pc=0x0000000000083cb0`
- freestanding call/ret conformance suite: pass.
  - command: `python3 /Users/zhoubot/linx-isa/avs/qemu/run_tests.py --suite callret --timeout 20`
  - includes runtime checks for volatile musttail-indirect rebinding and frame-heavy return integrity.
- strict call/ret contract-negative suite: pass (expanded to 11 contract cases).
  - command: `python3 /Users/zhoubot/linx-isa/avs/qemu/run_callret_contract.py --timeout 5`
- compile-only call/ret relocation + template gate: pass with workspace clang.
  - command: `OUT_DIR=/tmp/linx-callret-rerun bash /Users/zhoubot/linx-isa/avs/compiler/linx-llvm/tests/run.sh`
  - evidence: `33`-`38` pass relocation checks and `check_callret_templates.py` checks.
- Linux cross-stack call-target marker audit: pass.
  - command: `bash /Users/zhoubot/linx-isa/tools/ci/check_linx_callret_crossstack.sh`
  - evidence: terminal output `PASS: Linx Linux call/ret cross-stack audit passed` (default mode accepts resolved local fused targets; strict relocation mode remains available via `LINX_STRICT_CALLRET_RELOCS=1`).
- workspace LLVM toolchain build (`compiler/llvm/build-linxisa-clang`) now compiles full AVS C matrix; previous assert-only blockers (`PSEUDO_RET` verifier and softfp `Invalid TRUNCATE`) are resolved.
- Linux no-libc initramfs baselines: pass.
  - `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/smoke.py`
  - `python3 /Users/zhoubot/linux/tools/linxisa/initramfs/full_boot.py`
  - note: `sigill_test`/`sigsegv_test` currently use bring-up fallback markers while Linux signal-return paths are stabilized.

## Baseline artifacts

- snapshot + dirty state:
  - `out/libc/musl/logs/baseline.md`
- baseline Linux boot failure logs:
  - `out/libc/musl/logs/linux-initramfs-smoke.latest.err`
  - `out/libc/musl/logs/linux-initramfs-full.latest.err`

## Cross-domain gate table

- Canonical command/result table: `docs/bringup/GATE_STATUS.md`.
