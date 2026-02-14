# LLVM Backend Implementation Checklist

This checklist tracks the implementation of the LinxISA LLVM backend. Use this to track progress and ensure all components are implemented.

## Phase 1: Core Instruction Support

### TableGen Definitions
- [ ] Generate instruction patterns from ISA spec using `tools/isa/gen_llvm_tablegen.py`
- [ ] Integrate generated patterns into `LinxInstrInfo.td`
- [ ] Define instruction formats in `LinxInstrFormats.td`:
  - [ ] 16-bit compressed format
  - [ ] 32-bit base format
  - [ ] 48-bit extended format
  - [ ] 64-bit prefixed format
- [ ] Define operand types (simm12, uimm5, etc.)

### Instruction Selection
- [ ] Implement `LinxISelLowering.cpp`:
  - [ ] Lower arithmetic operations (ADD, SUB, MUL, DIV)
  - [ ] Lower memory operations (LWI, SWI, LDI, SDI, LDL, SDL)
  - [ ] Lower control flow (branches, calls, returns)
  - [ ] Lower immediate materialization (LUI, ADDI, HL.LUI, HL.ADDI)
  - [ ] Lower PC-relative addressing (ADDPC, LDL, SDL)
- [ ] Add instruction selection patterns in TableGen
- [ ] Handle instruction length selection (16/32/48/64-bit)

### Testing
- [ ] Run test suite: `impl/compiler/llvm/tests/run.sh`
- [ ] Verify arithmetic operations compile correctly
- [ ] Verify memory operations compile correctly
- [ ] Verify control flow compiles correctly

## Phase 2: Block ISA Enhancement

### Block Formation Pass
- [ ] Create `LinxBlockFormation.cpp` pass
- [ ] Insert BSTART at beginning of each MachineBasicBlock
- [ ] Insert BSTOP before terminators
- [ ] Handle block transitions (fall-through, branches, calls)
- [ ] Implement block type detection (STD, COND, DIRECT, CALL, RET)

### BSTART Variants
- [ ] Implement `BSTART.STD`
- [ ] Implement `BSTART.FALL`
- [ ] Implement `BSTART.COND`
- [ ] Implement `BSTART.DIRECT`
- [ ] Implement `BSTART.CALL`
- [ ] Implement `BSTART.RET`
- [ ] Implement `BSTART.IND`
- [ ] Implement `BSTART.ICALL`
- [ ] Implement `C.BSTART.*` compressed forms

### SETC Variants
- [ ] Implement `SETC.EQ`
- [ ] Implement `SETC.NE`
- [ ] Implement `SETC.LT`
- [ ] Implement `SETC.GE`
- [ ] Implement `SETC.LTU`
- [ ] Implement `SETC.GEU`
- [ ] Implement `C.SETC.*` compressed forms

### Block Semantics
- [ ] Ensure proper block boundaries
- [ ] Implement commit semantics
- [ ] Handle block-private register lifetimes (T/U queues)
- [ ] Test block ISA patterns with `24_block_isa.c`

## Phase 3: Tile Register Support

### Register Definitions
- [ ] Define Tile register classes in `LinxRegisterInfo.td`:
  - [ ] T registers (T0-T3)
  - [ ] U registers (U0-U3)
  - [ ] M registers
  - [ ] N registers
  - [ ] ACC register
- [ ] Define Tile register allocation constraints

### Tile Operation Lowering
- [ ] Create `LinxTileLowering.cpp`
- [ ] Implement TLOAD lowering:
  - [ ] 2D memory access patterns
  - [ ] Stride handling
  - [ ] Cacheable/non-cacheable
- [ ] Implement TSTORE lowering:
  - [ ] 2D memory write
  - [ ] Release semantics
- [ ] Implement TCVT lowering:
  - [ ] Data type conversion
  - [ ] Layout transformation
  - [ ] ATTEN operations
- [ ] Implement MAMULB lowering:
  - [ ] Matrix multiply
  - [ ] Matrix multiply-accumulate
- [ ] Implement MCALL mode switching

### Tile Register Allocation
- [ ] Create Tile register allocator pass
- [ ] Manage Tile register lifetimes within blocks
- [ ] Handle T/U hand queues (depth 4)
- [ ] Spill to GPR when Tile registers exhausted

## Phase 4: Variable-Length Instruction Support

### Code Emitter
- [ ] Implement `LinxMCCodeEmitter.cpp`:
  - [ ] 16-bit instruction encoding
  - [ ] 32-bit instruction encoding
  - [ ] 48-bit instruction encoding
  - [ ] 64-bit instruction encoding
- [ ] Implement instruction length selection logic
- [ ] Optimize encoding selection for code size

### Assembly Printer
- [ ] Update `LinxAsmPrinter.cpp`:
  - [ ] Print variable-length instructions correctly
  - [ ] Handle instruction alignment
  - [ ] Support instruction prefixes

