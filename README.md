<p align="center">
  <img src="docs/architecture/isa-manual/src/images/linxisa-logo.svg" alt="LinxISA logo" width="220" />
</p>

<h1 align="center">Linx Instruction Set Architecture</h1>

<p align="center"><strong>Public v0.3 repository for the LinxISA specification, tooling, and implementations.</strong></p>

## Overview

LinxISA is a specification-first ISA project with aligned compiler, emulator, and RTL implementation tracks.
The public tree is v0.3-only and uses canonical `spec/` + `impl/` layout.

## Canonical v0.3 Sources

- ISA golden sources: `spec/isa/golden/v0.3/`
- ISA compiled catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- ISA generated codecs: `spec/isa/generated/codecs/`
- Manual sources: `docs/architecture/isa-manual/`
- Public assembly samples: `examples/assembly/v0.3/`

## Workspace Dependencies (Submodules)

The bring-up workspace pins external implementation repositories as git submodules:

- `extern/qemu` -> `git@github.com:LinxISA/qemu.git`
- `extern/linux` -> `git@github.com:LinxISA/linux.git`
- `extern/pyCircuit` -> `git@github.com:zhoubot/pyCircuit.git` (temporary until org transfer)

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
git submodule set-url extern/pyCircuit git@github.com:LinxISA/pyCircuit.git
git submodule sync --recursive
git submodule update --init extern/pyCircuit
```

## Quick Start

```bash
bash tools/regression/run.sh
```

Optional tool overrides:

```bash
export CLANG=~/llvm-project/build-linxisa-clang/bin/clang
export LLD=~/llvm-project/build-linxisa-clang/bin/ld.lld
export QEMU=~/linx-isa/extern/qemu/build-tci/qemu-system-linx64
bash tools/regression/run.sh
```

## Bring-up Onboarding

See the public contributor onboarding guide:

- `docs/bringup/GETTING_STARTED.md`

## Repository Layout

- `spec/`: ISA specification assets
- `impl/`: compiler, emulator, RTL, models, and toolchain implementation assets
- `examples/`: canonical public examples and sample packs
- `docs/`: manual, architecture, bring-up, and migration docs
- `tools/`: generators, validators, and regression tooling
- `tests/`: runtime and integration test suites
- `extern/`: pinned bring-up dependencies as git submodules

## Migration and Compatibility

v0.3.0 includes one-release compatibility shims for old top-level paths (`isa/`, `compiler/`, `emulator/`, etc.).
New code must use canonical `spec/` and `impl/` paths.

Migration map: `docs/migration/path-map-v0.3.0.md`
