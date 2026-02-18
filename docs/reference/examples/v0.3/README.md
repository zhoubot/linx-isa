# LinxISA Assembly Sample Pack (v0.3)

Canonical public assembly examples for LinxISA v0.3.

## Layout

- `curated/`: reviewed hand-curated examples.
- `scratch-legacy/`: selected migrated scratch assembly (public allowlist).
- `generated/`: deterministic outputs generated from PTO examples.
- `legacy-reference/`: moved historical reference examples.
- `index.yaml`: manifest with provenance and generation commands.

## Canonical Example

```asm
BSTART.TMA   TLOAD, FP16
B.ARG        ND2ZN.normal, FP16, Null
B.IOTI       [], last ->t<4KB>
B.IOR        [x2,a6],[]
C.B.DIMI     64, ->lb0
C.B.DIMI     64, ->lb1
C.B.DIMI     64, ->lb2
```

Source: `docs/reference/examples/v0.3/curated/linxisa-v0.3-normalized.asm`