### Instruction Selection Optimization
- [ ] Prefer shorter encodings when possible
- [ ] Use 16-bit compressed forms for common operations
- [ ] Use 48-bit extended forms for large immediates
- [ ] Use 64-bit prefixed forms only when necessary

## Phase 5: Test Coverage Expansion

### Test Programs
- [x] Create `20_floating_point.c`
- [x] Create `21_atomic.c`
- [x] Create `22_memory_ops.c`
- [x] Create `23_bit_manipulation.c`
- [x] Create `24_block_isa.c`
- [x] Create `25_immediate_materialization.c`
- [x] Create `26_reduce_operations.c`
- [x] Create `27_three_source.c`
- [x] Create `28_prefetch.c`
- [x] Create `29_cache_ops.c`
- [x] Create `30_compressed.c`

### Coverage Analysis
- [x] Create `analyze_coverage.py` tool
- [ ] Run coverage analysis after each phase
- [ ] Identify missing instruction groups
- [ ] Add tests for missing groups

### Validation
- [ ] All test programs compile successfully
- [ ] Coverage reaches 100% of spec instructions
- [ ] Generated code matches ISA spec encoding
- [ ] No regressions in existing tests

## Phase 6: Naming Consistency

### Rename "LinxISA" to "Linx"
- [ ] Update all class names:
  - [ ] `LinxISATargetInfo` → `LinxTargetInfo`
  - [ ] `LinxISAInstrInfo` → `LinxInstrInfo`
  - [ ] `LinxISARegisterInfo` → `LinxRegisterInfo`
  - [ ] All other classes
- [ ] Update file names:
  - [ ] `LinxISA*.cpp` → `Linx*.cpp`
  - [ ] `LinxISA*.h` → `Linx*.h`
  - [ ] `LinxISA*.td` → `Linx*.td`
- [ ] Update comments and strings
- [ ] Update TableGen definitions
- [ ] Update documentation
- [ ] Update CMakeLists.txt and build files

### Consistency Check
- [ ] Search codebase for "LinxISA" (should find none)
- [ ] Verify all references use "Linx"
- [ ] Update test scripts if needed

## Files to Create/Modify

### Core Files (in `~/llvm-project/llvm/lib/Target/Linx/`)
- [ ] `Linx.td` - Target definition
- [ ] `LinxInstrInfo.td` - Instruction definitions
- [ ] `LinxInstrFormats.td` - Instruction formats
- [ ] `LinxRegisterInfo.td` - Register definitions
- [ ] `LinxCallingConv.td` - Calling conventions
- [ ] `LinxSubtarget.h/cpp` - Subtarget features
- [ ] `LinxISelLowering.h/cpp` - Instruction selection
- [ ] `LinxISelDAGToDAG.cpp` - DAG instruction selection
- [ ] `LinxAsmPrinter.cpp` - Assembly printer
- [ ] `LinxInstPrinter.cpp` - Instruction printer
- [ ] `LinxMCInstLower.cpp` - MC instruction lowering
- [ ] `LinxMCCodeEmitter.cpp` - Instruction encoding
- [ ] `LinxAsmBackend.cpp` - Assembler backend
- [ ] `LinxELFObjectWriter.cpp` - ELF object writer
- [ ] `LinxBlockFormation.cpp` - Block formation pass (new)
- [ ] `LinxTileLowering.cpp` - Tile operation lowering (new)

### Clang Files (in `~/llvm-project/clang/lib/`)
- [ ] `Basic/Targets/Linx.h` - Target info
- [ ] `Driver/ToolChains/Linx.cpp` - Toolchain support

## Tools and Scripts

### In This Repository
- [x] `tools/isa/gen_llvm_tablegen.py` - TableGen generator
- [x] `impl/compiler/llvm/tests/analyze_coverage.py` - Coverage analysis
- [x] `impl/compiler/llvm/templates/` - Implementation templates

### Usage
```bash
# Generate TableGen patterns
python3 tools/isa/gen_llvm_tablegen.py \
  --spec spec/isa/spec/current/linxisa-v0.3.json \
  --out impl/compiler/llvm/LinxInstrInfo.td

# Run tests
cd impl/compiler/llvm/tests
CLANG=~/llvm-project/build-linxisa-clang/bin/clang ./run.sh

# Analyze coverage
python3 impl/compiler/llvm/tests/analyze_coverage.py --verbose
```

## Validation Criteria

- [ ] All 774 instructions from spec are compilable
- [ ] Test suite passes with 100% instruction coverage
- [ ] Generated code matches ISA spec encoding
- [ ] Block ISA semantics are correctly implemented
- [ ] Tile operations compile and execute correctly
- [ ] Variable-length instructions encode correctly
- [ ] Code size is optimized (prefer shorter encodings)
- [ ] No "LinxISA" references remain (all use "Linx")

## Notes

- LLVM backend code is in `~/llvm-project/llvm/lib/Target/Linx/`
- Use templates in `impl/compiler/llvm/templates/` as starting points
- Run tests frequently to catch regressions
- Update this checklist as you complete items
