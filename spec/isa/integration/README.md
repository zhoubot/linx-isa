# Interop (QEMU / LLVM)

This folder documents how to consume `spec/isa/spec/current/linxisa-v0.3.json` when building decoders/encoders.

## The key data: `instructions[].encoding`

For each instruction *form*:

- `encoding.parts[].mask` / `encoding.parts[].match` provide classic `mask/match` matching:
  - a part matches when `(insn_part & mask) == match`
- `encoding.parts[].fields[].pieces[]` tells you which bits to extract for each named field.

This is intentionally similar to the mask/match tables used by:
- QEMU decodetree-generated decoders (fixedmask/fixedbits)
- LLVM TableGen decoders/disassemblers (fixed bits + named fields)

## Multi-part instructions

Some instructions are composed from multiple parts (e.g. a 64-bit instruction shown as a 32-bit prefix row + a 32-bit
main row in the draft tables).

In the JSON:
- `encoding.parts[]` is stored in **instruction-stream order**.
- Each part has its own `mask/match/pattern/fields`.

When generating a decoder, fetch and match parts in that order.
