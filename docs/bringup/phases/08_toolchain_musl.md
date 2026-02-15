# Phase 8: Toolchain/musl Bring-up

Canonical source repository:

- `lib/musl` (`git@github.com:LinxISA/musl.git`)

## Objective

Bring up a reproducible Linx musl path for:

- `linx64-unknown-linux-musl` (`M1/M2/M3`)
- Linux initramfs runtime smoke with a real C program using `malloc/free/printf` (`R1/R2`)

## Entry points

- musl build entrypoint:
  - `lib/musl/tools/linx/build_linx64_musl.sh`
- runtime harness:
  - `avs/qemu/run_musl_smoke.py`
- runtime sample program:
  - `avs/qemu/tests/linux_musl_malloc_printf.c`

## Default artifact layout

- musl build/install/logs:
  - `out/libc/musl/build`
  - `out/libc/musl/install`
  - `out/libc/musl/logs`
- smoke outputs:
  - `avs/qemu/out/musl-smoke/initramfs.cpio`
  - `avs/qemu/out/musl-smoke/musl_smoke`
  - `avs/qemu/out/musl-smoke/qemu.log`
  - `avs/qemu/out/musl-smoke/summary.json`

## Modes

- `phase-a`:
  - allows temporary TU exclusions in `arch/linx64/arch.mak`
  - records active excludes and crash signature in `out/libc/musl/logs/phase-a-exclusions.md`
- `phase-b`:
  - strict mode (`LINX_MUSL_MODE=phase-b`)
  - no temporary excludes allowed

## Commands

Build musl (`M1/M2/M3`):

```bash
cd /Users/zhoubot/linx-isa/lib/musl
MODE=phase-b ./tools/linx/build_linx64_musl.sh
```

Run end-to-end smoke (`R1/R2`):

```bash
cd /Users/zhoubot/linx-isa
python3 avs/qemu/run_musl_smoke.py --mode phase-b
```

## Current status (2026-02-15)

- `M1`: pass.
- `M2`: pass in `phase-b` (strict, no temporary excludes).
- `M3`: attempted; currently blocked by shared-link PIC relocation policy.
  - primary blocker: `R_LINX_32` / `R_LINX_HL_PCR29_{LOAD,STORE}` in shared link.
  - secondary `-z notext` probe reveals unresolved runtime symbols (`__add*`, `__sub*`, `__mul*`, `__div*`, `setjmp/longjmp`, `__syscall_cp_*`).
  - blocker report: `out/libc/musl/logs/phase-b-m3-blockers.md`
- `R1`: pass (sample compiles/links statically with musl sysroot + local builtins fallback objects).
- `R2`: pass (`MUSL_SMOKE_START` and `MUSL_SMOKE_PASS` observed in `avs/qemu/out/musl-smoke/qemu.log`).

## Baseline repro pointers

- baseline freeze:
  - `out/libc/musl/logs/baseline.md`
- latest Linux userspace boot failures:
  - `out/libc/musl/logs/linux-initramfs-smoke.latest.err`
  - `out/libc/musl/logs/linux-initramfs-full.latest.err`

## Exit criteria

- `M1/M2` pass in strict mode (`phase-b`) with no temporary excludes.
- `M3` either passes or has bounded blocker with owner + repro.
- runtime sentinels are observed under QEMU:
  - `MUSL_SMOKE_START`
  - `MUSL_SMOKE_PASS`
