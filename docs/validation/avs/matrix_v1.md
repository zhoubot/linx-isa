# Linx-AVS v1 Test Matrix

Version: v1 (draft)

This file is a **test matrix**, not an implementation. It defines test IDs,
required architectural behavior, and pass/fail criteria. A later agent should
implement tests under `tests/qemu/` (runtime) and `impl/compiler/llvm/tests/`
(compile-only), and wire difftest once RTL trace output conforms to
`docs/bringup/contracts/trace_schema.md`.

## Profiles

The suite is partitioned into profiles so early bring-up can pass a strict,
useful subset without pretending vector/tile/MMU are done.

- `LNX-S32`: scalar 32-bit, freestanding, little-endian, no MMU
- `LNX-S64`: scalar 64-bit, freestanding, little-endian, no MMU
- `LNX-PRIV`: adds ACR/SSR trap envelope behavior (no full OS/MMU required)
- `LNX-ATOM`: adds atomics + ordering qualifiers
- `LNX-FP`: adds scalar FP + FCSR behavior
- `LNX-VPAR`: enables vector blocks (`BSTART.VPAR`)
- `LNX-VSEQ`: enables vector blocks (`BSTART.VSEQ`)
- `LNX-TILE`: enables tile blocks / TAU (not required for v1 pass)

Unless stated otherwise, tests apply to both `LNX-S32` and `LNX-S64`.

## General Pass/Fail Rules

- A test **PASSes** if it observes the expected architectural state changes and
  does not trigger unexpected traps.
- A test **FAILs** if it:
  - traps unexpectedly,
  - produces wrong architectural results, or
  - violates Block ISA invariants (safety rule, call header adjacency, etc.).
- For tests expecting a trap, PASS requires:
  - correct trap class/cause (as defined by the privileged architecture), and
  - precise PC capture (trap source `BPC`/`TPC` semantics).

## Test ID Format

`AVS-<AREA>-<NNN>` where AREA is one of:

- `DEC` decode/illegal encodings
- `BLK` Block ISA invariants
- `ALU` integer ops
- `MEM` loads/stores/addressing
- `BR` branches/jumps/calls/returns
- `FP` floating-point
- `ATOM` atomics/fences
- `SYS` SSR/ACR/traps
- `ABI` calling convention / varargs (toolchain-facing)
- `TOOL` impl/toolchain/disasm/asm validation (non-architectural but required for bring-up)

## Decode and Illegal Encodings (`DEC`)

### AVS-DEC-001: Reserved/illegal encoding traps deterministically

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Any encoding not matched by the ISA catalog MUST trap as illegal
    instruction (not "execute random behavior").
- Pass criteria:
  - QEMU and RTL (when available) trap on the same byte sequence at the same
    `TPC`, and do not commit partial architectural side effects.

### AVS-DEC-002: `C.BSTOP` (all-zero 16b) decodes consistently

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - `C.BSTOP` is the all-zero 16-bit encoding and MUST be handled intentionally
    (either as a valid block stop or as a defined illegal encoding; the choice
    must match the ISA manual and catalog).
- Pass criteria:
  - Executing an all-zero halfword at a valid `TPC` produces the defined
    behavior.

## Toolchain/Disassembly (`TOOL`)

### AVS-TOOL-001: Disassembler recognizes all spec mnemonics

- Profile: all (bring-up gate)
- Requirement:
  - `llvm-objdump -d` MUST be able to decode and print a mnemonic token for
    every mnemonic enumerated in `spec/isa/spec/current/linxisa-v0.3.json` when fed
    the generated decode vectors (`impl/compiler/llvm/tests/gen_disasm_vectors.py`).
- Pass criteria:
  - `python3 impl/compiler/llvm/tests/analyze_coverage.py --fail-under 100` exits 0.

## Block ISA (`BLK`)

### AVS-BLK-001: Safety rule (targets must land on block start markers)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Any control-flow target that does not point at a block start marker MUST
    raise the defined exception (block-format-related class).
- Pass criteria:
  - Construct a program that attempts to jump into the middle of a block; the
    run traps at commit with correct `BPC/TPC` capture.

### AVS-BLK-002: Block boundary commit rules

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - A block ends at `BSTOP/C.BSTOP` or implicitly at the next `BSTART.*`.
  - `BARG` updates from `SETC.*` are consulted at commit for block-to-block
    control-flow.
- Pass criteria:
  - Directed programs show correct fall-through vs taken transitions with and
    without an explicit `BSTOP`.

### AVS-BLK-003: `SETC.*` validity (must execute inside a block)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - `SETC.*` executed before any block start marker is illegal (or traps as
    block-format error) as defined by the manual.
- Pass criteria:
  - A program that places `SETC.*` at reset `TPC` traps; the same `SETC.*`
    placed after a `BSTART.*` behaves correctly.

### AVS-BLK-004: Call header adjacency (`BSTART CALL` + `SETRET`)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - `SETRET/C.SETRET` used for a call MUST be immediately adjacent to the call
    block start marker; violating adjacency MUST trap or be rejected by the
    toolchain (one behavior must be specified and enforced).
