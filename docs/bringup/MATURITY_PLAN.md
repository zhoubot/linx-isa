# LinxISA Maturity Plan (Bring-up to "Tier-1" Quality)

Last updated: 2026-02-11

This document is a **concrete roadmap** to bring LinxISA to a maturity level
comparable to established ISAs (Arm/x86/RISC-V) across:

- ISA specification quality (unambiguous, executable, stable)
- compiler/toolchain maturity (correctness, diagnostics, optimizations)
- emulator maturity (correctness, debug, performance, determinism)
- validation maturity (conformance + differential + benchmarks)

This plan intentionally uses repository artifacts as gates and evidence sources.

## Definitions

- **ISA catalog (stable)**: `isa/spec/current/linxisa-v0.2.json` (generated from
  `isa/golden/v0.2/`).
- **ISA catalog (staged)**: `isa/spec/v0.3/linxisa-v0.3.json` (generated from
  `isa/golden/v0.3/`).
- **Block ISA invariants**: safety rule, commit-time control flow, template
  block behavior, call-header adjacency.
- **AVS**: architecture validation suite (see `avs/`).

## Maturity Levels

### Level 0: Bring-up usable (today)

Evidence in repo:

- `bash tools/regression/run.sh` passes.
- `./tests/qemu/check_system_strict.sh` passes (strict v0.2 ACR/debug resume + log-noise gate).
- `./tests/qemu/run_tests.sh --all` passes.
- `python3 tools/isa/check_no_legacy_v03.py --root .` passes (canonical v0.3 alias gate).
- `bash tools/pto/run_v03_pto_to_linx.sh` passes (PTO minimal compile flow to canonical v0.3 asm).
- Benchmarks run (CoreMark + Dhrystone).

### Level 1: Spec + emulator/compiler correctness baseline

Requirements:

- No normative TODOs for scalar core semantics.
- Encoding space is conflict-free and documented.
- Linx-AVS v1 scalar profiles pass on QEMU.
- Differential trace schema exists and is emitted by QEMU.

Evidence:

- `avs/matrix_v1.md` implemented for `LNX-S32/LNX-S64`.
- `tools/isa/` encodes conflict checks and encoding-space reporting.

### Level 2: System bring-up readiness (Linux on FPGA prerequisites)

Requirements:

- Privileged architecture (ACR/SSR/traps) is specified with precise behavior.
- Static linking and glibc bring-up are unblocked for a minimal Linux userspace.
- Loader/relocation model is complete for static and (later) ET_DYN.

Evidence:

- `tools/glibc/bringup_linx64.sh` passes at least through libc startup objects.
- QEMU supports required system instructions and devices.

### Level 3: Tier-1 developer experience

Requirements:

- Stable ABI + debug/unwind story.
- Robust toolchain packaging (sysroot, headers, assembler/disassembler).
- Benchmark portfolio beyond microbenchmarks and with repeatable methodology.

Evidence:

- Expanded `workloads/` harness with pinned sources and repeatable reports.
- CI-style scripts exist for correctness and performance regressions.

## Immediate Work Items (Next Agents)

- Implement Linx-AVS v1 tests under `tests/qemu/tests/` and/or compiler tests.
- Promote v0.3 from staged to current defaults after Janus/RTL parity evidence is captured.
- Wire difftest gate A3/B3 in `docs/bringup/PROGRESS.md` using
  `docs/bringup/contracts/trace_schema.md`.
- Close the known `-O2` miscompile noted in `workloads/benchmarks/README.md`.
- Define and document the memory model and fence semantics (see manual chapter
  `docs/architecture/isa-manual/src/chapters/08_memory_operations.adoc`).

## What Is Still Missing (to reach Arm/x86/RISC-V quality)

ISA / spec completeness:

- Privileged architecture needs a full, testable definition (trap causes,
  priority, precise state, CSR reset values, and what is WARL vs fixed).
- MMU story is not complete (page tables, TLB invalidation, access faults,
  cacheability attributes, address translation ordering).
- Debug architecture is not specified (single-step, breakpoints/watchpoints,
  register access rules, privilege interaction).
