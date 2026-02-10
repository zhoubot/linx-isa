# LinxISA

This directory is the **single source of truth** for the LinxISA instruction-set definition used by:
- the reference emulator
- the compiler/toolchain
- the C++ core models
- the RTL implementation

## What is authoritative?

- Golden sources (authoritative, current): `isa/golden/v0.2/**`
- Compiled catalog (checked in, current): `isa/spec/current/linxisa-v0.2.json`
- Legacy catalog (kept for reference): `isa/spec/current/linxisa-v0.1.json`

Build the compiled catalog from golden sources:

```bash
python3 tools/isa/build_golden.py --in isa/golden/v0.2 --out isa/spec/current/linxisa-v0.2.json --pretty
```

## ELF Machine Type

LinxISA uses ELF machine type `EM_LINXISA = 233` (0xE9).

This value is defined in:
- LLVM: `llvm/include/llvm/BinaryFormat/ELF.h`
- glibc: `elf/elf.h`

## Spec conventions (v0.2)

- **Bit numbering**: within an instruction part, `bit 0` is the least-significant bit (LSB) of that part.
- **Register encoding**: common register fields (`SrcL`, `SrcR`, `RegDst`, ...) use the 5-bit `registers.reg5` table in
  the JSON catalog (ABI aliases like `sp`, `a0`, `s0`, `t#1`, plus queue-push pseudo-dest selectors `t`/`u` used as
  `->t` / `->u`).
- **Variable-length instructions**: the draft defines 16/32/48/64-bit encodings using prefix/postfix composition.
  The extracted catalog represents these as one or more `parts`:
  - 16/32/48-bit forms typically have 1 part
  - 64-bit prefixed forms typically have 2 parts (shown as 2 rows in the draft tables)
- **Encoding segments**: each part contains `segments` describing `[msb:lsb]` bit ranges and the corresponding token
  from the draft tables (e.g. `SrcL`, `RegDst`, `3'b010`, `simm12`, etc). Constant tokens are also parsed into
  numeric values when possible.
- **Decode aids**: each instruction form also includes derived `encoding` information (`mask`, `match`, `pattern`,
  and `fields`) so decoders/encoders (QEMU, LLVM, etc.) can be generated from the spec.

## Block Split semantics (BSTART/BSTOP/SETC)

Linx uses an explicit *block split* ISA convention for control-flow:

- **Block start markers** (the architectural "bstart") define legal entry points for control-flow:
  - `BSTART.*` / `C.BSTART.*` / `HL.BSTART.*`
  - Template blocks such as `FENTRY` / `FEXIT` / `FRET.*` (and future macro blocks like `MCOPY`, `MSET`, `ESAVE`, `ERET`)
- **Safety rule:** every *control-flow target* (direct, conditional, indirect, call/return) must point at a block start
  marker. Branching to a non-block-start instruction raises an exception.

Block boundaries:

- A block starts at a block start marker.
- A block ends at an explicit `BSTOP` / `C.BSTOP` or implicitly at the next block start marker in the instruction
  stream.

## Block forms (v0.2 bring-up profile)

The bring-up profile defines three block forms:

1) **Coupled blocks** (default scalar CFG):
   - `BSTART{.<type>}  inst*  (BSTOP | next BSTART)`

2) **Decoupled blocks** (header + out-of-line body):
   - Header contains `BSTART.<type>` followed by **only** `B.*` descriptors and MUST include exactly one
     `B.TEXT <label>` selecting the body entrypoint.
   - Body begins at `BodyTPC` (from `B.TEXT`) and MUST be linear, terminating only at `BSTOP`/`C.BSTOP`.
   - Body MUST NOT contain any `BSTART.*`, template block, `B.*` descriptor, or any architectural control-flow
     instruction.
   - On body `BSTOP`, execution resumes at the header continuation (after header `BSTOP`, or at the next block start
     marker if the header ended implicitly).
   - `BodyTPC` is an engine-internal entrypoint and need not be a block start marker; architectural branches to it are
     illegal under the safety rule.