- Pass criteria:
  - A program with an instruction between `BSTART CALL` and `SETRET` produces
    the specified failure mode; the compliant sequence calls and returns
    correctly.

### AVS-BLK-005: Vector block headers use `BSTART.VPAR/VSEQ`

- Profile: `LNX-VPAR`, `LNX-VSEQ`
- Requirement:
  - Vector block types are `BSTART.VPAR` and `BSTART.VSEQ`. `BSTART.VEC` MUST
    not appear in the golden sources or manual.
- Pass criteria:
  - Assembler/disassembler and documentation use `VPAR/VSEQ`; no `VEC` string
    remains in the repo.

## Integer ALU (`ALU`)

### AVS-ALU-001: Add/sub basic correctness

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - `ADD/ADDI/SUB/SUBI` produce correct results including edge cases
    (carry-out ignored unless defined, sign extension rules applied).
- Pass criteria:
  - Directed tests cover: 0, -1, INT_MIN/INT_MAX, mixed sign, shifts of
    immediate materialization sequences.

### AVS-ALU-002: MUL/DIV/REM correctness including signedness

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Signed/unsigned variants match the spec for all corner cases, including
    divide-by-zero behavior (trap vs defined result).
- Pass criteria:
  - Directed tests cover `DIV/DIVU/REM/REMU` and 32-bit `*W` forms.

## Control Flow (`BR`)

### AVS-BR-001: Direct branch forms target scaling (halfword-scaled)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - PC-relative targets in `BSTART` and `SETRET` are halfword-scaled:
    `target = PC + (imm << 1)` as documented.
- Pass criteria:
  - A directed program validates exact target addresses (including negative
    offsets) on QEMU and (later) RTL.

### AVS-BR-002: Indirect transitions use `SETC.TGT`

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - `RET/IND/ICALL` class transitions consult the target selected by
    `SETC.TGT`.
- Pass criteria:
  - A directed test sets different targets and observes correct dispatch.

## Memory (`MEM`)

### AVS-MEM-001: Endianness is little-endian

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Multi-byte loads/stores are little-endian.
- Pass criteria:
  - Store a 64-bit pattern and load bytes/halfwords/words to confirm ordering.

### AVS-MEM-002: Alignment behavior is specified and consistent

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Misaligned behavior (trap vs split) is profile-defined and MUST be
    consistent across QEMU and RTL for the selected profile.
- Pass criteria:
  - A test attempts misaligned loads/stores and matches the defined outcome.

### AVS-MEM-003: Scaled vs unscaled immediate offsets (`.U`)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Scaled families apply implicit shift; `.U` families are byte-unscaled.
- Pass criteria:
  - Directed load/store sequences validate exact addresses touched.

## Atomics and Fences (`ATOM`)

### AVS-ATOM-001: `.aq/.rl` qualifiers enforce acquire/release ordering

- Profile: `LNX-ATOM`
- Requirement:
  - `.aq` prevents later memory ops from being observed before the atomic.
  - `.rl` prevents earlier memory ops from being observed after the atomic.
  - `.aqrl` enforces both.
- Pass criteria:
  - Litmus tests (2-thread) show forbidden outcomes do not occur when the
    qualifier is used as specified.

### AVS-ATOM-002: `FENCE.D pred,succ` orders the specified classes

- Profile: `LNX-ATOM`
- Requirement:
  - `FENCE.D` orders memory ops according to `pred` and `succ` bitfields
    (definition lives in the ISA manual).
- Pass criteria:
  - Litmus tests plus a microbenchmark confirm ordering and no-op cases.

### AVS-ATOM-003: `FENCE.I` synchronizes instruction fetch

- Profile: `LNX-ATOM`
- Requirement:
  - After modifying code memory, `FENCE.I` ensures subsequent instruction fetch
    observes the new contents.
- Pass criteria:
  - A self-modifying-code test behaves as specified (or traps if SMC is
    unsupported; the behavior must be defined).

## Privileged / System (`SYS`)

### AVS-SYS-001: `ACRC` is last in block and traps/routs as specified

- Profile: `LNX-PRIV`
- Requirement:
  - `ACRC` MUST be the last instruction in its block.
  - Valid `request_type` routing matches the manual rules.
- Pass criteria:
  - Violating placement produces the defined trap; valid requests reach the
    specified managing ring trap vector.

### AVS-SYS-002: `ACRE` restores state as specified

- Profile: `LNX-PRIV`
- Requirement:
  - `ACRE` performs `ACR_ENTER` and restores `CSTATE`, `BPC`, and (when
    requested) `BSTATE/BARG` per the manual.
- Pass criteria:
  - A trap handler returns and execution resumes at the expected block start.

## ABI / Toolchain (`ABI`)

### AVS-ABI-001: Calling convention correctness (leaf/non-leaf)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - Function calls preserve callee-saved registers, stack alignment, and return
    values as defined by the Linx ABI.
- Pass criteria:
  - C tests using nested calls validate stack and register preservation.

### AVS-ABI-002: Varargs correctness

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - `va_start/va_arg` works for integers and (if enabled) floating-point.
- Pass criteria:
  - Runtime tests match expected sums/prints and do not corrupt caller state.

