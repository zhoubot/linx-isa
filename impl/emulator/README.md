# Emulator (QEMU)

LinxISA's reference emulator is the Linx target in QEMU at `~/qemu`.

The emulator MUST decode/execute instructions according to:

- ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- Codec tables (for decode/disasm scaffolding): `spec/isa/generated/codecs/`
- ISA manual: `docs/architecture/isa-manual/`

Bring-up phase doc: `docs/bringup/phases/03_emulator_qemu.md`

## Run tests

```bash
./tests/qemu/run_tests.sh --all
```

Override QEMU path:

```bash
QEMU=~/qemu/build-tci/qemu-system-linx64 ./tests/qemu/run_tests.sh --all
```

## Keep QEMU aligned with LinxISA

This repo ships small QEMU patch sets under `impl/emulator/qemu/patches/`.

Apply:

```bash
QEMU_DIR=~/qemu bash impl/emulator/qemu/apply_patches.sh
```
