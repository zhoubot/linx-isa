# LinxISA v0.3.0 Path Migration Map (Historical)

This document records the public repository refactor introduced in `v0.3.0`.

For the current layout after `v0.3.1` path updates, see `docs/migration/path-map-v0.3.1.md`.

## Path Map at v0.3.0

| Old Path | New Canonical Path (v0.3.0) | Action | v0.3.0 Shim |
|---|---|---|---|
| `isa/` | `spec/isa/` | moved | `isa -> spec/isa` symlink |
| `compiler/` | `impl/compiler/` | moved | `compiler -> impl/compiler` symlink |
| `emulator/` | `impl/emulator/` | moved | `emulator -> impl/emulator` symlink |
| `rtl/` | `impl/rtl/` | moved | `rtl -> impl/rtl` symlink |
| `models/` | `impl/models/` | moved | `models -> impl/models` symlink |
| `toolchain/` | `impl/toolchain/` | moved | `toolchain -> impl/toolchain` symlink |
| `avs/` | `docs/validation/avs/` | moved | `avs -> docs/validation/avs` symlink |
| `docs/reference/examples/` | `examples/assembly/v0.3/legacy-reference/` | moved | pointer README at old path |
| `tests/scratch/` | `examples/assembly/v0.3/scratch-legacy/` | curated migration | `tests/scratch/README.md` pointer only |

## Legacy Removal Summary

Removed from public tree in `v0.3.0`:

- `spec/isa/golden/v0.1/**`
- `spec/isa/golden/v0.2/**`
- `spec/isa/spec/current/linxisa-v0.1.json`
- `spec/isa/spec/current/linxisa-v0.2.json`
- `linx-bingup.md` (replaced by `docs/architecture/v0.3-architecture-contract.md`)

## Shim Policy at v0.3.0

- Shim paths were compatibility-only for `v0.3.0`.
- Shim removal was scheduled for `v0.3.1`.
