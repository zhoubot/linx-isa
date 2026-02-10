# LinxISA Bring-up Hierarchy (Canonical)

This directory is the canonical bring-up plan hierarchy for the repository.

The hierarchy now follows the execution path to Linux-on-FPGA:

1. C/compiler bring-up
2. ISA source-of-truth integration
3. Emulator execution bring-up
4. RTL alignment and verification (pyCircuit-driven)
5. FPGA platform bring-up (ZYBO Z7-20)
6. Linux bring-up on FPGA (Linx first, Janus final)
7. Toolchain/libc support and closure

RTL development is planned to be authored in `/Users/zhoubot/pyCircuit` (external), generating C++ cycle models and
Verilog/SV for integration and validation in this repo.

## Phase documents

- `docs/bringup/phases/01_compiler.md`
- `docs/bringup/phases/02_isa_spec.md`
- `docs/bringup/phases/03_emulator_qemu.md`
- `docs/bringup/phases/04_rtl.md`
- `docs/bringup/phases/05_fpga_zybo_z7.md`
- `docs/bringup/phases/06_linux_on_janus.md`
- `docs/bringup/phases/07_toolchain_glibc.md`
- `docs/bringup/PROGRESS.md`

## Contracts

- `docs/bringup/contracts/pyc_artifact_contract.md`
- `docs/bringup/contracts/trace_schema.md`
- `docs/bringup/contracts/fpga_platform_contract.md`
- `docs/bringup/contracts/linxisa_v0_2_profile_lock.md`

## Scope boundary

The following top-level directories are useful support surfaces, but are not separate bring-up phases:

- `workloads/` (performance measurement and example programs)
- `tests/qemu/` (runtime tests used by emulator/toolchain bring-up)
- `tests/scratch/` (ad hoc tests and scratch programs)
- `tools/` (spec/codegen/regression utilities)
