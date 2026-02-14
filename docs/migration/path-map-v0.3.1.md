# LinxISA Path Migration Map v0.3.1

This map documents the post-v0.3.0 layout update that removes `impl/` and `extern/` from canonical paths.

## Path Changes

| Previous Canonical Path | New Canonical Path | Notes |
|---|---|---|
| `impl/compiler/` | `compiler/` | `compiler/llvm` is upstream submodule; `compiler/linx-llvm` is in-repo integration |
| `impl/emulator/` | `emulator/` | `emulator/qemu` is upstream submodule; `emulator/linx-qemu` is in-repo integration |
| `impl/models/` | `models/` | `models/pyCircuit` is upstream submodule |
| `impl/rtl/` | `rtl/` | in-repo |
| `impl/toolchain/` | `toolchain/` | in-repo |
| `extern/linux` | `kernel/linux` | moved under new kernel domain |
| `extern/qemu` | `emulator/qemu` | moved under emulator domain |
| `extern/pyCircuit` | `models/pyCircuit` | moved under models domain |

## Canonical Rules

- New development MUST use `spec/` plus top-level domain folders (`compiler/`, `kernel/`, `emulator/`, `models/`, `rtl/`, `toolchain/`).
- `compiler/llvm`, `emulator/qemu`, `kernel/linux`, and `models/pyCircuit` are submodule paths.
- In-repo integration assets remain under domain-local non-submodule paths (`compiler/linx-llvm`, `emulator/linx-qemu`).
