# Workloads

Unified home for runnable workload content:

- `benchmarks/` - benchmark suites and runner scripts.
- `examples/` - standalone example programs.
- `generated/` - generated artifacts from workload runs (objdump, binaries, logs, reports).

Run benchmark workloads:

```bash
python3 workloads/benchmarks/run_benchmarks.py
```

Run the PTO AI kernel bring-up benchmark flow (auto-mode GEMM + flash-attention):

```bash
python3 workloads/benchmarks/run_pto_ai_kernels.py
```

Run PTO CPU sim + Linx QEMU checksum matching for GEMM/flash:

```bash
python3 workloads/benchmarks/compare_pto_cpu_qemu.py
```

Run TSVC on Linx QEMU:

```bash
python3 workloads/benchmarks/run_tsvc.py
```

Primary codegen-quality artifacts:

- `workloads/generated/objdump/`
- `workloads/generated/report.md`
- `workloads/generated/objdump/pto_ai/`
- `workloads/generated/pto_ai_report.md`
- `workloads/generated/pto_qemu_value_match.md`
- `workloads/generated/tsvc_report.md`
