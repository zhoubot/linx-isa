# LinxISA end-to-end flow (planned)

This repo is organized so that **the ISA spec is the source of truth**, and all implementations derive from it.

## 1) C → compiler

- C code is compiled by a LinxISA backend (likely LLVM/Clang-based).
- The backend emits LinxISA assembly/object code using instruction encodings defined by `isa/spec/current/linxisa-v0.2.json`.

## 2) compiler → ISA (encoding)

- Instruction selection must be consistent with the ISA catalog:
  - which encodings exist
  - operand fields / immediate widths
  - reserved encodings

## 3) ISA → emulator

- The emulator decoder should be generated (or at least validated) from `isa/spec/current/linxisa-v0.2.json`.
- Execution semantics should match the ISA spec exactly.

## 4) ISA → RTL

- The RTL decode should be generated from the same catalog (or checked against it).
- Any micro-architectural choices are allowed, but the architected behavior must match the ISA.