3) **Template blocks** (standalone, executed by a code-template generator):
   - `FENTRY`/`FEXIT`/`FRET.*`/`MCOPY`/`MSET` are standalone blocks and MUST NOT appear inside `BSTART..BSTOP`.
   - Templates are **restartable**: traps may occur between generated micro-ops; progress is recorded in
     `BSTATE/EBSTATE` and resumed via `ACRE RRAT_RESTORE`.

Conditional transitions use a two-step convention:

1. Set the block condition using `SETC.*` / `C.SETC.*`
2. Select the next block using a conditional block header (e.g. `BSTART.* COND, <target>`)

### Commit arguments (CARG)

Blocks carry block-internal control-flow state called **commit arguments** (**CARG**). CARG is similar in spirit to a
condition flags register on other architectures, but it is committed at block boundaries.

Bring-up model:

- CARG includes the block's intended exit kind (e.g. FALL/DIRECT/COND/CALL/RET/IND/ICALL), any predicate/condition, and
  the selected next target (PC-relative or register-based).
- `BSTART.*` initializes the block's branch kind/target defaults.
- `SETC.*` updates CARG (predicate/selection) and must execute *inside a block* (after a block start marker).
- At block commit (`BSTOP` or the next block start marker), the execution engine consults CARG and commits the
  block-to-block control-flow.
- Context switches must preserve CARG as part of the LXCPU architectural state.

### Assembly/disassembly conventions

- The default block type `.STD` is omitted in printed assembly:
  - `BSTART` means `BSTART.STD`
  - `C.BSTART` means `C.BSTART.STD`
- `RET` blocks use an explicit target selector:
  - `C.BSTART RET` then `c.setc.tgt ra`

For PC-relative forms, the offset fields are **halfword-scaled**:

- `target = PC + (simm<<1)` (e.g. `simm12`, `simm17`, `simm25` in `BSTART` / `C.BSTART`)
- `setret` targets are also halfword-scaled: `target = PC + (uimm5<<1)` / `PC + (imm20<<1)`

## MMU/IOMMU (v0.2 bring-up profile)

The v0.2 bring-up profile defines a real MMU + IOMMU using:

- **TTBR0/TTBR1 split** (ARM-inspired): `VA[47]=0` selects `TTBR0`, `VA[47]=1` selects `TTBR1`.
- 48-bit canonical virtual addresses, 4 KiB pages, 4-level tables (512 entries per level).
- Privileged configuration SSRs in `ACR1`: `TTBR0/TTBR1/TCR/MAIR` and `IOTTBR/IOTCR/IOMAIR` (see the ISA manual).

## Repository flow (target)

The intended end-to-end flow is:

1. **C source** (`.c/.h`)
2. **Compiler** (`compiler/`) emits LinxISA assembly / object code
3. **ISA spec** (`isa/spec/current/linxisa-v0.2.json`) is referenced for encoding/decoding + semantics
4. **Emulator** (`emulator/`) executes the same semantics as the spec
5. **RTL** (`rtl/`) implements the same decode + architectural behavior as the spec

## Interop

See `isa/integration/README.md` for notes on consuming the spec from QEMU/LLVM-style decoders/encoders.

## Codec

See `isa/generated/codecs/README.md` for generated encode/decode tables derived from the spec.

## T/U hand queues (ClockHands)

Linx provides two independent private result queues per basic block:

- **T-hand** queue: depth 4 (`t#1..t#4`), written by `->t`
- **U-hand** queue: depth 4 (`u#1..u#4`), written by `->u` (explicit; no `->` shorthand)

Rules:
- Each push shifts the queue and **kills** the oldest value; codegen must never reference killed entries.
- If an older value is needed after it would be killed, copy it out (or push it again) before it expires.
- The current bring-up assumes at most **one T read** and **one U read** per instruction.

Guidance:

- Prefer ClockHands (`t#k`/`u#k`) for **block-private dataflow** (values that do not cross a block boundary).
- Use GPRs (`r1`–`r23`) for **block-shared values** (values live across blocks).
