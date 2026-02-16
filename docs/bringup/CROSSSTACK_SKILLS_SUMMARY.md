# Linx Cross-Stack Skills Summary

This note summarizes practical skills and review focus for Linx call/ret and libc bring-up across Linux, LLVM, QEMU, and libc stacks.

## Linux

- Validate emitted call/ret patterns in kernel objects (`entry.o`, `switch_to.o`, `panic.o`).
- Keep return/indirect paths explicit (`BSTART.RET/IND` + `setc.tgt`).
- Cross-check with:
  - `/Users/zhoubot/linux/arch/linx/kernel/entry.S`
  - `/Users/zhoubot/linux/arch/linx/kernel/switch_to.S`
- Gate with `tools/ci/check_linx_callret_crossstack.sh`.

## LLVM

- Enforce call-header adjacency (`BSTART.CALL` followed immediately by setret form).
- Keep musttail lowering on `FEXIT` tail-transfer path; non-tail on `FRET.STK`.
- Preserve stable MC/disasm fused view (`CALL ..., ra=...`) and relocation legality.
- Add lit coverage for:
  - normal call/return shape
  - musttail tail-transfer shape
  - call-header adjacency constraints

## QEMU

- Treat strict contract violations as deterministic traps:
  - missing setret
  - invalid setret sequence
  - missing `setc.tgt` for `RET/IND/ICALL`
- Validate dynamic targets as legal block starts before transfer.
- Preserve call-header contract across TB boundaries (translator state must not be lost).
- Support all setret widths (`c.setret`, `setret`, `hl.setret` alias path).

## musl

- Keep linx64 arch ABI concrete (no riscv-derived placeholder layouts).
- Replace stubs with arch asm for `clone`, `sigsetjmp`, restorer, `__unmapself`, ldso paths.
- Keep relocation contract aligned to canonical `R_LINX_*`.
- Gate static/shared runtime behavior with `avs/qemu/run_musl_smoke.py`.

## glibc

- Keep syscall/trap mechanism aligned with Linx contract (`acrc 1` path).
- Keep relocation numbering and setjmp/signal/ucontext contracts aligned with Linux UAPI and musl.
- Avoid fallback relocation/shim behavior that diverges from musl/QEMU expectations.

## Common Review Checklist

1. Call headers: returning calls always have adjacent setret form.
2. Return/indirect: every `RET/IND/ICALL` path has explicit `setc.tgt`.
3. Targets: dynamic targets are block starts.
4. Tail calls: `FENTRY + FEXIT` only on musttail/tail-transfer path.
5. Runtime gates: compiler-only + freestanding QEMU + Linux+musl all pass.
