# Toolchain

This repo's toolchain surface is split into:

- LLVM/Clang backend (external): `~/llvm-project`
- Runtime/libc (in-repo): `impl/toolchain/libc/`
- (Planned) binutils port notes: `impl/toolchain/binutils/`

Canonical ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json`

## LLVM bring-up quickstart

Expected build dir (used by tests in this workspace):

- `~/llvm-project/build-linxisa-clang/bin/clang`

Run compile tests:

```bash
CLANG=~/llvm-project/build-linxisa-clang/bin/clang ./impl/compiler/llvm/tests/run.sh
```

## Sync spec-driven opcode tables into LLVM

The LLVM LinxISA backend consumes generated opcode tables:

- source: `spec/isa/generated/codecs/linxisa_opcodes.c`
- dest: `~/llvm-project/llvm/lib/Target/LinxISA/MCTargetDesc/linxisa_opcodes.c`

Sync:

```bash
bash impl/toolchain/llvm/sync_generated_opcodes.sh
cmake --build ~/llvm-project/build-linxisa-clang --target llvm-objdump clang llc -j 12
```

## libc (freestanding)

- Headers: `impl/toolchain/libc/include/`
- Startup: `impl/toolchain/libc/src/crt0.s`
- Syscall shim: `impl/toolchain/libc/src/syscall.c`
