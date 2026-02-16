# Linx ASM ABI/Unwind/Context Checklist

Use this checklist when landing Linx64 asm changes in musl/glibc/runtime code.

## A) ABI Register Contract

- [ ] Uses linx64 ABI register map from `/Users/zhoubot/linux/Documentation/linxisa/abi.md`.
- [ ] Preserves callee-saved set (`s0..s8`, `sp`) in callable functions.
- [ ] Preserves `ra` semantics across calls/returns.
- [ ] Keeps stack 16-byte aligned at call boundaries.

## B) Block-ISA Legality

- [ ] Control flow is expressed with valid block markers (`BSTART/C.BSTART` + `BSTOP/C.BSTOP`).
- [ ] Conditional branches consume `SETC` inside the same conditional block.
- [ ] Indirect transfers use `setc.tgt` in an `IND` block.
- [ ] `RET` blocks use explicit target setup (`BSTART.RET` + `setc.tgt ra`).
- [ ] No direct illegal branch into middle of a block body.
- [ ] Dynamic `RET/IND/ICALL` targets are legal block starts.

## C) Call Header Fusion/Adjacency

- [ ] Returning `CALL` headers use fused form: `BSTART.CALL` + immediate adjacent `SETRET/C.SETRET`.
- [ ] No instructions are emitted between call header and setret materialization.
- [ ] Non-returning `CALL` headers without `SETRET` are explicit/intentional and keep `ra` unchanged.

## D) Syscall Path Correctness

- [ ] Syscall number is in `a7`.
- [ ] Args are in `a0..a5`.
- [ ] Trap instruction is `acrc 1` (not legacy fallback traps).
- [ ] Return value path preserves negative errno convention in `a0`.

## E) setjmp/sigsetjmp/longjmp

- [ ] `__jmp_buf` matches Linx ABI save set size (11 words).
- [ ] `setjmp` saves only ABI-preserved state (`s0..s8`, `sp`, `ra`).
- [ ] `longjmp` restores the same state and enforces `val==0 -> 1`.
- [ ] `sigsetjmp` routes through `__sigsetjmp_tail` for mask save/restore when `savemask!=0`.

## F) Signal/Restorer ABI

- [ ] Userspace `SA_RESTORER` is defined as `0x04000000`.
- [ ] `__restore_rt` issues `rt_sigreturn` (`a7=139`, `acrc 1`).
- [ ] No-op generic restorer fallback is not active for Linx.
- [ ] `mcontext/sigcontext/ucontext` layouts are Linux Linx UAPI-compatible.

## G) Unwind/Context-Switch Consistency

- [ ] Save/restore order is consistent with Linux Linx kernel patterns:
  - `/Users/zhoubot/linux/arch/linx/kernel/switch_to.S`
  - `/Users/zhoubot/linux/arch/linx/kernel/entry.S`
  - `/Users/zhoubot/linux/arch/linx/kernel/signal.c`
- [ ] Linux cross-stack check confirms call/ret target setup matches kernel patterns.
- [ ] Noreturn terminal stubs (`sigreturn`, `exit`, unmap-self path) do not expose fake unwind paths.
- [ ] Any context struct exported to userspace matches ptrace/signal expectations.

## H) Relocation/TLS Contract

- [ ] Linx arch relocation headers use canonical `R_LINX_*` constants.
- [ ] `CRTJMP` is a real control transfer (not a no-op).
- [ ] `dlsym` passes caller return-address metadata to `__dlsym`.
- [ ] `tlsdesc` stubs are arch implementations (no zero-return fallback).

## I) Runtime Gate Expectations

- [ ] Static and shared links both pass smoke gates.
- [ ] Shared runtime includes `/lib/libc.so` and `/lib/ld-musl-linx64.so.1`.
- [ ] QEMU tests cover:
  - signal delivery/restorer
  - setjmp/sigsetjmp/longjmp
  - thread create/join + TLS
  - dlopen/dlsym + TLS descriptor paths
