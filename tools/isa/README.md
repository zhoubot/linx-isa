# LinxISA ISA Tooling (v0.3)

These tools operate on the canonical LinxISA v0.3 catalog.

- Golden source root: `spec/isa/golden/v0.3/`
- Compiled catalog: `spec/isa/spec/current/linxisa-v0.3.json`
- Generated codecs: `spec/isa/generated/codecs/`

## Core Commands

Build catalog:

```bash
python3 tools/isa/build_golden.py --profile v0.3 --pretty
```

Validate catalog:

```bash
python3 tools/isa/validate_spec.py --profile v0.3
```

Generate decode tables:

```bash
python3 tools/isa/gen_qemu_codec.py --profile v0.3 --out-dir spec/isa/generated/codecs
python3 tools/isa/gen_c_codec.py --profile v0.3 --out-dir spec/isa/generated/codecs
```

Generate manual fragments:

```bash
python3 tools/isa/gen_manual_adoc.py --profile v0.3 --out-dir docs/architecture/isa-manual/src/generated
python3 tools/isa/gen_ssr_adoc.py --profile v0.3 --out-dir docs/architecture/isa-manual/src/generated
```

Run v0.3 legacy/drift guard:

```bash
python3 tools/isa/check_no_legacy_v03.py --root .
```
