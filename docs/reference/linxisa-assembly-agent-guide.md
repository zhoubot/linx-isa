# LinxISA Assembly Agent Guide

This guide is the canonical agent-facing reference for writing LinxISA assembly in libc/runtime bring-up work.

## 1) Scalar ABI (linx64 Linux userspace)

Source of truth:

- `/Users/zhoubot/linux/Documentation/linxisa/abi.md`

Register contract:

- `R0` = `zero` (constant 0)
- `R1` = `sp`
- `R2..R9` = `a0..a7` (arguments/return + syscall args)
- `R10` = `ra`
- `R11..R19` = `s0..s8` (callee-saved)
- `R20..R23` = `x0..x3` (caller scratch)

Calling convention:

- Integer/pointer return: `a0`
- Integer/pointer args: `a0..a7`, then stack
- Callee-saved: `s0..s8` and `sp`
- Caller-saved: `a*`, `ra`, `x0..x3`
- Stack alignment: 16 bytes

Thread pointer:

- TLS base is modeled in SSR `0x0000` (`TP`) and accessed through `ssrget/ssrset`.

## 2) Block-Structured Control Flow Rules

Linx is block-structured; control flow must target legal block boundaries.

Required patterns:

- Use `BSTART ...` / `BSTOP` or `C.BSTART ...` / `C.BSTOP` around control-flow regions.
- Use direct block branches for static labels: `C.BSTART DIRECT, <label>`.
- Use conditional block branches with `SETC` predicate in the same block:
  - `C.BSTART COND, <label>`
  - `setc.<cond> ...`
  - `C.BSTOP`
- Use indirect block transfer via target register:
  - `C.BSTART IND`
  - `setc.tgt <reg>`
  - `C.BSTOP`

Practical rule:

- Do not emit ad-hoc fallthrough/jump mixes that bypass block enter/exit markers.

## 3) Precise Call/Ret Contract (Mandatory)

Normal function form:

- Use `FENTRY` at function entry.
- Use `FRET.STK` for normal returns.
- Canonical pair is `FENTRY + FRET.STK`.

Tail-call form:

- Tail-call functions still enter with `FENTRY`.
- Use `FEXIT` for tail-transfer exit.
- Canonical tail pair is `FENTRY + FEXIT`, then block-legal transfer to callee.

`RET/IND/ICALL` target setup:

- `BSTART.RET` requires `setc.tgt` to define target source.
- Canonical return block is:
  - `C.BSTART.RET`
  - `c.setc.tgt ra`
  - `C.BSTOP`
- `IND` and `ICALL` blocks also require explicit `setc.tgt` in the same block.

Fused call header (returning calls):

- `CALL` is emitted as fused header pair:
  - `BSTART.CALL`
  - immediate adjacent `C.SETRET` (or `SETRET` in non-compressed form)
- No instruction may appear between `BSTART.CALL` and `SETRET/C.SETRET`.
- `SETRET` defines an explicit return block label (`ra` target); do not assume return is lexical fall-through.

Non-fallthrough return example:

```asm
BSTART.STD CALL, callee
setret .Lresume, ->ra
... call block body ...
C.BSTOP

... other blocks ...

.Lresume:
C.BSTART.STD
```

Setret width guidance:

- Prefer smallest legal form (`c.setret`, then `setret`).
- Use `hl.setret` when target range/layout cannot be encoded by smaller forms.
- `hl.setret` is part of normal correctness/relaxation support, not a special-case extension.

Non-returning call form:

- `BSTART.CALL` without `SETRET` is valid only for non-returning transfer paths.
- In this form, `ra` is preserved; any eventual return must still satisfy dynamic target safety.

## 4) Linux Syscall Template

Linx Linux userspace syscall ABI:

- Syscall number in `a7`
- Args in `a0..a5`
- Trap with `acrc 1`
- Return in `a0` (`<0` means `-errno`)

Reference template:

```asm
C.BSTART.STD
c.movr  <arg0>, ->a0
c.movr  <arg1>, ->a1
c.movr  <arg2>, ->a2
c.movr  <arg3>, ->a3
c.movr  <arg4>, ->a4
c.movr  <arg5>, ->a5
c.movr  <nr>,   ->a7
acrc 1
C.BSTART.STD RET
```

Notes:

- Keep `"memory"` clobber for inline-asm syscall helpers.
- Use `__syscall_ret` when libc API needs errno canonicalization.

## 5) setjmp / sigsetjmp / longjmp Invariants

Linx64 jmp ABI save set:

- `s0..s8`, `sp`, `ra` (11 slots total)

Rules:

- `setjmp` stores exactly the call-preserved set above.
- `longjmp` restores exactly that set and returns with:
  - `ret = val` if `val != 0`
  - `ret = 1` if `val == 0` (C standard)
- `sigsetjmp(env, savemask)`:
  - `savemask == 0` behaves like plain `setjmp`
  - `savemask != 0` must route through `__sigsetjmp_tail` so mask save/restore is symmetric across `siglongjmp`.

## 6) Signal Restorer Protocol (`rt_sigreturn`)

Linux contract:

- Userspace restorer symbol (`__restore_rt`) must issue `rt_sigreturn` syscall (`a7=139`, `acrc 1`).
- `SA_RESTORER` (`0x04000000`) must be available in userspace signal ABI.

Kernel side reference:

- `/Users/zhoubot/linux/arch/linx/kernel/signal.c`

Userspace requirements:

- `__restore` / `__restore_rt` must be arch implementations (no-op fallback is invalid for full Linux signal ABI).

## 7) Unwind and CFI Policy

Policy for bring-up parity:

- Preserve ABI-stable frame behavior in hand-written asm:
  - keep `sp` 16-byte aligned at call boundaries
  - preserve/restore callee-saved set exactly
- For stubs that intentionally do not unwind through normal return paths (`__restore_rt`, `__unmapself`, `exit` paths), treat them as noreturn terminal stubs.
- Avoid synthetic register save layouts that diverge from ABI docs; `setjmp`/context save structs must match exported headers.

## 8) Linux Context-Switch / Trap Patterns to Mirror

Primary references:

- `/Users/zhoubot/linux/arch/linx/kernel/switch_to.S`
- `/Users/zhoubot/linux/arch/linx/kernel/entry.S`
- `/Users/zhoubot/linux/arch/linx/kernel/signal.c`

Patterns to reuse in userspace arch asm:

- Save/restore order for callee-saved state (`s0..s8`, `sp`, `ra`) is stable and explicit.
- Trap/return flow uses SSR snapshots and restores the exact interrupted context.
- Signal frame setup/restore expects userspace restorer + aligned frame.

Agent checklist before submitting asm:

1. Are block markers legal and balanced?
2. Are returning `CALL` headers fused (`BSTART.CALL` + adjacent `SETRET/C.SETRET`), and non-returning calls explicitly intentional?
3. Do all `RET/IND/ICALL` blocks set target via `setc.tgt`?
4. Is ABI save/restore set minimal and correct?
5. Does syscall path use `a7 + acrc 1`?
6. Are signal restorer and `SA_RESTORER` wired?
7. Does `longjmp` normalize `0 -> 1`?
8. Are context structs consistent with Linux UAPI headers?