## Floating-Point (`FP`)

### AVS-FP-001: Basic FP ops match scalar spec

- Profile: `LNX-FP`
- Requirement:
  - Scalar FP arithmetic (`FADD/FSUB/FMUL/FDIV`) matches IEEE-754 rounding mode
    and exception flag behavior as specified by the Linx manual (FCSR).
- Pass criteria:
  - Directed tests cover NaN propagation, +/-0, infinities, denorm handling (if
    supported), and flag setting/clearing.

### AVS-FP-002: FP <-> int conversions match spec

- Profile: `LNX-FP`
- Requirement:
  - `FCVT/SCVTF/UCVTF` (and variants) produce correct results for edge cases.
- Pass criteria:
  - Directed tests validate exact bit patterns and trap/flag behavior.

## Vector Blocks (`VPAR` / `VSEQ`)

### AVS-VEC-001: Vector blocks require correct header marker

- Profile: `LNX-VPAR`, `LNX-VSEQ`
- Requirement:
  - Vector instructions that require a vector block MUST trap (illegal or
    block-format error) if executed outside a vector block.
- Pass criteria:
  - A directed program executes one vector instruction before any
    `BSTART.VPAR/VSEQ` and traps with the defined cause; the same instruction
    inside the correct block executes and updates state.

### AVS-VEC-002: `BSTART.VPAR` vs `BSTART.VSEQ` legality is enforced

- Profile: `LNX-VPAR`, `LNX-VSEQ`
- Requirement:
  - Any instruction that is only legal in `VPAR` (or only in `VSEQ`) MUST trap
    if executed in the wrong block type.
- Pass criteria:
  - A directed program places a `VPAR-only` op in a `VSEQ` block (and vice
    versa) and observes the defined failure mode.

### AVS-VEC-003: `C.BSTART.VPAR/VSEQ` fall-through markers are honored

- Profile: `LNX-VPAR`, `LNX-VSEQ`
- Requirement:
  - `C.BSTART.VPAR` and `C.BSTART.VSEQ` are valid compressed fall-through
    markers and MUST begin a new block of the corresponding type.
- Pass criteria:
  - A directed program uses the compressed marker and executes subsequent
    vector instructions successfully.

## Memory Model Litmus (`ATOM`)

### AVS-ATOM-010: Message passing requires acquire/release or fence

- Profile: `LNX-ATOM`
- Requirement:
  - Without `.aq/.rl` or `FENCE.D`, the outcome `r1=1, r2=0` in the classic
    message-passing test MAY occur under weak ordering.
  - With release on the publishing store (or atomic) and acquire on the
    consuming load (or atomic), that outcome MUST NOT occur.
- Pass criteria:
  - A litmus harness runs the test for a large number of iterations and
    observes the allowed/forbidden outcome sets as required.

### AVS-ATOM-011: `FENCE.D` orders MMIO and Normal memory when `O` selected

- Profile: `LNX-ATOM`
- Requirement:
  - If `O` is selected in `pred/succ`, then MMIO writes are ordered against
    Normal memory per the manual.
- Pass criteria:
  - A directed test uses a fake MMIO device (or a QEMU model) that records
    ordering, and the observed order matches the fence definition.

## Toolchain Round-Trip (`TOOL`)

### AVS-TOOL-010: `llvm-mc` asm/disasm round-trip for canonical syntax

- Profile: bring-up gate
- Requirement:
  - For a curated set of instruction forms, `llvm-mc` assembles canonical
    syntax and `llvm-objdump` disassembles back to a mnemonic that maps to the
    same spec entry.
- Pass criteria:
  - A `lit`-style or script-based test assembles bytes, disassembles them, and
    the mnemonic set matches expectations.

### AVS-TOOL-011: Spec decode vectors disassemble with 100% mnemonic coverage

- Profile: bring-up gate
- Requirement:
  - The generated spec decode vectors (`99_spec_decode`) cover all spec
    mnemonics as printed by `llvm-objdump`.
- Pass criteria:
  - `python3 impl/compiler/llvm/tests/analyze_coverage.py --fail-under 100` exits 0
    (auto-detecting the newest `out-linx*` directory).

## Emulator Observability (`EMU`)

### AVS-EMU-001: Illegal encodings trap deterministically (no partial state)

- Profile: `LNX-S32`, `LNX-S64`
- Requirement:
  - QEMU MUST reject reserved/illegal encodings deterministically and MUST NOT
    commit partial architectural side effects when trapping.
- Pass criteria:
  - Directed negative tests validate trap PC/cause and that registers/memory
    remain unchanged.

### AVS-EMU-010: Dynamic instruction histogram matches instruction counter

- Profile: bring-up gate
- Requirement:
  - When running with the QEMU plugin histogram, the histogram total MUST be
    within a small tolerance of QEMU's `LINX_INSN_COUNT` (translation artifacts
    aside), and both totals MUST be reported.
- Pass criteria:
  - `workloads/benchmarks/run_benchmarks.py --dynamic-hist` produces a report
    that includes both totals and stable percentages based on the plugin total.
