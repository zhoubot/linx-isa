# LinxISA Bring-up Getting Started

This guide is the entry point for contributors joining the public LinxISA bring-up project.

## 1. Prerequisites

Required:

- `git` (with SSH access to GitHub)
- `python3`
- `clang` + `ld.lld` (LinxISA-enabled toolchain build)
- Ruby/Bundler stack only if rebuilding the ISA manual PDF

Recommended:

- `gh` (GitHub CLI) for PR/release workflows

## 2. Clone the Workspace with Submodules

The bring-up flow uses pinned submodules for external implementation repos.

```bash
git clone --recurse-submodules git@github.com:LinxISA/linx-isa.git
cd linx-isa
git submodule update --init --recursive
```

Submodule map:

- `extern/qemu` -> `LinxISA/qemu`
- `extern/linux` -> `LinxISA/linux`
- `extern/pyCircuit` -> `zhoubot/pyCircuit` (temporary until `LinxISA/pyCircuit` transfer is complete)

If you already cloned `linx-isa` before submodules were added:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

## 3. Verify a Baseline Bring-up Environment

Run the public regression gate from repo root:

```bash
bash tools/regression/run.sh
```

If tools are not on default paths, set overrides:

```bash
export CLANG=~/llvm-project/build-linxisa-clang/bin/clang
export LLD=~/llvm-project/build-linxisa-clang/bin/ld.lld
export QEMU=~/linx-isa/extern/qemu/build-tci/qemu-system-linx64
bash tools/regression/run.sh
```

Run the architecture contract gate:

```bash
python3 tools/bringup/check26_contract.py --root .
```

## 4. Daily Contributor Workflow

1. Pick a bring-up area from `docs/bringup/phases/`.
2. Implement changes in the relevant repo (`linx-isa` or one of the `extern/*` submodules).
3. Run local validation (at minimum: changed tests + `check26` gate).
4. Open PRs in upstream repos first when submodule content changes (`qemu`, `linux`, `pyCircuit`).
5. Update submodule pointers in `linx-isa` only after upstream commits are merged.

Update submodule pointers after upstream merge:

```bash
git submodule update --remote extern/qemu extern/linux extern/pyCircuit
git add .gitmodules extern/qemu extern/linux extern/pyCircuit
git commit -m "chore(submodules): bump bring-up dependencies"
```

## 5. pyCircuit URL Migration (When Org Transfer Completes)

When `LinxISA/pyCircuit` is published, switch URL in your local clone:

```bash
git submodule set-url extern/pyCircuit git@github.com:LinxISA/pyCircuit.git
git submodule sync --recursive
git submodule update --init extern/pyCircuit
```

Then commit the `.gitmodules` URL update in `linx-isa`.

## 6. Where to Coordinate

- Bring-up status and completion criteria: `docs/bringup/PROGRESS.md`
- Contract and architecture checkpoints: `docs/bringup/CHECK26_CONTRACT.md`
- Path and compatibility migration notes: `docs/migration/path-map-v0.3.0.md`
