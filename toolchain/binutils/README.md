# binutils port (planned)

This folder will track work needed to support LinxISA in GNU binutils (assembler, disassembler, linker, etc).

## Source of truth

- ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- Generated C tables (mask/match + field extraction): `spec/isa/generated/codecs/linxisa_opcodes.h`
- Register encoding (`SrcL`, `SrcR`, `RegDst`, ...): `registers.reg5` in `spec/isa/spec/current/linxisa-v0.3.json`

## Suggested bring-up order

1. **opcodes + disassembler** (`objdump -d`)
   - Use `linxisa_inst_forms[]` (`mask/match`, `length_bits`) to decode.
   - Print ABI register names (`sp`, `a0`, `s0`, `t#1`, `->t`/`->u`).
2. **gas assembler** (`as`)
   - Implement parsing for the draft assembly syntax (arrow-dest, memory forms like `[base, off]`).
3. **bfd/ld integration**
   - Add ELF machine id + relocations matching the ISA’s PC-relative forms.

Everything should be generated/validated from `spec/isa/spec/current/linxisa-v0.3.json` to avoid drift.
