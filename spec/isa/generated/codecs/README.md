# Codec files (encoding / decoding tables)

This folder contains **generated codec tables** derived from the canonical ISA JSON spec:

- Source of truth: `spec/isa/spec/current/linxisa-v0.3.json`
- Generators:
  - `tools/isa/gen_qemu_codec.py` (QEMU decodetree-style text)
  - `tools/isa/gen_c_codec.py` (C tables for LLVM/binutils integration)

## Files

- `linxisa16.decode`: 16-bit instruction forms (LX-C / compressed)
- `linxisa32.decode`: 32-bit instruction forms (base ISA and extensions)
- `linxisa48.decode`: 48-bit instruction forms (HL.*)
- `linxisa64.decode`: 64-bit instruction forms (e.g. V.* prefix+main forms)
- `linxisa_opcodes.h` / `linxisa_opcodes.c`: packed `mask/match` + field extraction tables (C API)

The `.decode` syntax is QEMU *decodetree-style*:
- `%field` definitions describe how to extract bitfields (including multi-piece fields)
- Each instruction form line includes a fixed-bit pattern plus field refs/assignments.

## Regenerating

```bash
python3 tools/isa/gen_qemu_codec.py --spec spec/isa/spec/current/linxisa-v0.3.json --out-dir spec/isa/generated/codecs
python3 tools/isa/gen_c_codec.py --spec spec/isa/spec/current/linxisa-v0.3.json --out-dir spec/isa/generated/codecs
python3 tools/isa/validate_spec.py --spec spec/isa/spec/current/linxisa-v0.3.json
```

## Notes on 64-bit forms

Some 64-bit forms are represented as two 32-bit parts in the spec.
The codec generator packs parts in **instruction-stream order**:
- the first 32-bit word occupies bits `[31:0]`
- the second 32-bit word occupies bits `[63:32]`
