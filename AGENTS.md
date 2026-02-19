# LinxISA Agent Navigation Contract (v0.5)

Follow `docs/project/navigation.md` as the canonical map.

## Allowed top-level roots

- `avs`
- `compiler`
- `emulator`
- `kernel`
- `rtl`
- `tools`
- `workloads`
- `isa`
- `docs`
- `lib`

## Canonical destinations

- Runtime tests: `avs/qemu/`
- Compile-only tests: `avs/compiler/linx-llvm/tests/`
- Freestanding libc support: `avs/runtime/freestanding/`
- pyCircuit workspace mirror: `tools/pyCircuit/` (submodule)
- PTO kernel workspace mirror: `workloads/pto_kernels/` (submodule)
- Assembly sample pack: `docs/reference/examples/v0.3/`

## Forbidden / replaced paths

Do not create, restore, or route new work to:

- `compiler/linx-llvm`
- `emulator/linx-qemu`
- `examples/`
- `models/`
- `toolchain/`
- `tests/`
- `docs/validation/avs/`
- `tools/ctuning/`
- `tools/libc/`
- `tools/glibc/`
- `workloads/benchmarks/`
- `workloads/examples/`
- `spec/`

## No random folders rule

Do not introduce new top-level directories. Place new files only in canonical domains above.

## Submodule bump workflow

```bash
git submodule sync --recursive
git submodule update --init --recursive
git submodule update --remote compiler/llvm emulator/qemu kernel/linux rtl/LinxCore tools/pyCircuit lib/glibc lib/musl workloads/pto_kernels
```

Then run:

```bash
bash tools/ci/check_repo_layout.sh
```
