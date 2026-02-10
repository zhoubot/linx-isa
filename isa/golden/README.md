# LinxISA Golden Specification

This directory contains the **golden** (authoritative) ISA sources for LinxISA.

## What "golden" means here

"Golden ISA" means a single source-of-truth that is:

- **Versioned and reviewable**: split into many small files for clean diffs.
- **Machine-checkable**: can deterministically generate a compiled catalog, decoder tables, and documentation fragments.
- **Semantics-capable**: intended to be paired with a formal semantics model (see `isa/sail/`).

## Source vs compiled outputs

- **Golden sources (stable current)**: `isa/golden/v0.2/**`
- **Compiled catalog (stable current)**: `isa/spec/current/linxisa-v0.2.json`
- **Golden sources (staged next)**: `isa/golden/v0.3/**`
- **Compiled catalog (staged next)**: `isa/spec/v0.3/linxisa-v0.3.json`
- **Generated decoders/codecs** (checked in): `isa/generated/codecs/**`

Build stable v0.2:

```bash
python3 tools/isa/build_golden.py --profile v0.2 --pretty
```

Build staged v0.3:

```bash
python3 tools/isa/build_golden.py --profile v0.3 --pretty
```

## Opcode database

Instruction encodings live in `isa/golden/v0.2/opcodes/*.opc` and `isa/golden/v0.3/opcodes/*.opc` using a simple,
line-based DSL.
These files are the authoritative encoding database.

## References

- Sail ISA semantics language: https://alasdair.github.io/
- Opcode database + generators precedent: https://github.com/riscv/riscv-opcodes
