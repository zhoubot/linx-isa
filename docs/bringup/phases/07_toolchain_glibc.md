# Phase 7: Toolchain/libc Support and Closure

Primary automation/scripts:

- `tools/glibc/bringup_linx64.sh`
- `tools/glibc/patches/0001-glibc-linx-bringup.patch`

## Objective

Close impl/compiler/linker/libc gaps needed by Linux-on-FPGA milestones and sustained userspace validation.

## Role in the bring-up sequence

- This is a support/closure phase for phases 4-6, not a replacement for RTL/FPGA/Linux gates.
- Prioritize blockers that directly affect Linux boot and userspace stability.

## Current direction

- Keep compiler compatibility moving for glibc sources.
- Resolve linker/sysroot/ABI issues in the Linx Linux toolchain path.
- Close runtime blockers around syscall/trap handling and loader/process support.
- Treat dynamic linking as a later milestone after static bring-up stability.

## Exit criteria

- Toolchain and libc state no longer block D3 (Janus MMU Linux shell gate).
- Remaining gaps are explicit, scoped, and non-blocking for core bring-up milestones.
