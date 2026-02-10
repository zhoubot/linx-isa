# Phase 1: Compiler Bring-up

Primary plan/status file: `compiler/COMPILER_PLAN.md`

## Current checkpoint

- Compiler binary in use: `~/llvm-project/build-linxisa-clang/bin/clang`
- Supported targets: `linx64-linx-none-elf`, `linx32-linx-none-elf`
- Compile test suite: `compiler/llvm/tests/`

## Required invariants

- Generated code must follow Block ISA control-flow invariants.
- Encodings and decode assumptions must match `isa/spec/current/linxisa-v0.2.json`.
- Call header adjacency rule must hold for direct calls.

## Execution

```bash
CLANG=~/llvm-project/build-linxisa-clang/bin/clang ./compiler/llvm/tests/run.sh
```
