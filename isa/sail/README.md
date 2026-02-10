# LinxISA Sail Model (Skeleton)

This directory is the starting point for a **Sail** formal/executable ISA model for LinxISA.

Scope policy:

- The Sail model is a reference direction for semantics.
- Missing semantics MUST be explicit (no guessed behavior). Use `unimplemented("MNEMONIC")`-style traps.
- Coverage is tracked as data in `isa/sail/coverage.json`.

## Coverage report

`isa/sail/coverage.json` is generated from:

- the compiled ISA catalog: `isa/spec/current/linxisa-v0.2.json`
- the list of implemented instruction mnemonics: `isa/sail/implemented_mnemonics.txt`

Regenerate:

```bash
python3 tools/isa/sail_coverage.py --spec isa/spec/current/linxisa-v0.2.json --implemented isa/sail/implemented_mnemonics.txt --out isa/sail/coverage.json
```

## Layout

- `isa/sail/model/linxisa.sail_project`: Sail project entry (placeholder)
- `isa/sail/model/lib/`: shared helpers (placeholder)
- `isa/sail/model/state/`: architectural state definitions (placeholder)
- `isa/sail/model/decode/`: decode model stubs (placeholder)
- `isa/sail/model/execute/`: per-unit execute semantics (placeholder)
