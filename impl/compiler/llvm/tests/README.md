# Linx clang compile tests

This folder contains small **freestanding C** programs intended to exercise the
current Linx clang/LLVM backend.

- Sources: `c/*.c`
- Outputs (generated): `out/<testname>/{<testname>.s,<testname>.o,<testname>.bin,<testname>.objdump,...}`

Run:

```bash
CLANG=/path/to/clang ./impl/compiler/llvm/tests/run.sh
```

Run (linx32):

```bash
CLANG=/path/to/clang TARGET=linx32-linx-none-elf OUT_DIR=./impl/compiler/llvm/tests/out-linx32 ./impl/compiler/llvm/tests/run.sh
```

## C test programs

### Basic Operations
- `01_arith.c` — integer arithmetic + immediates
- `02_control_flow.c` — if/loops/branches
- `03_arrays.c` — array indexing + pointer arithmetic
- `04_structs.c` — struct loads/stores
- `05_switch.c` — switch lowering
- `06_recursion.c` — calls + recursion
- `07_constants.c` — constant materialization
- `08_loadstore.c` — loads/stores (byte/half/word/dword)
- `09_bitops.c` — shifts/bitwise ops
- `10_select.c` — `cmp.*` + `csel`
- `11_minmax.c` — min/max patterns (often `csel`)
- `12_more_ops.c` — div/rem, shifts, more compares
- `13_br_ge.c` — force `b.ge` / `b.geu`
- `14_lui.c` — force `lui` materialization
- `15_i32_imms.c` — force `andiw` / `oriw` / `xoriw`
- `16_andorw.c` — force `andw` / `orw`
- `17_indexed.c` — indexed addressing (`[base, idx<<shamt]`)
- `18_setret_relax.c` — force `setret` (relax from `c.setret`)
- `19_shifted_add.c` — force `add x, y<<shamt` peepholes

### Extended Operations
- `20_floating_point.c` — floating-point arithmetic, comparisons, conversions
- `21_atomic.c` — atomic load/store/exchange/operations
- `22_memory_ops.c` — load/store pairs, pre/post-index, unscaled, PC-relative
- `23_bit_manipulation.c` — bit counting, rotation, field operations
- `24_block_isa.c` — Block ISA patterns, nested blocks, calls, switch
- `25_immediate_materialization.c` — immediate handling (small/medium/large)
- `26_reduce_operations.c` — reduction operations (sum, product, min, max, bitwise)
- `27_three_source.c` — three-source operations (FMA, multiply-subtract)
- `28_prefetch.c` — prefetch operations
- `29_cache_ops.c` — cache maintenance operations
- `30_compressed.c` — compressed instruction forms (C.*)

Notes:
- The runner links each test object with a tiny runtime via `ld.lld` to resolve relocations, then extracts `.text` to
  a raw `.bin`.

## ISA coverage report

After running the tests, you can summarize which ISA mnemonics show up in the generated `.s`:

```bash
python3 ./impl/compiler/llvm/tests/report_isa_coverage.py
```

For detailed coverage analysis:

```bash
python3 ./impl/compiler/llvm/tests/analyze_coverage.py --verbose
```

This will show:
- Coverage statistics (spec vs emitted)
- Missing instruction groups
- Coverage by test program
- Detailed breakdown by instruction category
