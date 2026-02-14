<p align="center">
  <img src="docs/architecture/isa-manual/src/images/linxisa-logo.svg" alt="LinxISA logo" width="220" />
</p>

<h1 align="center">Linx Instruction Set Architecture</h1>

<p align="center"><strong>Public repository for the LinxISA specification, bring-up tooling, and implementation tracks.</strong></p>

## Overview

LinxISA is a specification-first ISA project with aligned compiler, emulator, kernel, model, and RTL workstreams.
The public tree is v0.3+ and uses a top-level domain layout (no `impl/` root).

## Canonical ISA Sources

- ISA golden sources: `spec/isa/golden/v0.3/`
- ISA compiled catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- ISA generated codecs: `spec/isa/generated/codecs/`
- Manual sources: `docs/architecture/isa-manual/`
- Public assembly samples: `examples/assembly/v0.3/`

## Workspace Submodules

The workspace pins upstream dependencies as domain submodules:

- `compiler/llvm` -> `git@github.com:LinxISA/llvm-project.git`
- `emulator/qemu` -> `git@github.com:LinxISA/qemu.git`
- `kernel/linux` -> `git@github.com:LinxISA/linux.git`
- `models/pyCircuit` -> `git@github.com:zhoubot/pyCircuit.git` (temporary until org transfer)

LinxISA in-repo integration assets stay separate from upstream mirrors:

- LLVM integration/tests: `compiler/linx-llvm/`
- QEMU integration patches/scripts: `emulator/linx-qemu/`

Fresh clone:

```bash
git clone --recurse-submodules git@github.com:LinxISA/linx-isa.git
cd linx-isa
git submodule update --init --recursive
```

With GitHub CLI:

```bash
gh repo clone LinxISA/linx-isa -- --recurse-submodules
cd linx-isa
git submodule update --init --recursive
```

If you already cloned this repo:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

When `LinxISA/pyCircuit` becomes available, switch the submodule URL:

```bash
git submodule set-url models/pyCircuit git@github.com:LinxISA/pyCircuit.git
git submodule sync --recursive
git submodule update --init models/pyCircuit
```

## Quick Start

```bash
bash tools/regression/run.sh
```

Optional tool overrides:

```bash
export CLANG=~/llvm-project/build-linxisa-clang/bin/clang
export LLD=~/llvm-project/build-linxisa-clang/bin/ld.lld
export QEMU=~/linx-isa/emulator/qemu/build-tci/qemu-system-linx64
bash tools/regression/run.sh
```

## Bring-up Onboarding

- Contributor guide: `docs/bringup/GETTING_STARTED.md`

## Repository Layout

- `spec/`: ISA specification assets
- `compiler/`: compiler plans, Linx integration assets, and LLVM submodule
- `kernel/`: kernel integrations and Linux submodule
- `emulator/`: emulator integrations and QEMU submodule
- `models/`: model integrations and pyCircuit submodule
- `rtl/`: RTL notes and integration wrappers
- `toolchain/`: libc/binutils/pto support code
- `examples/`: canonical public sample packs
- `docs/`: architecture, bring-up, migration, and references
- `tools/`: generators, validators, and regression tooling
- `tests/`: runtime and integration tests

## Migration

Latest path migration map: `docs/migration/path-map-v0.3.1.md`
