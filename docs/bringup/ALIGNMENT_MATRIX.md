# Linx Stack Alignment Matrix (strict v0.3 current)

Last updated: 2026-02-12

Legend:

- ✅ implemented + validated
- 🟡 implemented but partial coverage
- ❌ missing

## Current source-of-truth

- Current ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- Legacy ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json` (non-default)
- Machine-checkable contract: `docs/bringup/check26_contract.yaml`
- Contract gate: `tools/bringup/check26_contract.py`

## Cross-stack alignment summary

| Area | linxisa spec/docs | LLVM | QEMU | Linux | pyCircuit/Janus | Gate/evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 26-check contract freeze | ✅ check26 docs + gate landed | ✅ mapped in MC/CodeGen tests | ✅ mapped in system/tile tests | ✅ validated by boot flows | ✅ validated by cpp + trace diff | `python3 tools/bringup/check26_contract.py --root .` |
| Canonical v0.3 asm policy (`V.*`, typed `BSTART.*`) | ✅ docs/tooling defaulted to v0.3 | ✅ MC/CodeGen updated | ✅ runtime accepts canonical typed forms | 🟡 ABI unchanged (v0.2 baseline trap ABI) | 🟡 model traces aligned; parser policy external | `python3 tools/isa/check_no_legacy_v03.py --root . --extra-root ...` |
| TSVC C-source auto-vectorization (`MSEQ` default, safe `MPAR`) | 🟡 TSVC harness + coverage scripts landed | 🟡 SIMT autovec pass skeleton + remarks flags landed | 🟡 executable vector subset for TSVC kernels | 🟡 unchanged in this slice | 🟡 n/a (user-space benchmark flow) | `python3 workloads/benchmarks/run_tsvc.py --vector-mode all --coverage-fail-under 151`; `python3 workloads/benchmarks/compare_tsvc_modes.py ...` |
| Block/descriptor contracts (`B.ARG/B.IOR/B.IOT/C.B.DIMI`) | ✅ manual + generated refs | ✅ descriptor emission/tests | ✅ descriptor execution and tile tests | ✅ userspace boot not regressed | ✅ model runs with matching traces | `bash tools/regression/run.sh`; `python3 workloads/benchmarks/run_pto_ai_kernels.py` |
| ACR/IRQ/exception correctness | ✅ privileged chapter + generated trap table | ✅ MC symbols + encodings | ✅ strict system tests | ✅ smoke/full/virtio boots pass | ✅ qemu-vs-pyc commit diff pass | `tests/qemu/check_system_strict.sh`; linux initramfs scripts |
| PTO auto-mode AI kernels (GEMM + Flash) | ✅ workload docs + generated reports | ✅ compiler emits runnable kernels | ✅ qemu tile suite + checksum outputs | 🟡 kernel-side selftest deferred | 🟡 model-side perf not yet tiered | `python3 workloads/benchmarks/run_pto_ai_kernels.py`; `python3 workloads/benchmarks/compare_pto_cpu_qemu.py` |

## Regression baseline

- `bash tools/regression/run.sh` ✅
- `bash tools/regression/full_stack.sh` ✅
- `llvm-lit llvm/test/MC/LinxISA llvm/test/CodeGen/LinxISA` ✅
- `python3 ~/linux/tools/linxisa/initramfs/smoke.py` ✅
- `python3 ~/linux/tools/linxisa/initramfs/full_boot.py` ✅
- `python3 ~/linux/tools/linxisa/initramfs/virtio_disk_smoke.py` ✅
- `bash ~/pyCircuit/tools/run_linx_cpu_pyc_cpp.sh` ✅
- `bash ~/pyCircuit/janus/tools/run_janus_bcc_pyc_cpp.sh` ✅
- `bash ~/pyCircuit/janus/tools/run_janus_bcc_ooo_pyc_cpp.sh` ✅
- `QEMU_BIN=~/qemu/build-tci/qemu-system-linx64 bash ~/pyCircuit/tools/run_linx_qemu_vs_pyc.sh` ✅
