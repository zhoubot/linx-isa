# ISA tools

## `build_golden.py`

Build the compiled machine-readable catalog from the multi-file golden sources:

- Golden sources (current): `isa/golden/v0.2/**`
- Compiled catalog (checked in, current): `isa/spec/current/linxisa-v0.2.json`
- Legacy catalog (kept for reference): `isa/spec/current/linxisa-v0.1.json`

```bash
python3 tools/isa/build_golden.py --in isa/golden/v0.2 --out isa/spec/current/linxisa-v0.2.json --pretty
```

Use `--check` to verify the checked-in compiled catalog is up-to-date:

```bash
python3 tools/isa/build_golden.py --in isa/golden/v0.2 --out isa/spec/current/linxisa-v0.2.json --check
```

## `split_compiled.py` (bootstrap / review)

Split a compiled catalog JSON back into opcode DSL files (for bootstrapping/review only):

```bash
python3 tools/isa/split_compiled.py --spec isa/spec/current/linxisa-v0.2.json --out isa/golden/v0.2
```

## `validate_spec.py`

Sanity-checks that the generated `mask`/`match`/`pattern` are internally consistent:

```bash
python3 tools/isa/validate_spec.py --spec isa/spec/current/linxisa-v0.2.json
```

## `check_no_legacy_v02.py`

Drift gate to ensure v0.2 “current” artifacts do not reintroduce v0.1 trap ABI terminology:

```bash
python3 tools/isa/check_no_legacy_v02.py --root .
```

To enforce the same terminology lock across external bring-up repos (Linux/QEMU/LLVM), add `--extra-root`:

```bash
python3 tools/isa/check_no_legacy_v02.py \
  --root . \
  --extra-root ~/linux \
  --extra-root ~/qemu \
  --extra-root ~/llvm-project
```

## `gen_qemu_codec.py`

Generates QEMU decodetree-style codec tables in `isa/generated/codecs/`:

```bash
python3 tools/isa/gen_qemu_codec.py --spec isa/spec/current/linxisa-v0.2.json --out-dir isa/generated/codecs
```

The output is intended to be consumed by:
- assembler/disassembler
- emulator decoder
- RTL decode generation

The extractor also computes per-instruction `mask`/`match` + field bit ranges under `instructions[].encoding`,
which is directly usable for QEMU-style decode tables and LLVM TableGen generation.

## `gen_c_codec.py`

Generates a C header/source pair containing packed `mask/match` + field extraction metadata:

```bash
python3 tools/isa/gen_c_codec.py --spec isa/spec/current/linxisa-v0.2.json --out-dir isa/generated/codecs
```

This is intended as a convenient input for LLVM MC and binutils ports without requiring a JSON parser.

## `gen_ssr_adoc.py`

Generates ISA-manual AsciiDoc fragments for the v0.2 SSR map (including EBARG + debug regs) and TRAPNO encoding:

```bash
python3 tools/isa/gen_ssr_adoc.py --spec isa/spec/current/linxisa-v0.2.json --out-dir docs/architecture/isa-manual/src/generated
```

## `linxdisasm.py`

Reference decoder for quick sanity-checks against hex words:

```bash
python3 tools/isa/linxdisasm.py --hex 5316 000fcf87 25cc0f95
```

This uses the same `mask/match` + field extraction derived from the JSON spec.
