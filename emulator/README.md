# Emulator (QEMU)

Emulator work is split into:

- Upstream QEMU source mirror (submodule): `emulator/qemu/`
- LinxISA QEMU integration patches/scripts: `emulator/linx-qemu/`

The emulator MUST decode/execute instructions according to:

- ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- Codec tables: `spec/isa/generated/codecs/`
- ISA manual: `docs/architecture/isa-manual/`

Bring-up phase doc: `docs/bringup/phases/03_emulator_qemu.md`

## Run tests

```bash
./tests/qemu/run_tests.sh --all
```

Override QEMU path:

```bash
QEMU=~/linx-isa/emulator/qemu/build-tci/qemu-system-linx64 ./tests/qemu/run_tests.sh --all
```

## Keep QEMU aligned with LinxISA

This repo ships patch sets under `emulator/linx-qemu/patches/`.

Apply:

```bash
QEMU_DIR=~/linx-isa/emulator/qemu bash emulator/linx-qemu/apply_patches.sh
```
