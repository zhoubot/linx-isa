# Linx-AVS (Architecture Validation Suite)

This folder defines the **Linx Architecture Validation Suite (Linx-AVS)**: a
conformance-style test plan and test matrix intended to drive Linx ISA bring-up
to a maturity bar comparable to established ISAs (Arm/x86/RISC-V).

This repository currently runs:

- compiler compile-only tests (`./impl/compiler/llvm/tests/`)
- QEMU runtime tests (`./tests/qemu/`)
- a small benchmark harness (`./workloads/benchmarks/`)

Linx-AVS adds:

- a normative test matrix (`matrix_v1.md`)
- a place to track implementation status and required evidence
- (future) a shared runner format for QEMU vs RTL difftest

## Scope (v1)

`matrix_v1.md` covers:

- scalar ISA correctness (integer, bit ops, shifts, control flow)
- Block ISA invariants (block boundaries, safety rule, call-header adjacency)
- memory operations (alignment, addressing modes)
- fences and atomic qualifiers (`.aq/.rl`) at the architectural contract level
- privileged entry points (`ACRC/ACRE`) as behavioral contracts (full OS/MMU is v2)

Vector/tile/accelerator surfaces are tracked but not required for v1 pass unless
the selected profile enables them.

## How to Use

- Treat `matrix_v1.md` as the source of truth for "what must be tested".
- Map each test ID to:
  - a directed unit test under `./tests/qemu/tests/` (runtime)
  - or a compiler lit-like test (compile-only)
  - or a differential trace test (QEMU vs RTL) when the subset is supported

