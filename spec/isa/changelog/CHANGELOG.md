# LinxISA changelog

This changelog tracks notable changes to the LinxISA draft catalog and related spec artifacts in this repository.

## v0.1 (draft) — 2026-02-06

- `FENCE.D` operand spelling: `pred_imm` (was `perd_imm` in the draft tables).
- Manual and generated reference updates:
  - Defined `TPC`/`BPC` terminology for PC-relative formulas and exception capture.
  - Expanded Block ISA documentation for block header descriptors (`B.*` metadata).
  - Improved generated instruction-description pseudocode for key system/control instructions (`ASSERT`, `EBREAK`,
    `FENCE.*`, `ACRE/ACRC`, `B.DIM`).
  - Privileged architecture alignment (from golden sources under `spec/isa/golden/v0.3/`):
    - Defined the v0.1 ACR model (up to 16 rings in a tree rooted at ACR0) and documented `ACR_ENTER` /
      `SERVICE_REQUEST` flows and `ACRC` routing.
    - Updated the curated SSR table used by the manual to match the v0.1 draft system-register IDs and to document
      12-bit vs 24-bit SSR ID encodings (`SSRGET` vs `HL.SSRGET`) (including `CYCLE = 0x0C00`).
    - Renamed the block commit argument state from CARG to BARG in the manual to match the draft naming.
