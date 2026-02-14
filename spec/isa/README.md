# LinxISA Specification (v0.3)

`spec/isa/` is the canonical specification root for the public LinxISA repository.

## Canonical Artifacts

- Golden sources: `spec/isa/golden/v0.3/`
- Compiled catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- Generated codec tables: `spec/isa/generated/codecs/`
- Sail model + coverage assets: `spec/isa/sail/`

## Build + Validate

```bash
python3 tools/isa/build_golden.py --profile v0.3 --pretty
python3 tools/isa/validate_spec.py --profile v0.3
```

## Downstream Consumption

Compiler, emulator, and RTL integration MUST consume the compiled v0.3 catalog to avoid decode/semantic drift.

See also:

- `spec/isa/integration/README.md`
- `spec/isa/generated/codecs/README.md`
