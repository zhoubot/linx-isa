# Phase 1: Compiler Bring-up

Primary plan/status file: `impl/compiler/COMPILER_PLAN.md`

## Current checkpoint

- Compiler binary in use: `~/llvm-project/build-linxisa-clang/bin/clang`
- Supported targets: `linx64-linx-none-elf`, `linx32-linx-none-elf`
- Compile test suite: `impl/compiler/llvm/tests/`

## Required invariants

- Generated code must follow Block ISA control-flow invariants.
- Encodings and decode assumptions must match `spec/isa/spec/current/linxisa-v0.3.json`.
- Call header adjacency rule must hold for direct calls.

## Execution

```bash
CLANG=~/llvm-project/build-linxisa-clang/bin/clang ./impl/compiler/llvm/tests/run.sh
```
