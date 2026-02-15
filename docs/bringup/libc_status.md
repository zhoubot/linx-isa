# Linx libc Bring-up Status

Canonical libc sources:

- `lib/glibc`
- `lib/musl`

## Repositories and pins

- `lib/glibc` @ `b87a5e30608a7e00aadef9eee035a32ee0611dbf`
- `lib/musl` @ `e1d149303fa91eedcc2beeeb1544502ec7c7b4b3`

## Current policy

- Bring-up deltas live in fork history (`LinxISA/glibc`, `LinxISA/musl`).
- This repository provides orchestration, runtime smoke, and status tracking.

## Latest verified state (2026-02-15)

- glibc `G1`: blocked (`working alias attribute support required` in current Linx clang flow).
- musl `M1`: pass (`configure` accepts `linx64-unknown-linux-musl`).
- musl `M2`: pass in `phase-b` (strict, no temporary excludes).
- musl `M3`: attempted; blocked by shared-link PIC relocations (`R_LINX_32`, `R_LINX_HL_PCR29_*`).
  - `-z notext` diagnostic probe additionally shows unresolved runtime symbols (`__add*`, `__sub*`, `__mul*`, `__div*`, `setjmp/longjmp`, `__syscall_cp_*`).
  - blocker report: `out/libc/musl/logs/phase-b-m3-blockers.md`
- musl sample compile/link `R1`: pass via `avs/qemu/run_musl_smoke.py`.
- musl runtime `R2`: pass (`MUSL_SMOKE_START` and `MUSL_SMOKE_PASS` observed).

## Baseline artifacts

- snapshot + dirty state:
  - `out/libc/musl/logs/baseline.md`
- baseline Linux boot failure logs:
  - `out/libc/musl/logs/linux-initramfs-smoke.latest.err`
  - `out/libc/musl/logs/linux-initramfs-full.latest.err`
