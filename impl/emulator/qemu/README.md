# QEMU Integration Notes

The LinxISA QEMU implementation lives in `~/qemu` (external to this repo).

This repo provides small patch sets under `impl/emulator/qemu/patches/` to keep the
QEMU Linx target aligned with the canonical ISA spec in `isa/`.

## Apply patches

```bash
QEMU_DIR=~/qemu bash impl/emulator/qemu/apply_patches.sh
```

## Current patches

- `0001-linxisa-stack-alignment.patch`: updates the Linx QEMU target to match
  the v0.1 bring-up profile:
  - Decoupled headers via `B.TEXT` (header→body→return)
  - Restartable template blocks (`FENTRY/FEXIT/FRET*/MCOPY/MSET`)
  - TTBR0/TTBR1 MMU + tile IOMMU (bring-up subset)
  - Directed bring-up tests under `tests/linxisa/` + runner scripts
