# LinxISA Navigation Guide (v0.5)

This is the canonical navigation contract for contributors and agents.

## Top-level map

- `README.md` — workspace overview
- `AGENTS.md` — agent-facing routing and guardrails
- `avs/` — architectural verification suite
- `compiler/` — upstream LLVM submodule (`compiler/llvm`)
- `emulator/` — upstream QEMU submodule (`emulator/qemu`)
- `kernel/` — upstream Linux submodule (`kernel/linux`)
- `rtl/` — LinxCore submodule (`rtl/LinxCore`) + rtl notes
- `tools/` — generators, regression, pyCircuit submodule
- `workloads/` — benchmark runners + generated workload artifacts
- `isa/` — ISA source of truth and generated catalogs
- `docs/` — architecture, bring-up, migration, project references
- `lib/` — glibc/musl fork submodules

## Canonical test locations

- Runtime AVS suites: `avs/qemu/`
- Compile AVS suites: `avs/compiler/linx-llvm/tests/`
- AVS matrix/docs: `avs/`

## Canonical toolchain support locations

- Freestanding libc support used by AVS/tests: `avs/runtime/freestanding/`
- Linux libc source forks: `lib/glibc/`, `lib/musl/`
- PTO headers (vendored snapshot + Linx backend): `lib/pto/include/pto/`
- LLVM opcode sync helper: `tools/isa/sync_generated_opcodes.sh`

## Benchmark locations

- CoreMark upstream: `workloads/coremark/upstream/`
- Dhrystone upstream: `workloads/dhrystone/upstream/`
- PolyBench source cache: `workloads/third_party/PolyBenchC/`
- ctuning runner: `workloads/ctuning/`

## Removed / forbidden paths

Do not add or revive these paths:

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

CI guard: `tools/ci/check_repo_layout.sh`

## Submodule policy

When implementation repos change:

1. Merge in the upstream ecosystem repo first.
2. Update submodule SHA in this workspace.
3. Keep `.gitmodules` URLs aligned to LinxISA org forks/repos.
4. Validate with:

```bash
git submodule sync --recursive
git submodule update --init --recursive
bash tools/ci/check_repo_layout.sh
```
