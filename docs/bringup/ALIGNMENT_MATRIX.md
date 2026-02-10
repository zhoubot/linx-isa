# Linx Stack Alignment Matrix (v0.2)

Last updated: 2026-02-10

This document tracks **end-to-end alignment** of the Linx stack (spec → compiler → emulator → OS → RTL/models) for
bring-up-critical features. It is intended to be kept up-to-date as work lands across repos under `~/`.

Legend:

- ✅ implemented + tested
- 🟡 implemented but missing tests / incomplete
- ❌ not implemented

## Feature matrix

| Feature | Spec (linxisa) | Golden (linxisa) | LLVM (llvm-project) | QEMU (qemu) | Linux (linux) | RTL (linxisa/Janus) | pyCircuit/Janus SW model | Tests/Gates |
|---|---|---|---|---|---|---|---|---|
| Decoupled blocks (`BSTART.<type>` header + `B.TEXT` body) | ✅ `docs/architecture/isa-manual/src/chapters/04_block_isa.adoc` (`blockisa-forms-decoupled`) | ✅ `isa/spec/current/linxisa-v0.2.json` (opcode: `B.TEXT`) | ✅ `llvm/lib/Target/LinxISA/LinxISABlockify.cpp` | ✅ `target/linx/insn32.decode`, `target/linx/translate.c`, `target/linx/cpu.h` | 🟡 `arch/linx/` trap/resume aligned; decoupled execution not used by Linux yet | ❌ (pending) | ❌ (pending) | ✅ QEMU: `qemu/scripts/linxisa/run-tile-copy-btext.sh`; ✅ LLVM lit: `llvm/test/CodeGen/LinxISA/*` |
| Restartable templates (`FENTRY/FEXIT/FRET*/MCOPY/MSET`) | ✅ `docs/architecture/isa-manual/src/chapters/06_templates.adoc`, `docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc` | ✅ `isa/spec/current/linxisa-v0.2.json` (template opcodes) | ✅ prologue/epilogue + template emission: `llvm/lib/Target/LinxISA/*` | ✅ `target/linx/translate.c`, `target/linx/helper.c` | 🟡 trap routing + `TRAPNO` decode aligned; EBSTATE/template-restart integration pending | ❌ (pending) | ❌ (pending) | ✅ QEMU: `qemu/scripts/linxisa/run-mcopy-mset-basic.sh` |
| v0.2 trap ABI (`TRAPNO` encoding + `EBARG` group + `ACRC/ACRE`) | ✅ `docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc`, `docs/architecture/isa-manual/src/generated/trapno_encoding.adoc` | ✅ `isa/golden/v0.2/state/system_registers.json` (`trapno_encoding`, `ebarg_group`) | ✅ SSR symbol names + EBARG IDs in `llvm/lib/Target/LinxISA/{AsmParser,MCTargetDesc}` | ✅ `target/linx/{cpu.c,helper.c,translate.c}` | ✅ `arch/linx/kernel/{entry.S,traps.c}` | ❌ (pending) | ❌ (pending) | ✅ `tools/isa/validate_spec.py` (v0.2 guards); ✅ denylist gate `tools/isa/check_no_legacy_v02.py`; ✅ cross-repo legacy scan (`--extra-root`); ✅ QEMU runtime: `tests/qemu/tests/11_system.c` |
| Debug BP/WP SSRs + traps (linking bring-up subset) | ✅ `isa/golden/v0.2/state/system_registers.json` (`debug_ssr`) + manual text | ✅ `isa/spec/current/linxisa-v0.2.json` | ✅ SSR symbol names in `llvm/lib/Target/LinxISA/{AsmParser,MCTargetDesc}` | ✅ `target/linx/helper.c` (matching + trap delivery) | ✅ trap decode + SIGTRAP: `arch/linx/kernel/traps.c` | ❌ (pending) | ❌ (pending) | ✅ directed runtime tests: `tests/qemu/tests/11_system.c` (DBG_BP/DBG_WP/DBG_BP_RESUME); ✅ strict gate: `tools/regression/run.sh` (`run_tests.py --suite system --require-test-id 0x110E`) |
| ESAVE/ERCOV template blocks | ✅ `docs/architecture/isa-manual/src/chapters/06_templates.adoc` | ✅ `isa/golden/v0.2/opcodes/lx_32.opc` + `isa/spec/current/linxisa-v0.2.json` | 🟡 asm/disasm pending | ✅ `target/linx/helper.c` | ❌ (pending) | ❌ (pending) | ❌ (pending) | ❌ directed tests (pending) |
| TTBR0/TTBR1 CPU MMU | ✅ `docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc` | ✅ `isa/spec/current/linxisa-v0.2.json` | ❌ (none) | ✅ `target/linx/cpu.c` (page walk), `target/linx/helper.c` (TLB maint) | ❌ (CONFIG_MMU=n; page tables not implemented) | ❌ (pending) | ❌ (pending) | ✅ QEMU: `qemu/scripts/linxisa/run-mmu-ttbr-basic.sh` |
| IOMMU (DMA/TMA translation) | ✅ `docs/architecture/isa-manual/src/chapters/09_system_and_privilege.adoc` | ✅ `isa/spec/current/linxisa-v0.2.json` | ❌ (none) | ✅ `target/linx/helper.c` (tile IOMMU walk) | ❌ (pending) | ❌ (pending) | ❌ (pending) | ✅ QEMU: `qemu/scripts/linxisa/run-iommu-tile-basic.sh` |
| TMA `TLOAD/TSTORE` ordering vs scalar LSU | ✅ `isa/golden/v0.2/state/memory_model.json` | ✅ `isa/spec/current/linxisa-v0.2.json` | 🟡 (compiler emits ordered blocks; fence/aq/rl coverage pending) | 🟡 enforce in `target/linx/` (serialize at block boundaries) | ❌ (pending) | ❌ (pending) | ❌ (pending) | 🟡 directed/litmus pending |