- Vector/tile architectural model needs full end-to-end alignment:
  - which instructions are valid only in `BSTART.VPAR` vs `BSTART.VSEQ`
  - what traps on misuse (illegal vs block-format error)
  - how memory ordering interacts with vector blocks

Encoding / decode rigor:

- Encoding conflicts must remain at 0:
  - Report: `docs/reference/encoding_space_report.md`
  - Check: `python3 tools/isa/report_encoding_space.py --check`
- "Empty space" is acceptable only if explicitly documented as reserved/unassigned:
  - Occupancy + hole ranges are tracked in `docs/reference/encoding_space_report.md`.

Toolchain maturity gaps:

- Disassembler coverage must stay at 100% of spec mnemonics:
  - `python3 compiler/llvm/tests/analyze_coverage.py --fail-under 100`
- ABI needs a stable, externally documented contract (register roles, stack
  alignment, varargs, TLS, unwind info).
- Relocation/object model needs a complete dynamic-linking story (ET_DYN,
  PLT/GOT, ifunc, and ASLR constraints).
- Assembler quality: accept the full canonical syntax (including V/TILE
  operand spellings) and emit accurate diagnostics.

Emulator maturity gaps:

- Coverage: decode/execute parity with the scalar spec and deterministic traps
  on any reserved encoding.
- Observability: stable trace schema for difftest and perf histograms.
- System model: enough devices/MMIO to boot real software stacks (UART, timers,
  interrupt controller, block-device/net, etc).

## Benchmarking Plan (C/C++)

Downloaded suites (pinned by `workloads/benchmarks/fetch_third_party.sh`):

- Embench IoT: `workloads/benchmarks/third_party/embench-iot/` ([project](https://github.com/embench/embench-iot))
- PolyBench/C: `workloads/benchmarks/third_party/PolyBenchC/` ([project](https://github.com/cavazos-lab/PolyBenchC))
- MiBench: `workloads/benchmarks/third_party/mibench/` ([mirror](https://github.com/embecosm/mibench))
- LLVM test-suite: `workloads/benchmarks/third_party/llvm-test-suite/` ([project](https://github.com/llvm/llvm-test-suite))
- Google Benchmark (microbench harness): `workloads/benchmarks/third_party/google-benchmark/` ([project](https://github.com/google/benchmark))

Methodology (repeatable and ISA-friendly):

1. Define build modes: `-O0`, `-O2`, `-O3`, `-Os`, plus LTO if supported.
2. Produce both static and dynamic metrics:
   - Static: `.text` size, instruction counts/histograms from `llvm-objdump`.
   - Dynamic: QEMU instruction counter + per-mnemonic histogram (already used
     by `workloads/benchmarks/run_benchmarks.py`).
3. Control sources of noise:
   - Pin input sizes/iterations, disable I/O where possible, warm caches for
     steady-state runs, record QEMU build hash and compiler revision.
4. Keep an apples-to-apples baseline:
   - Run the same benchmark sources on AArch64 and/or x86_64 (native) to catch
     harness bugs and to calibrate performance expectations.

## Tool Sync Rules (avoid drift)

- Spec generators in this repo are the source of truth for decode tables.
- After any ISA golden/spec change:
  1. Regenerate: `python3 tools/isa/gen_c_codec.py` and `python3 tools/isa/gen_qemu_codec.py`.
  2. Sync LLVM opcode tables: `bash toolchain/llvm/sync_generated_opcodes.sh`.
  3. Keep QEMU aligned via `emulator/qemu/patches/` (apply with
     `bash emulator/qemu/apply_patches.sh`).

## Benchmark Harness Entrypoints

- CoreMark + Dhrystone (static + dynamic stats):
  - `python3 workloads/benchmarks/run_benchmarks.py --dynamic-hist`
- Portfolio (adds ctuning codelets; optional PolyBench subset):
  - `python3 workloads/benchmarks/run_portfolio.py --ctuning-limit 5 --polybench`
- PolyBench subset (freestanding port):
  - `python3 workloads/benchmarks/run_polybench.py --dynamic-hist --kernels gemm,jacobi-2d`
