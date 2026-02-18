# LinxISA Path Migration Map v0.4.0

This map documents the AVS-centric and submodule-first repository tidy.

## Path Changes

| Previous Canonical Path | New Canonical Path | Notes |
|---|---|---|
| `compiler/linx-llvm/` | `avs/compiler/linx-llvm/` | compile-only AVS assets moved under AVS |
| `tests/qemu/` | `avs/qemu/` | runtime AVS suites consolidated |
| `tests/scratch/` | `avs/scratch/` | scratch tests consolidated |
| `models/pyCircuit` | `tools/pyCircuit` | submodule moved to tools domain |
| `docs/validation/avs/` | `avs/` | AVS docs now top-level canonical folder |
| `toolchain/libc/` | `tools/libc/freestanding/` | freestanding support moved under tools |
| `toolchain/pto/include/pto/` | `lib/pto/include/pto/` | PTO headers moved to vendored snapshot + Linx backend |
| `toolchain/llvm/sync_generated_opcodes.sh` | `tools/isa/sync_generated_opcodes.sh` | LLVM sync helper moved |
| `examples/assembly/v0.3/` | `docs/reference/examples/v0.3/` | sample pack consolidated into docs |
| `tools/ctuning/` | `workloads/benchmarks/ctuning/` | benchmark runner moved under workloads |

## Submodule Changes

| Path | URL |
|---|---|
| `compiler/llvm` | `git@github.com:LinxISA/llvm-project.git` |
| `emulator/qemu` | `git@github.com:LinxISA/qemu.git` |
| `kernel/linux` | `git@github.com:LinxISA/linux.git` |
| `rtl/LinxCore` | `git@github.com:LinxISA/LinxCore.git` |
| `tools/pyCircuit` | `git@github.com:LinxISA/pyCircuit.git` |
| `lib/glibc` | `git@github.com:LinxISA/glibc.git` |
| `lib/musl` | `git@github.com:LinxISA/musl.git` |

## Removed Paths

- `emulator/linx-qemu`
- `examples/`
- `models/`
- `toolchain/`
- `tests/`
- `docs/validation/avs/`
- `tools/ctuning/`
