# Compiler

Compiler work is split into:

- Upstream LLVM source mirror (submodule): `compiler/llvm/`
- LinxISA compiler integration assets/tests: `compiler/linx-llvm/`
- Future GCC track placeholder: `compiler/gcc/`

All encode/decode and instruction definitions must reference `spec/isa/spec/current/linxisa-v0.3.json`.

Canonical compiler bring-up plan and status: `compiler/COMPILER_PLAN.md`.
