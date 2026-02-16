# TSVC Workflow

This folder provides a reproducible TSVC workflow for Linx SIMT auto-vectorization
assembly inspection.

## Source policy

- Preferred source location: `workloads/tsvc/upstream/TSVC_2/src`
- Pinned upstream commit: `badf9adb2974867ac0937718d85a44dec6dec95a`
- Fetch helper: `workloads/tsvc/fetch_tsvc.sh`

## Generate auto-mode objdumps + QEMU validation

```bash
python3 workloads/tsvc/run_tsvc.py \
  --clang /Users/zhoubot/llvm-project/build-linxisa-clang/bin/clang \
  --vector-mode auto
```

Artifacts are written under `workloads/generated/`:

- `objdump/tsvc/tsvc.auto.objdump.txt`
- `objdump/tsvc/kernels/auto/*.objdump.txt`
- `qemu/tsvc/tsvc.auto.stdout.txt`
- `qemu/tsvc/tsvc.auto.stderr.txt`
- `reports/tsvc/vectorization_coverage.auto.{md,json}`
- `reports/tsvc/vectorization_remarks.auto.json`
- `reports/tsvc/vectorization_gap_plan.auto.json`
