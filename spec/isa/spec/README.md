# Compiled ISA Catalog

This folder contains the **compiled** (machine-readable) LinxISA ISA catalog.

Source of truth is the multi-file golden tree:

- `spec/isa/golden/v0.3/**` (stable current)
- `spec/isa/golden/v0.3/**` (legacy stable)

The compiled output is checked in at:

- `spec/isa/spec/current/linxisa-v0.3.json` (stable current)
- `spec/isa/spec/current/linxisa-v0.3.json` (legacy stable)
- `spec/isa/spec/v0.3/linxisa-v0.3.json` (staged snapshot path)

## Rebuild

```bash
python3 tools/isa/build_golden.py --profile v0.3 --pretty
python3 tools/isa/validate_spec.py --profile v0.3
python3 tools/isa/build_golden.py --profile v0.2 --pretty
python3 tools/isa/validate_spec.py --profile v0.2
```
