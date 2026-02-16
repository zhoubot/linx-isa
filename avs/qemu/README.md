# LinxISA QEMU test suite

This folder contains small, freestanding C tests compiled for LinxISA and run on the Linx QEMU `virt` machine.

Key constraints:

- The QEMU Linx `virt` machine loads an ELF **relocatable** (`ET_REL`) object via `-kernel` (not a fully linked `ET_EXEC`).
- No libc, and avoid inline assembly (the Linx textual asm parser is not part of the bring-up yet).
- UART output is via MMIO at `0x10000000`.
- Program termination is via MMIO at `0x10000004` (exit code written becomes QEMU’s process exit code).

## Requirements

- LinxISA Clang: `~/llvm-project/build-linxisa-clang/bin/clang`
- LinxISA LLD: `~/llvm-project/build-linxisa-clang/bin/ld.lld`
- QEMU: `~/qemu/build-tci/qemu-system-linx64` (recommended on macOS) or `~/qemu/build/qemu-system-linx64`

Override paths with environment variables:

```bash
export CLANG=/path/to/clang
export LLD=/path/to/ld.lld
export QEMU=/path/to/qemu-system-linx64
```

## Run

```bash
cd ~/linxisa/avs/qemu
./run_tests.sh
```

`run_tests.sh` is just a thin wrapper over `python3 run_tests.py`.

Run a specific suite:

```bash
./run_tests.sh --suite arithmetic
./run_tests.sh --suite branch
./run_tests.sh --suite move
./run_tests.sh --suite jumptable
./run_tests.sh --suite varargs
```

Run everything (includes float + atomic):

```bash
./run_tests.sh --all --timeout 20
```

List suites:

```bash
./run_tests.sh --list-suites
```

Compile-only:

```bash
./run_tests.sh --compile-only
```

Verbose build/run commands:

```bash
./run_tests.sh -v --suite arithmetic
```

Increase timeout:

```bash
./run_tests.sh --timeout 10
```

Rebuild from scratch (clear all intermediates):

```bash
rm -rf out
./run_tests.sh --all --compile-only
```

Custom output directory:

```bash
./run_tests.sh --out-dir /tmp/linx-qemu-tests --all --compile-only
```

## Outputs

- `out/linx-qemu-tests.o`: single `ET_REL` object built with `ld.lld -r` and passed to QEMU as `-kernel`.

## Linux musl smoke

For Linux initramfs + musl runtime smoke (malloc/free/printf), use:

```bash
python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py --mode phase-b
```

`run_musl_smoke.py` links the sample at a high userspace image base by default
(`--image-base 0x40000000`) to avoid low-VA overlap with the current kernel mapping.

Artifacts are written under:

- `/Users/zhoubot/linx-isa/avs/qemu/out/musl-smoke/`

## Call/Ret contract gate

Run the positive call/ret runtime matrix:

```bash
python3 /Users/zhoubot/linx-isa/avs/qemu/run_tests.py \
  --suite callret \
  --require-test-id 0x140b \
  --require-test-id 0x140c
```

Run strict call/ret contract checks:
negative cases must trap with deterministic causes, and positive cases must
run without block-format faults.

```bash
python3 /Users/zhoubot/linx-isa/avs/qemu/run_callret_contract.py
```

Run Linux+musl call/ret sample with strict Linux cross-stack audit:

```bash
python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py \
  --sample callret \
  --link both \
  --callret-crossstack strict
```

Enable additional whole-vmlinux contract audit (optional, slower):

```bash
LINX_AUDIT_VMLINUX=1 python3 /Users/zhoubot/linx-isa/avs/qemu/run_musl_smoke.py \
  --sample callret \
  --link both \
  --callret-crossstack strict
```

## Memory map (Linx virt machine)

| Address | Purpose |
|--------:|---------|
| `0x00008000` | failure record (`test_result_t`) |
| `0x00010000` | kernel load base (`.text/.data/...`) |
| `0x10000000` | UART data register |
| `0x10000004` | exit register (write → shutdown) |
