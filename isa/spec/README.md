# Compiled ISA Catalog

This folder contains the **compiled** (machine-readable) LinxISA ISA catalog.

Source of truth is the multi-file golden tree:

- `isa/golden/v0.2/**` (stable current)
- `isa/golden/v0.3/**` (staged next)

The compiled output is checked in at:

- `isa/spec/current/linxisa-v0.2.json` (stable current)
- `isa/spec/v0.3/linxisa-v0.3.json` (staged next)

## Rebuild

```bash
python3 tools/isa/build_golden.py --profile v0.2 --pretty
python3 tools/isa/validate_spec.py --profile v0.2
python3 tools/isa/build_golden.py --profile v0.3 --pretty
python3 tools/isa/validate_spec.py --profile v0.3
```
