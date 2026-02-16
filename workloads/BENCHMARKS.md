# Benchmarks

This directory keeps benchmark sources close to upstream and provides explicit cross-target runners.

Included suites:

- `coremark/upstream/` — CoreMark upstream sources
- `dhrystone/upstream/` — Dhrystone upstream sources
- `third_party/PolyBenchC/` — fetched on demand
- `tsvc/` — TSVC strict auto-vectorization runner + pinned source fetch script
- `ctuning/` — Milepost codelet runner

## Fetch third-party suites

```bash
bash workloads/fetch_third_party.sh
```

## CoreMark + Dhrystone

Build only:

```bash
python3 workloads/run_benchmarks.py \
  --cc /path/to/clang \
  --target <triple>
```

Build + execute through a wrapper (example):

```bash
python3 workloads/run_benchmarks.py \
  --cc /path/to/clang \
  --target <triple> \
  --run-command "qemu-system-linx64 -M virt -nographic -monitor none -kernel {exe}"
```

## PolyBench

```bash
python3 workloads/run_polybench.py \
  --cc /path/to/clang \
  --target <triple> \
  --kernels gemm,jacobi-2d
```

## ctuning Milepost codelets

```bash
python3 workloads/ctuning/run_milepost_codelets.py \
  --ctuning-root ~/ctuning-programs \
  --clang /path/to/clang \
  --lld /path/to/ld.lld \
  --target <triple> \
  --compile-only
```

## Portfolio runner

```bash
python3 workloads/run_portfolio.py --cc /path/to/clang --target <triple>
```

## TSVC strict auto-vectorization objdumps

Fetch pinned TSVC source:

```bash
bash workloads/tsvc/fetch_tsvc.sh
```

Generate auto-mode objdump + QEMU run + strict coverage/gap reports:

```bash
python3 workloads/tsvc/run_tsvc.py --vector-mode auto
```
