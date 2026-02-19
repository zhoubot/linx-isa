# LinxISA Bring-up Getting Started

This guide is the entry point for contributors joining the LinxISA bring-up workspace.

## 1. Prerequisites

Required:

- `git` (with GitHub SSH access)
- `python3`
- `clang` + `ld.lld` for Linx cross builds

Recommended:

- `gh` (GitHub CLI)

## 2. Clone with Submodules

```bash
git clone --recurse-submodules git@github.com:LinxISA/linx-isa.git
cd linx-isa
git submodule sync --recursive
git submodule update --init --recursive
```

Submodule map:

- `compiler/llvm` -> `LinxISA/llvm-project`
- `emulator/qemu` -> `LinxISA/qemu`
- `kernel/linux` -> `LinxISA/linux`
- `rtl/LinxCore` -> `LinxISA/LinxCore`
- `tools/pyCircuit` -> `LinxISA/pyCircuit`
- `lib/glibc` -> `LinxISA/glibc`
- `lib/musl` -> `LinxISA/musl`
- `workloads/pto_kernels` -> `LinxISA/PTO-Kernel`

## 3. Validate Baseline

From repo root:

```bash
bash tools/regression/run.sh
```

Optional overrides:

```bash
export CLANG=~/llvm-project/build-linxisa-clang/bin/clang
export LLD=~/llvm-project/build-linxisa-clang/bin/ld.lld
export QEMU=~/qemu/build-tci/qemu-system-linx64
bash tools/regression/run.sh
```

Run contract gate:

```bash
python3 tools/bringup/check26_contract.py --root .
```

## 4. Daily Workflow

1. Pick a scope under `docs/bringup/phases/`.
2. Implement in the relevant submodule/repo first.
3. Run AVS + regression gates locally.
4. Merge upstream in ecosystem repos.
5. Bump submodule SHAs in `linx-isa`.

Submodule bump command:

```bash
git submodule update --remote compiler/llvm emulator/qemu kernel/linux rtl/LinxCore tools/pyCircuit lib/glibc lib/musl workloads/pto_kernels
git add .gitmodules compiler/llvm emulator/qemu kernel/linux rtl/LinxCore tools/pyCircuit lib/glibc lib/musl workloads/pto_kernels
git commit -m "chore(submodules): bump ecosystem revisions"
```

## 5. Canonical Paths

- AVS runtime tests: `avs/qemu/`
- AVS compile tests: `avs/compiler/linx-llvm/tests/`
- Freestanding libc support used by AVS: `avs/runtime/freestanding/`
- Linux libc source forks: `lib/glibc/`, `lib/musl/`
- PTO kernel headers: `workloads/pto_kernels/include/`
- Assembly sample pack: `docs/reference/examples/v0.3/`

## 6. Coordination References

- Bring-up progress: `docs/bringup/PROGRESS.md`
- Contract checkpoint: `docs/bringup/CHECK26_CONTRACT.md`
- Migration map: `docs/migration/path-map-v0.4.0.md`
- Navigation guide: `docs/project/navigation.md`
