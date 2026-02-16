# LinxISA Precise Call/Ret Contract (linx64)

This document is normative for compiler, emulator, runtime, and Linux cross-check work.

## 1) Function Entry/Exit Forms

Normal function path:

- Entry must use `FENTRY`.
- Return must use `FRET.STK`.
- Canonical form is `FENTRY ... FRET.STK`.

Tail-transfer path:

- Entry still uses `FENTRY`.
- Tail exit uses `FEXIT`.
- Control transfer after `FEXIT` must be block-legal (direct or indirect block transfer).
- Canonical form is `FENTRY ... FEXIT`.

`FRET.RA` is valid when return target is consumed from pre-restore `ra` by design, but standard C ABI returns use `FRET.STK`.

## 2) Return-Target Semantics

- `FRET.STK`: return target comes from restored `ra` state loaded from the frame.
- `FRET.RA`: return target comes from `ra` before stack-restore return resolution.
- `BSTART.RET` blocks must include explicit target setup:
  - `setc.tgt <src>` where `<src>` resolves to `ra` for normal returns.

Required `RET` block form:

```asm
C.BSTART.RET
c.setc.tgt ra
C.BSTOP
```

## 3) Call Header Contract

Returning call headers are architecturally fused:

- `BSTART.CALL + C.SETRET` for compressed/direct call headers.
- `BSTART.CALL + SETRET` for non-compressed forms.

Adjacency rule for returning calls:

- `SETRET/C.SETRET` must be immediately adjacent to the corresponding `BSTART.CALL`.
- No instruction may be scheduled between call-header and setret materialization.
- Return target is the explicit label encoded by `setret`, not the lexical fall-through.

Non-returning call headers:

- `BSTART.CALL` without `SETRET` is valid only for non-returning control transfer paths.
- In this form, `ra` is preserved (no implicit return-target rewrite).
- If control eventually returns and the dynamic target is not a legal block start, dynamic target safety checks must fault.

Required fused form:

```asm
BSTART.CALL, callee
c.setret .Lret, ->ra
```

Non-fallthrough return form is valid and common:

```asm
BSTART.CALL, callee
setret .Ljoin, ->ra
... call block body ...
C.BSTOP

... unrelated blocks ...

.Ljoin:
C.BSTART.STD
```

Setret width selection:

- `c.setret`: short forward range only.
- `setret`: larger forward range only.
- `hl.setret`: wide signed range (forward/backward); required when smaller forms cannot encode the return label.

## 4) Indirect Target Setup Rules

Before any `RET`, `IND`, or `ICALL` block transfer, a `setc.tgt` must define the dynamic target register source in the same block.

Non-conforming sequences (`setc.tgt` missing, or non-adjacent `SETRET` in returning call headers) are contract violations and must trap in strict mode.

## 5) Dynamic Target Safety Rule

Dynamic control-flow targets from `RET`/`IND`/`ICALL` must resolve to legal block start markers (`BSTART*`, `C.BSTART*`, template block starts like `FENTRY/FEXIT/FRET.*`). Non-block targets must fault.

## 6) Cross-Stack Validation Anchors

Cross-check against Linux Linx implementation patterns:

- `/Users/zhoubot/linux/arch/linx/kernel/switch_to.S`
- `/Users/zhoubot/linux/arch/linx/kernel/entry.S`

These files are treated as authoritative reference behavior for return-target setup and call/return block sequencing.
