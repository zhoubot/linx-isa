# Phase 2: ISA Spec Integration

Source of truth: `isa/golden/v0.2/**` (compiled to `isa/spec/current/linxisa-v0.2.json`)

Supporting context:
- `isa/README.md`
- `isa/generated/codecs/` (generated decode/encode artifacts)

## Rule

Compiler, emulator, and RTL behavior must be derived from, or checked against, the same catalog.

## Regeneration

```bash
python3 tools/isa/build_golden.py --in isa/golden/v0.2 --out isa/spec/current/linxisa-v0.2.json --pretty
python3 tools/isa/validate_spec.py --spec isa/spec/current/linxisa-v0.2.json
```
