# Phase 2: ISA Spec Integration

Source of truth: `spec/isa/golden/v0.3/**` (compiled to `spec/isa/spec/current/linxisa-v0.3.json`)

Supporting context:
- `isa/README.md`
- `spec/isa/generated/codecs/` (generated decode/encode artifacts)

## Rule

Compiler, emulator, and RTL behavior must be derived from, or checked against, the same catalog.

## Regeneration

```bash
python3 tools/isa/build_golden.py --in spec/isa/golden/v0.3 --out spec/isa/spec/current/linxisa-v0.3.json --pretty
python3 tools/isa/validate_spec.py --spec spec/isa/spec/current/linxisa-v0.3.json
```
