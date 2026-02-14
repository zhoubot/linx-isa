# LinxISA Sail Model (Skeleton)

This directory is the starting point for a **Sail** formal/executable ISA model for LinxISA.

Scope policy:

- The Sail model is a reference direction for semantics.
- Missing semantics MUST be explicit (no guessed behavior). Use `unimplemented("MNEMONIC")`-style traps.
- Coverage is tracked as data in `spec/isa/sail/coverage.json`.

## Coverage report

`spec/isa/sail/coverage.json` is generated from:

- the compiled ISA catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- the list of implemented instruction mnemonics: `spec/isa/sail/implemented_mnemonics.txt`

Regenerate:

```bash
python3 tools/isa/sail_coverage.py --spec spec/isa/spec/current/linxisa-v0.3.json --implemented spec/isa/sail/implemented_mnemonics.txt --out spec/isa/sail/coverage.json
```

## Layout

- `spec/isa/sail/model/linxisa.sail_project`: Sail project entry (placeholder)
- `spec/isa/sail/model/lib/`: shared helpers (placeholder)
- `spec/isa/sail/model/state/`: architectural state definitions (placeholder)
- `spec/isa/sail/model/decode/`: decode model stubs (placeholder)
- `spec/isa/sail/model/execute/`: per-unit execute semantics (placeholder)
