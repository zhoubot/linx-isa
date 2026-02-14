# Benchmarking Method (C/C++) for LinxISA

Goal: run a repeatable portfolio of C/C++ workloads on LinxISA (QEMU first),
collect correctness signals, and track performance regressions while the
compiler/emulator mature.

This file is intentionally methodology-first. Suite-specific porting lives
under `workloads/benchmarks/`.

## What to measure (minimum useful set)

- `static`:
  - `.text` size and symbol map from `llvm-objdump`
  - instruction histogram from disassembly (sanity + codegen drift)
- `dynamic` (QEMU):
  - retired instruction count
  - per-mnemonic histogram (hotspot guidance)
  - exit status and UART log (correctness)

## Control noise (so numbers mean something)

- Fix input sizes and iteration counts; avoid wall-clock timing as a primary signal during bring-up.
- Record revisions for every run:
  - LinxISA spec version (`spec/isa/spec/current/linxisa-v0.3.json`)
  - LLVM revision + build dir
  - QEMU revision + build dir
  - run flags (`-O2`, `-ffreestanding`, etc.)
- Prefer deterministic, CPU-bound kernels first; gate I/O-heavy workloads until syscall/hosted ABI is stable.
- Use warmup where relevant:
  - Embench explicitly targets "hot cache" behavior by running warmup iterations before timing.
  - Google Benchmark supports a configurable warmup phase (`MinWarmUpTime` / `--benchmark_min_warmup_time`).

## Suites in this repo

Microbench (already integrated):

- CoreMark + Dhrystone: `workloads/benchmarks/` via `python3 workloads/benchmarks/run_benchmarks.py`

Third-party suites (downloaded to `workloads/benchmarks/third_party/`):

- Embench IoT (baremetal-friendly): `embench-iot/`
- PolyBench/C (compute kernels): `PolyBenchC/`
- MiBench (mixed kernels): `mibench/`
- LLVM test-suite (large; mostly hosted): `llvm-test-suite/`
- Google Benchmark (C++ microbench harness): `google-benchmark/`

Sources and pinned revisions are tracked in `workloads/benchmarks/third_party/SOURCES.md`.

## Bring-up plan for running third-party suites

1. Start with "freestanding-friendly" subsets:
   - Embench IoT
   - PolyBench/C kernels (compile+run with fixed sizes)
2. Add a minimal hosted layer only when required:
   - syscall ABI + `printf` is already available via `impl/toolchain/libc/`
   - file I/O and time APIs should remain stubbed until Linux/hosted ABI is ready
3. Treat LLVM test-suite as a long-term gate:
   - begin with a small curated subset of `SingleSource/Benchmarks/` that does not require OS services
   - expand once ET_DYN + dynamic linking + libc are reliable

## Suggested reporting format

Write one report file per run under `workloads/generated/` with:

- toolchain + QEMU revision ids
- build flags per suite
- per-workload PASS/FAIL
- static: code size, instruction histogram
- dynamic: retired count, top-20 instructions by frequency

## References (upstream suite docs)

- Google Benchmark: https://github.com/google/benchmark
- Google Benchmark user guide (warmup/repetitions/min time): https://google.github.io/benchmark/user_guide.html
- LLVM test-suite: https://github.com/llvm/llvm-test-suite
- LLVM test-suite guide (cross-compiling, `TEST_SUITE_RUN_UNDER`, result JSON): https://llvm.org/docs/TestSuiteGuide.html
- Embench IoT: https://github.com/embench/embench-iot
- PolyBench/C: https://github.com/cavazos-lab/PolyBenchC
