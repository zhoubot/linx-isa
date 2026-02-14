# LinxISA Repository Flow (v0.3+)

The repository is organized around a spec-first flow where implementation tracks consume the same v0.3 ISA catalog.

## Workspace Bootstrap

Before working on bring-up, initialize dependency submodules:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

Pinned dependency repos are domain-scoped:

- `compiler/llvm`
- `emulator/qemu`
- `kernel/linux`
- `models/pyCircuit`

## Flow

1. ISA definition in `spec/isa/golden/v0.3/`
2. Compiled catalog in `spec/isa/spec/current/linxisa-v0.3.json`
3. Generated decode assets in `spec/isa/generated/codecs/`
4. In-repo implementation integration under `compiler/`, `emulator/`, `models/`, `rtl/`, `toolchain/`
5. Cross-repo alignment through domain submodules
6. Validation through `tools/regression/run.sh` and `tools/bringup/check26_contract.py`
