# RTL (LinxISA v0.3)

This directory hosts RTL bring-up assets and verification notes for LinxISA.

## Contracts

- ISA source of truth: `spec/isa/spec/current/linxisa-v0.3.json`
- Golden source root: `spec/isa/golden/v0.3/`
- Decode generation inputs: `spec/isa/generated/codecs/`
- Bring-up phase doc: `docs/bringup/phases/04_rtl.md`
- Trace contract: `docs/bringup/contracts/trace_schema.md`

## Alignment Rule

RTL behavior MUST match the architected semantics in the v0.3 catalog and manual, independent of microarchitectural implementation details.
