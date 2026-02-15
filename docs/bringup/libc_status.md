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

## Latest verified state (2026-02-16)

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
- musl `a_cas`/`a_cas_p`: now implemented with a `swapw`-backed process-global lock in `arch/linx64` (non-atomic load/store CAS removed).
- musl sample compile/link `R1`: pass via `avs/qemu/run_musl_smoke.py`.
- musl runtime `R2`: pass (`MUSL_SMOKE_START` and `MUSL_SMOKE_PASS` observed).
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
