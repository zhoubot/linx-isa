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

## Quick Start

```bash
bash tools/regression/run.sh
```

Optional tool overrides:

```bash
export CLANG=~/llvm-project/build-linxisa-clang/bin/clang
export LLD=~/llvm-project/build-linxisa-clang/bin/ld.lld
export QEMU=~/qemu/build-tci/qemu-system-linx64
bash tools/regression/run.sh
```

## Repository Layout

- `spec/`: ISA specification assets
- `impl/`: compiler, emulator, RTL, models, and toolchain implementation assets
- `examples/`: canonical public examples and sample packs
- `docs/`: manual, architecture, bring-up, and migration docs
- `tools/`: generators, validators, and regression tooling
- `tests/`: runtime and integration test suites

## Migration and Compatibility

v0.3.0 includes one-release compatibility shims for old top-level paths (`isa/`, `compiler/`, `emulator/`, etc.).
New code must use canonical `spec/` and `impl/` paths.

Migration map: `docs/migration/path-map-v0.3.0.md`
