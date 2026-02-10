# Bring-up Progress

Last updated: 2026-02-10

## Phase status matrix

| Phase | Not Started | In Progress | Blocked | Passed | Evidence |
| --- | --- | --- | --- | --- | --- |
| 1. Compiler |  | X |  |  | `compiler/COMPILER_PLAN.md`, `compiler/llvm/tests/` |
| 2. ISA spec integration |  | X |  |  | `isa/golden/v0.2/`, `isa/spec/current/linxisa-v0.2.json`, `tools/isa/` |
| 3. Emulator/QEMU |  | X |  |  | `tests/qemu/`, `docs/bringup/phases/03_emulator_qemu.md` |
| 4. RTL (pyCircuit agile) |  | X |  |  | `docs/bringup/phases/04_rtl.md`, `/Users/zhoubot/pyCircuit` |
| 5. FPGA (ZYBO Z7-20) | X |  |  |  | `docs/bringup/phases/05_fpga_zybo_z7.md` |
| 6. Linux on FPGA (Janus goal) | X |  |  |  | `docs/bringup/phases/06_linux_on_janus.md` |
| 7. Toolchain/libc support |  | X |  |  | `docs/bringup/phases/07_toolchain_glibc.md`, `tools/glibc/` |

## Gate tracker (`A1`..`D3`)

| Gate | Date | Command/script | Result | Blocker |
| --- | --- | --- | --- | --- |
| A1 Linx pyCircuit C++ pass | 2026-02-07 | `bash /Users/zhoubot/pyCircuit/tools/run_linx_cpu_pyc_cpp.sh` | In Progress | Not yet tracked in this repo with trace-contract output |
| A2 Linx Verilog pass | 2026-02-07 | `python3 /Users/zhoubot/pyCircuit/tools/pyc_flow.py verilog-sim linx_cpu_pyc --tool verilator` | In Progress | Need reproducible run log pinned to gate records |
| A3 Linx trace-diff pass | 2026-02-07 | `QEMU vs Linx-model trace comparison` | Blocked | Unified trace comparator flow not yet wired to `docs/bringup/contracts/trace_schema.md` |
| B1 Janus C++ pass | 2026-02-07 | `bash /Users/zhoubot/pyCircuit/janus/tools/run_janus_bcc_ooo_pyc_cpp.sh` | In Progress | Run status exists in pyCircuit; not yet mirrored in this gate table with command output |
| B2 Janus Verilog pass | 2026-02-07 | `bash /Users/zhoubot/pyCircuit/janus/tools/run_janus_bcc_ooo_pyc_verilator.sh` | In Progress | Same as B1; need gate-local log capture and pass criteria record |
| B3 Janus trace-diff pass | 2026-02-07 | `QEMU vs Janus trace comparison` | Blocked | Subset mapping and comparator output not yet captured in `linxisa` |
| C1 Linx FPGA smoke pass | 2026-02-07 | `ZYBO Z7-20 smoke suite` | Not Started | PS DDR + AXI + PL wrapper and board project flow not landed |
| C2 Janus FPGA smoke pass | 2026-02-07 | `ZYBO Z7-20 smoke suite` | Not Started | Depends on C1 plus Janus PL integration |
| D1 Linx NOMMU Linux shell pass | 2026-02-07 | `Boot kernel + BusyBox on Linx FPGA` | Not Started | Depends on C1 and Linux image pipeline |
| D2 Janus NOMMU Linux shell pass | 2026-02-07 | `Boot kernel + BusyBox on Janus FPGA` | Not Started | Depends on C2 and D1 path transfer |
| D3 Janus MMU Linux shell pass (final) | 2026-02-07 | `Boot full MMU Linux + BusyBox on Janus FPGA` | Not Started | Depends on MMU/TLB/page-walk completion and D2 |

## Latest validation snapshot in `linxisa`

### Compiler + QEMU regression (2026-02-10)

- End-to-end regression: `bash tools/regression/run.sh` (PASS)
- Strict system gate: `./tests/qemu/run_tests.sh --suite system --require-test-id 0x110E` (PASS)
- Compiler tests: 31 compile tests for `linx64` and `linx32` (PASS)
- ISA mnemonic coverage (from `compiler/llvm/tests/analyze_coverage.py`):
  - Spec unique mnemonics: 702
  - Covered spec mnemonics: 702
  - Missing spec mnemonics: 0
  - Note: `BSTART`/`C.BSTART` and `*.STD` are treated as equivalent spellings for coverage.
- QEMU runtime tests: `./tests/qemu/run_tests.sh --all` (PASS)
- Optional ctuning smoke (`CTUNING_LIMIT=5`): PASS (5/5 codelets)

### Benchmarks (2026-02-07)

- Report: `workloads/generated/report.md`
- CoreMark: static 5594, dynamic 827648
- Dhrystone: static 3201, dynamic 830152

### Linux/glibc bring-up (2026-02-10)

- `bash tools/glibc/bringup_linx64.sh`:
  - Linux UAPI headers install: ok
  - glibc configure: ok
  - `csu/subdir_lib`: FAIL (`ld64` rejects GNU linker options)
- Linux busybox userspace boot on QEMU (external `/Users/zhoubot/linux` tree):
  - `python3 tools/linxisa/initramfs/virtio_disk_smoke.py` (PASS)
  - `python3 tools/linxisa/initramfs/full_boot.py` (PASS; reaches userspace checks and `poweroff`)

## Active blockers snapshot

- Shared-library/ET_DYN flow is still gated on complete relocation + PLT/GOT model.
- 128-bit/lockless edge-case lowering requires additional validation.
- Privileged architecture details (exceptions/MMU/system behavior) are still being finalized.

## Update protocol

When progress changes:

1. Update phase status matrix and gate rows in this file.
2. Update `compiler/COMPILER_PLAN.md` for compiler-specific status changes.
3. Record command, result, and blocker explicitly for any gate change.
