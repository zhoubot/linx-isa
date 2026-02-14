# LinxISA Strict 26-Check Contract (v0.3 current)

Last updated: 2026-02-12

This document freezes the architectural contract from `docs/architecture/v0.3-architecture-contract.md` into a machine-checkable
ledger used by bring-up gates.

Machine-readable source:

- `./docs/bringup/check26_contract.yaml`
- Gate script: `./tools/bringup/check26_contract.py`

## Numbering policy

- The source text contains two sections labeled "Check 15".
- In the strict contract they are represented as:
  - `Check 15` (generic `BSTART.TEPL` interface)
  - `Check 15`, clause `2` (FP-hint behavior of `BSTART.FP`)
- The contract still contains exactly 26 checks (`1..26`).

## Scope

Each check in the machine ledger contains:

- norm text clauses (`clauses[]`)
- implementation owner references (`owners[]`)
- required verification references (`tests[]`)
- required canonical text/encoding tokens (`patterns[]`)

The gate enforces:

1. exactly 26 checks with contiguous IDs `1..26`,
2. no empty clauses/owners/tests,
3. every required pattern exists in canonical v0.3 artifacts.

## Required canonical artifacts

Pattern scanning is restricted to the strict v0.3 source-of-truth artifacts:

- `./spec/isa/golden/v0.3/opcodes/lx_32.opc`
- `./spec/isa/golden/v0.3/state/memory_model.json`
- `./docs/architecture/isa-manual/src/chapters/02_programming_model.adoc`
- `./docs/architecture/isa-manual/src/chapters/04_block_isa.adoc`
- `./docs/architecture/isa-manual/src/chapters/08_memory_operations.adoc`
- `./docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc`

## Exit criteria

- `python3 ./tools/bringup/check26_contract.py --root .` returns `OK`.

