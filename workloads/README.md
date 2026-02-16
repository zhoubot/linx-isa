# Workloads

Unified home for runnable workload content:

- benchmark suites and runner scripts under `workloads/` (for example `coremark/`, `dhrystone/`, `ctuning/`, `run_benchmarks.py`).
- TSVC auto-vectorization workflow under `workloads/tsvc/`.
- `generated/` - generated artifacts from workload runs.

## Primary runners

Build CoreMark + Dhrystone (explicit cross target):

```bash
python3 workloads/run_benchmarks.py --cc /path/to/clang --target <triple>
```

Build PolyBench kernels (explicit cross target):

```bash
python3 workloads/run_polybench.py --cc /path/to/clang --target <triple> --kernels gemm,jacobi-2d
```

Run consolidated portfolio:

```bash
python3 workloads/run_portfolio.py --cc /path/to/clang --target <triple>
```

Generate TSVC auto-vectorization objdumps (and run on Linx QEMU):

```bash
python3 workloads/tsvc/run_tsvc.py --vector-mode auto
```
