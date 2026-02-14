# LinxISA Gap Analysis (v0.2 -> Tier-1)

Last updated: 2026-02-07

This document summarizes what is currently missing or immature in LinxISA (spec,
toolchain, emulator, validation), flags known inconsistencies, and maps each
gap to concrete artifacts and bring-up gates in this repo.

Primary roadmap: `docs/bringup/MATURITY_PLAN.md`.

## What "Tier-1" Means Here

Comparable to Arm/x86/RISC-V maturity means:

- A spec that is unambiguous and machine-checkable (encoding + semantics + traps).
- A impl/compiler/toolchain that is correct, well-tested, and debuggable.
- An emulator that is correct, deterministic, observable, and useful as a reference model.
- A validation suite (AVS) with clear pass/fail criteria and coverage gates.

## Current Strengths (Evidence in Repo)

- Golden opcode database and generated JSON catalog:
  - `spec/isa/golden/v0.3/` -> `spec/isa/spec/current/linxisa-v0.3.json`
- Encoding conflict checks and hole reporting:
  - `python3 tools/isa/report_encoding_space.py --check`
  - Report: `docs/reference/encoding_space_report.md`
- LLVM backend has full mnemonic disassembly coverage when using the per-target outputs:
  - `python3 impl/compiler/llvm/tests/analyze_coverage.py --out-dir impl/compiler/llvm/tests/out-linx64 --fail-under 100`
- Benchmark harness exists with static and dynamic instruction statistics:
  - `python3 workloads/benchmarks/run_benchmarks.py --dynamic-hist`

## Gaps: ISA Specification

- Privileged architecture completeness:
  - Missing or under-specified: trap causes, trap priority, precise state capture,
    CSR reset values, and which fields are WARL vs fixed.
  - Required artifacts:
    - Expand manual: `docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc`
    - AVS coverage: `avs/matrix_v1.md` (SYS area)

- MMU and memory attributes:
  - No complete, testable definition of translation, page tables, TLB invalidation,
    access faults, and cacheability attributes.
  - Bring-up strategy:
    - Define a "no-MMU" profile gate first (current), then a minimal MMU profile.

- Debug architecture:
  - No spec for single-step, breakpoints/watchpoints, debug register access rules,
    and privilege interactions.
  - Bring-up strategy:
    - Start with GDB remote support in QEMU + a minimal debug CSR contract.

- Vector/tile semantic envelope:
  - Block legality rules need to be explicit and tested (what is legal in VPAR vs VSEQ,
    and what traps on misuse).
  - AVS vector tests: `AVS-VEC-*` in `avs/matrix_v1.md` and `avs/linx_avs_v1_test_matrix.yaml`.

## Gaps: Memory Model and Fences

- The repo now documents a weak model, but many "sharp edges" still need closure:
  - Device vs Normal ordering, cumulative fence intent, and exact `.aq/.rl` scope.
  - Reference chapter: `docs/architecture/isa-manual/src/chapters/08_memory_operations.adoc`
  - Required validation:
    - Litmus tests for message passing and fence ordering (AVS-ATOM-010/011).

## Gaps: Toolchain (LLVM/Asm/Obj)

- ABI contract stability:
  - Register roles, stack alignment, varargs, TLS, and unwind/debug info must be
    formally documented and tested.
  - Required artifacts:
    - ABI doc under `docs/architecture/` (future)
    - AVS ABI tests (AVS-ABI-001/002)

- Object/relocation model:
  - ET_DYN/dynamic linking is not yet a bring-up gate; relocation coverage must be
    expanded before hosted workloads (LLVM test-suite) can run.
  - Evidence: `impl/compiler/llvm/tests/run.sh` has PIC relocation checks, shared-lib gated.

- Disasm coverage gate ergonomics:
  - Stale `impl/compiler/llvm/tests/out/` directories can cause false failures if used.
  - Mitigation: `impl/compiler/llvm/tests/analyze_coverage.py` auto-detects `out-linx*`.

## Gaps: Emulator (QEMU)

- Execution completeness:
  - Any spec-defined instruction not implemented in QEMU must trap deterministically
    as illegal, not silently execute or decode as a different instruction.
  - AVS gate: AVS-DEC-001, AVS-EMU-001.

- Observability and difftest:
  - A stable commit-trace schema must be emitted by QEMU and consumed by RTL difftest.
  - Contract: `docs/bringup/contracts/trace_schema.md`

## Gaps: Validation (Linx-AVS)

- The matrix exists but many tests are not implemented yet:
  - Matrix: `avs/matrix_v1.md`
  - Machine-readable: `avs/linx_avs_v1_test_matrix.yaml`
  - Required work:
    - Implement runtime tests under `tests/qemu/`
    - Implement compile-only / MC tests under `impl/compiler/llvm/tests/`

## Gaps: Benchmark Portfolio

- Third-party sources exist, but only a freestanding-friendly subset is expected
  to run early:
  - Fetch script: `workloads/benchmarks/fetch_third_party.sh`
  - Method: `workloads/benchmarks/BENCHMARKING_METHOD.md`
  - Reality:
    - Hosted workloads (full LLVM test-suite, Google Benchmark) require more libc
      and OS services than the current minimal environment provides.

## Concrete "Fill The Missing Parts" Checklist

- Spec:
  - Close all normative TODOs for scalar core semantics and traps.
  - Define privileged traps/CSR behavior with reset values and illegal/WARL rules.
  - Define memory model litmus expectations and fence semantics precisely.

- Toolchain:
  - Lock ABI and add ABI tests (calls, varargs, stack alignment).
  - Expand relocation/object model and enable ET_DYN when ready.
  - Keep disasm/asm round-trip tests and coverage at 100%.

- Emulator:
  - Implement missing instructions or trap deterministically.
  - Emit commit-trace for difftest and keep histogram tooling stable.

- Validation:
  - Implement AVS tests and treat them as the primary bring-up gate.
