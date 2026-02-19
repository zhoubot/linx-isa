<p align="center">
  <img src="docs/architecture/isa-manual/src/images/linxisa-logo.svg" alt="LinxISA logo" width="220" />
</p>

<h1 align="center">Linx Instruction Set Architecture</h1>

<p align="center"><strong>Public workspace for the block-structured LinxISA specification, AVS validation, and ecosystem submodule pinning.</strong></p>

## Overview

This repository is now submodule-first for implementation domains and AVS-centric for test content.
In-repo focus is:

- ISA sources and generators
- AVS test suites and validation matrix
- bring-up docs and navigation
- submodule pinning of ecosystem repos

## Canonical Sources

- ISA golden sources: `isa/v0.3/`
- ISA compiled catalog: `isa/v0.3/linxisa-v0.3.json`
- ISA generated codecs: `isa/generated/codecs/`
- ISA manual sources: `docs/architecture/isa-manual/`
- Assembly sample pack: `docs/reference/examples/v0.3/`

## Submodules

- `compiler/llvm` -> `git@github.com:LinxISA/llvm-project.git`
- `emulator/qemu` -> `git@github.com:LinxISA/qemu.git`
- `kernel/linux` -> `git@github.com:LinxISA/linux.git`
- `rtl/LinxCore` -> `git@github.com:LinxISA/LinxCore.git`
- `tools/pyCircuit` -> `git@github.com:LinxISA/pyCircuit.git`
- `lib/glibc` -> `git@github.com:LinxISA/glibc.git`
- `lib/musl` -> `git@github.com:LinxISA/musl.git`
- `workloads/pto_kernels` -> `https://github.com/LinxISA/PTO-Kernel.git`

Clone + init:

```bash
git clone --recurse-submodules git@github.com:LinxISA/linx-isa.git
cd linx-isa
git submodule sync --recursive
git submodule update --init --recursive
```

## Validation Entrypoints

- AVS runtime suites: `avs/qemu/run_tests.sh`
- AVS compile suites: `avs/compiler/linx-llvm/tests/run.sh`
- Main regression gate: `tools/regression/run.sh`
- Full-stack regression: `tools/regression/full_stack.sh`
- Strict cross-repo gate: `tools/regression/strict_cross_repo.sh`

## Repository Layout

- `avs/`: architectural verification suite (runtime + compile)
- `compiler/`: LLVM submodule only
- `emulator/`: QEMU submodule only
- `kernel/`: Linux submodule
- `rtl/`: LinxCore submodule + RTL notes
- `tools/`: generators, pyCircuit submodule, regression scripts
- `workloads/`: benchmark sources + runners + `pto_kernels` submodule mirror
- `isa/`: ISA specification assets
- `docs/`: bring-up, architecture, migration, navigation

## Navigation

- Canonical navigation guide: `docs/project/navigation.md`
- Agent routing policy: `AGENTS.md`
- Migration map for this layout: `docs/migration/path-map-v0.4.0.md`
- Bring-up gate truth table: `docs/bringup/GATE_STATUS.md`
