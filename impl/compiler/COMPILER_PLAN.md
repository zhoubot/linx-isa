# Compiler Bring-up Plan (Canonical)

This document is the single source of truth for compiler bring-up status in this repo.

Last updated: 2026-02-07

## Bring-up hierarchy alignment

This plan follows the repository bring-up order:

1. Compiler (`impl/compiler/`)
2. ISA spec/catalog (`isa/`)
3. Emulator (`impl/emulator/`, `tests/qemu/`)
4. RTL (`impl/rtl/`)
5. Toolchain + libc (`impl/toolchain/`, `tools/glibc/`)

## Current status snapshot

- Host compiler binary used by tests: `~/llvm-project/build-linxisa-clang/bin/clang`
- Active backend targets: `linx64-linx-none-elf`, `linx32-linx-none-elf`
- Compile test suite in-repo: `impl/compiler/llvm/tests/c/01_*.c` through `31_*.c` (31 tests total)
- Cross-component regression entrypoint: `tools/regression/run.sh`

## Phases and progress

### Phase 1: Core codegen and MC plumbing

Status: In progress

Done:
- Spec-driven helpers in `tools/isa/` are present and used by bring-up scripts.
- Basic compile pipeline used by `impl/compiler/llvm/tests/run.sh` is operational.

Remaining:
- Close remaining backend gaps for full spec-driven instruction coverage.
- Keep instruction naming/encoding parity with `spec/isa/spec/current/linxisa-v0.3.json`.

### Phase 2: Block ISA invariants

Status: In progress

Done:
- Block start/boundary conventions are documented and used by bring-up workflows.

Remaining:
- Enforce/validate all invariants in automated tests:
  - target-to-block-start safety rule
  - call header adjacency (`BSTART CALL` + `SETRET`)
  - template block standalone behavior (`FENTRY`/`FEXIT`/`FRET.*`)

### Phase 3: Extended ISA groups (floating/atomic/memory/bitmanip/tile-oriented surface)

Status: In progress

Done:
- Extended compile tests exist (`20_` through `31_` programs).

Remaining:
- Expand compile and runtime validation so coverage reflects the current spec catalog.

### Phase 4: Variable-length and relocation correctness

Status: In progress

Done:
- Bring-up scripts and docs capture known relocation/codegen pitfalls.

Remaining:
- Continue hardening relocation/linker edge cases and long-range control-flow sequences.

### Phase 5: End-to-end regression quality bar

Status: In progress

Done:
- Regression driver exists: `tools/regression/run.sh`.
- Compile-only and QEMU runtime test entrypoints are in place.

Remaining:
- Keep regression green while broadening feature coverage.
- Track unresolved blockers in `docs/bringup/PROGRESS.md`.

## Validation commands

```bash
# Compile tests
CLANG=~/llvm-project/build-linxisa-clang/bin/clang ./impl/compiler/llvm/tests/run.sh

# QEMU runtime tests
./tests/qemu/run_tests.sh --all

# End-to-end regression
bash tools/regression/run.sh
```

## Latest validation results (2026-02-07)

- Regression: `env CLANG=~/llvm-project/build-linxisa-clang/bin/clang QEMU=~/qemu/build-tci/qemu-system-linx64 bash tools/regression/run.sh` (PASS)
- Compile tests: 31 compile tests for both `linx64` and `linx32` (PASS)
- ISA mnemonic coverage (from `impl/compiler/llvm/tests/analyze_coverage.py`):
  - Spec unique mnemonics: 700
  - Covered spec mnemonics: 700
  - Missing spec mnemonics: 0
  - Alias note: `BSTART`/`C.BSTART` and `*.STD` are treated as equivalent spellings for coverage.

Keeping LLVM aligned:

- Sync spec-driven opcode tables into `~/llvm-project` with `bash impl/toolchain/llvm/sync_generated_opcodes.sh`.
- Rebuild `llvm-objdump` (and friends) after syncing to keep disassembly coverage accurate.

## Ownership rule

When status changes, update this file and `docs/bringup/PROGRESS.md` in the same change.
