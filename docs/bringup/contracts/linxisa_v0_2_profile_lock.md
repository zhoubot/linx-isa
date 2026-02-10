# LinxISA v0.2 Profile Lock (Bring-up Contract)

Last updated: 2026-02-10

## Purpose

This document is the **normative lock** for LinxISA v0.2 bring-up behavior across:

- `/Users/zhoubot/linxisa`
- `/Users/zhoubot/qemu`
- `/Users/zhoubot/linux`
- `/Users/zhoubot/llvm-project`

It resolves ambiguous or conflicting wording in draft notes and defines one stable contract for implementation and
regression gates.

## Normative sources

1. `isa/spec/current/linxisa-v0.2.json`
2. `docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc`
3. `docs/architecture/isa-manual/src/chapters/06_templates.adoc`
4. `docs/architecture/isa-manual/src/generated/system_registers_ssr.adoc`
5. `docs/architecture/isa-manual/src/generated/trapno_encoding.adoc`

Draft notes (for rationale only, not normative):

- `/Users/zhoubot/llvm-project/llvm/test/CodeGen/LinxISA/Linx Update.txt`

## Locked architecture decisions

### 1) Exception taxonomy and return-PC rules

- `Fault`: synchronous, restart at faulting PC.
- `Trap`: synchronous, resume at following PC.
- `Interrupt`: asynchronous, resume at following PC.
- `ECSTATE.BI` (not `TRAPNO`) defines header/body trap context:
  - `BI=0`: block header
  - `BI=1`: block body

### 2) TRAPNO v0.2 encoding

```
63        62        47        24        5      0
+----------+----------+----------------+--------+
|    E     |   ARGV   |     CAUSE      | TRAPNUM|
+----------+----------+----------------+--------+
```

- `E=1`: asynchronous interrupt
- `E=0`: synchronous fault/trap
- `ARGV=1`: `TRAPARG0` valid
- `CAUSE[47:24]`: sub-cause
- `TRAPNUM[5:0]`: major class

### 3) Mandatory TRAPNUM values (bring-up)

- `0`: EXEC_STATE_CHECK
- `4`: ILLEGAL_INST
- `5`: BLOCK_TRAP
- `6`: SCALL (ACRC)
- `32`: INST_PC_FAULT
- `33`: INST_PAGE_FAULT
- `34`: DATA_ALIGN_FAULT
- `35`: DATA_PAGE_FAULT
- `44`: INTERRUPT class
- `49`: HW_BREAKPOINT
- `50`: SW_BREAKPOINT
- `51`: HW_WATCHPOINT

### 4) EBARG and BSTATE

- `BSTATE = BARG + LPR`.
- `EBARG` is the sole authoritative trap snapshot interface in v0.2.
- Legacy trap-save names (`EBPC`, `ETPC`, `EBPCN`) are not part of v0.2.
- `ECSTATE.BI=1` resume uses `EBARG_TPC`.
- `BPC` restore uses `EBARG_BPC_CUR`.

### 5) ACRC and ACRE

- `ACRC` triggers immediate synchronous service trap.
- Bring-up rule: `ACRC` must be followed by explicit `BSTOP`/`C.BSTOP` in the same block.
- `ACRC` trap save uses `BI=1`, `EBARG_TPC = next PC` (bring-up: explicit `BSTOP`).
- `ACRE` is the only architectural restore entry:
  - `RRA=0`: reset BARG/LPR defaults
  - `RRA=1`: restore from EBARG + profile-defined extended context
  - other values: fault (`EXEC_STATE_CHECK`)

### 6) Floating-point exceptions

- No FP trap delivery in v0.2 bring-up.
- FP exceptions only update sticky `CSTATE.FFLAGS`.

### 7) Debug SSRs and linking subset

- Debug SSR bank:
  - `DBGID`: `0xnf80`
  - `DBCR/DBVR`: `0xnf90 + 2*n`, `0xnf91 + 2*n`
  - `DCCR/DCVR`: `0xnfA0 + 2*n`, `0xnfA1 + 2*n`
  - `DWCR/DWVR`: `0xnfB0 + 2*n`, `0xnfB1 + 2*n`
- Bring-up counts:
  - `CPs=1`, `BPs=4`, `WPs=4`
- Matching subset:
  - Address match mandatory
  - Linking behavior enabled per control bits
  - `DWCR.LS`: load/store/both semantics required

### 8) ESAVE/ERCOV

- `ESAVE` and `ERCOV` are standalone restartable template blocks.
- They use `[BasePtr, LenBytes, Kind]`.
- Trap/interrupt inside template micro-ops must remain precise and resumable.

## Conflict resolution from draft notes

When draft note fragments disagree, this profile lock resolves by:

1. `linxisa-v0.2.json` + generated manual artifacts first.
2. Exception model section with refined `Fault/Trap/Interrupt` semantics.
3. Existing bring-up tests and implementation contracts.

This explicitly rejects legacy wording such as:

- `BI[62]` in `TRAPNO`
- `E_SCALL` naming as major-class encoding
- `EBPC/ETPC/EBPCN` v0.1 trap-save model

## Cross-repo implementation requirements

- **LinxISA repo**: generators, manual, v0.2 golden/spec, drift checks.
- **QEMU**: trap ABI, EBARG save/restore, ACRC adjacency, debug traps, ESAVE/ERCOV.
- **Linux**: trap decode by `E/ARGV/CAUSE/TRAPNUM`, EBARG-based resume, SIGTRAP delivery.
- **LLVM**: MC/asm/disasm for ESAVE/ERCOV, SSR symbol tables, ACRC+BSTOP emission constraints.

## Regression and drift gates

Minimum required gates:

1. `tools/isa/validate_spec.py` on `linxisa-v0.2.json`
2. `tools/isa/check_no_legacy_v02.py` on v0.2-current artifacts
3. Generated-doc parity checks (`gen_manual_adoc.py`, `gen_ssr_adoc.py`)
4. QEMU runtime tests (`tests/qemu/run_tests.sh --all`)
5. Cross-repo legacy scan (Linx paths in Linux/QEMU/LLVM)

