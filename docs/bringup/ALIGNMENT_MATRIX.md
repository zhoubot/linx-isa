# Alignment Matrix

This matrix tracks cross-domain alignment at the current workspace scope.

| Topic | Spec | Compiler | Emulator | Kernel | Model | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Linx Linux libc ABI + relocation contract (`EM_LINXISA`, `R_LINX_*`, `setjmp/signal/ucontext`) | ✅ ABI guide/checklist + musl/glibc header sync | ✅ workspace clang call/ret relocation+template gates pass (`FENTRY/FRET.STK` vs musttail `FENTRY/FEXIT`) | ⚠ Linux+musl runtime still traps with `LINX_EBLOCK_CAUSE_BAD_BRANCH_TARGET` (`a=0x83c9c`, `pc=0x83cb0`) | ✅ cross-stack object audit passes in default semantic mode (strict reloc-only mode still available) | ⚠ dynamic/runtime path alignment still under debug | `out/libc/musl/logs/phase-b-summary.txt`; `avs/qemu/out/musl-smoke/summary.json`; `avs/qemu/out/callret-contract`; `tools/ci/check_linx_callret_crossstack.sh`; `avs/compiler/linx-llvm/tests/check_callret_relocs.py`; `avs/compiler/linx-llvm/tests/check_callret_templates.py` |
| Block/descriptor contracts (`B.ARG/B.IOR/B.IOT/C.B.DIMI`) | ✅ manual + generated refs | ✅ descriptor emission/tests | ✅ descriptor execution + AVS gates | ✅ userspace boot not regressed | ✅ trace-compatible bring-up subset | `bash tools/regression/run.sh` |
| ACR/IRQ/exception correctness | ✅ privileged chapter + trap table | ✅ MC symbols + encodings | ✅ strict system tests | ✅ smoke/full/virtio boots pass | ✅ qemu-vs-pyc commit diff pass | `avs/qemu/check_system_strict.sh` |
| ISA catalog parity (`v0.3`) | ✅ golden + current json | ✅ compile coverage tests | ✅ decode/execute gates | ✅ no legacy refs in runtime scripts | ✅ model-side contract checks | `python3 tools/isa/check_no_legacy_v03.py --root .` |
| AVS consolidation | ✅ matrix maintained in `avs/` | ✅ compile tests under `avs/compiler/linx-llvm/tests` | ✅ runtime tests under `avs/qemu` | ✅ n/a | ✅ n/a | `bash tools/ci/check_repo_layout.sh` |
