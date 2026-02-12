# Benchmarks (CoreMark + Dhrystone) for LinxISA QEMU

This folder vendors two classic C benchmarks and provides a small build/run
harness for the LinxISA QEMU `virt` machine.

Benchmarks:

- `coremark/` — EEMBC CoreMark (upstream in `coremark/upstream/`)
- `dhrystone/` — Dhrystone 2.1 (Netlib `dhry-c`, upstream in `dhrystone/upstream/`)

Port notes:

- CoreMark uses a minimal `core_portme.*` under `coremark/linx/`.
- CoreMark builds `core_list_join.c` at `-O0` (the Linx LLVM backend currently
  miscompiles the list-reversal path at `-O2`, breaking CRC validation).
- Dhrystone is adapted for a freestanding environment under `dhrystone/linx/`
  (no `scanf`, no OS timing, no `%f` printing).

Build + run everything and write `workloads/generated/report.md`:

```bash
python3 workloads/benchmarks/run_benchmarks.py
```

Compile and run PTO AI kernels (auto-mode GEMM + flash-attention), validate
tile-group usage (T/U/M/N on auto kernels), and emit objdumps + report for
all PTO example kernels (`tload_store`, `mamulb`, `tmatmul_acc`,
`gemm_auto`, `flash_attention_auto`):

```bash
python3 workloads/benchmarks/run_pto_ai_kernels.py
```

Run PTO CPU sim baselines and verify GEMM/flash checksums against Linx QEMU:

```bash
python3 workloads/benchmarks/compare_pto_cpu_qemu.py
```

Fetch third-party suites (including TSVC):

```bash
bash workloads/benchmarks/fetch_third_party.sh
```

Build and run the TSVC suite with a QEMU-sized profile and vector-block
markers enabled (`BSTART.MSEQ` by default):

```bash
python3 workloads/benchmarks/run_tsvc.py
```

Override tool paths:

```bash
export CLANG=~/llvm-project/build-linxisa-clang/bin/clang
export LLD=~/llvm-project/build-linxisa-clang/bin/ld.lld
export QEMU=~/qemu/build/qemu-system-linx64   # or: ~/qemu/build-tci/qemu-system-linx64
python3 workloads/benchmarks/run_benchmarks.py
```

Generated artifacts are written under:

- `workloads/generated/elf/`
- `workloads/generated/bin/`
- `workloads/generated/objdump/` (codegen-quality inspection)
- `workloads/generated/qemu/`
- `workloads/generated/report.md` (static/dynamic instruction counts and histograms)
- `workloads/generated/objdump/pto_ai/` (PTO GEMM/flash auto-mode objdumps)
- `workloads/generated/pto_ai_report.md` (PTO AI kernel validation report)
- `workloads/generated/pto_qemu_value_match.md` (PTO CPU sim vs QEMU checksum match report)
- `workloads/generated/tsvc_report.md` (TSVC kernel coverage + vector marker check)
