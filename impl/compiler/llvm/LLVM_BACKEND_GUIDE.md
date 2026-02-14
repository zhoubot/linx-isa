# LLVM Backend Implementation Guide

This guide helps implement the LinxISA LLVM backend. The backend code is located in `~/llvm-project/llvm/lib/Target/Linx/` (or `LinxISA/` depending on naming).

## Directory Structure

The LLVM backend should be organized as follows:

```
llvm/lib/Target/Linx/
├── Linx.td                    # Target definition
├── LinxInstrInfo.td           # Instruction definitions (TableGen)
├── LinxRegisterInfo.td         # Register definitions
├── LinxCallingConv.td          # Calling conventions
├── LinxSubtarget.h/cpp         # Subtarget features
├── LinxISelLowering.h/cpp      # Instruction selection lowering
├── LinxISelDAGToDAG.cpp        # DAG instruction selection
├── LinxAsmPrinter.cpp          # Assembly printer
├── LinxInstPrinter.cpp         # Instruction printer
├── LinxMCInstLower.cpp         # MC instruction lowering
├── LinxMCCodeEmitter.cpp       # Instruction encoding
├── LinxAsmBackend.cpp          # Assembler backend
├── LinxELFObjectWriter.cpp     # ELF object writer
├── LinxBlockFormation.cpp      # Block formation pass (new)
└── LinxTileLowering.cpp        # Tile operation lowering (new)
```

## Generating Instruction Patterns

Use the tool in this repo to generate TableGen instruction patterns:

```bash
python3 tools/isa/gen_llvm_tablegen.py \
  --spec spec/isa/spec/current/linxisa-v0.3.json \
  --out impl/compiler/llvm/LinxISAInstrInfo.td
```

Then integrate the generated patterns into `LinxInstrInfo.td`.

## Instruction Selection

### Arithmetic Operations

Map LLVM IR operations to LinxISA instructions:

- `add` → `ADD`, `ADDI`, `HL.ADDI` (based on immediate size)
- `sub` → `SUB`, `SUBI`
- `mul` → `MUL`
- `udiv`/`sdiv` → `DIV` variants
- `urem`/`srem` → `REM` variants

### Memory Operations

- `load` → `LWI`, `LDI`, `LDL` (based on addressing mode)
- `store` → `SWI`, `SDI`, `SDL`
- Load/store pairs → `LDP`/`STP` variants
- Pre/post-index → appropriate variants

### Control Flow

- `br` → `BSTART.*` variants
- `call` → `BSTART.CALL`
- `ret` → `BSTART.RET`
- `switch` → `BSTART` with jump table

### Block ISA

Each `MachineBasicBlock` should:
1. Start with `BSTART.*` or `C.BSTART.*`
2. End with `BSTOP` or implicit termination
3. Use `SETC.*` for conditional execution

## Register Allocation

### GPR Registers

- R0-R23: Standard ABI registers
- R24-R55: XGPR (if implemented)

### Tile Registers

Tile registers (T, U, M, N, ACC) should be:
- Modeled as virtual registers during codegen
- Assigned in a post-RA pass
- Managed with proper lifetimes within blocks

## Variable-Length Instructions

Support instruction length selection:

- 16-bit: Compressed forms (C.*)
- 32-bit: Base instructions
- 48-bit: Extended forms (HL.*)
- 64-bit: Prefixed forms (V.*)

The `MCCodeEmitter` should select the shortest encoding that fits.

## Testing

Run the test suite:

```bash
cd impl/compiler/llvm/tests
CLANG=~/llvm-project/build-linxisa-clang/bin/clang ./run.sh
```

Analyze coverage:

```bash
python3 impl/compiler/llvm/tests/analyze_coverage.py --verbose
```

## Naming Convention

**Important**: The plan specifies renaming "LinxISA" to "Linx" throughout the codebase.

- Class names: `LinxTargetInfo`, `LinxInstrInfo`, etc. (not `LinxISA*`)
- File names: `Linx*.cpp`, `Linx*.td` (not `LinxISA*`)
- Comments and strings: Use "Linx" not "LinxISA"

## Implementation Checklist

### Phase 1: Core Instructions
- [ ] Generate TableGen patterns from ISA spec
- [ ] Implement arithmetic instruction selection
- [ ] Implement memory instruction selection
- [ ] Implement control flow instruction selection
- [ ] Support immediate materialization
- [ ] Implement PC-relative addressing

### Phase 2: Block ISA
- [ ] Implement BSTART variants
- [ ] Implement SETC variants
- [ ] Implement block formation pass
- [ ] Handle block-private registers (T/U queues)

### Phase 3: Tile Registers
- [ ] Define Tile register classes
- [ ] Implement TLOAD/TSTORE lowering
- [ ] Implement TCVT operations
- [ ] Implement matrix operations (MAMULB, etc.)

### Phase 4: Variable-Length
- [ ] Implement instruction length selection
- [ ] Support 16/32/48/64-bit encodings
- [ ] Optimize for code size

### Phase 5: Testing
- [ ] Run all test programs
- [ ] Achieve 100% instruction coverage
- [ ] Fix any failures

### Phase 6: Naming
- [ ] Rename all "LinxISA" to "Linx"
- [ ] Update documentation
- [ ] Verify consistency

## Resources

- ISA Spec: `spec/isa/spec/current/linxisa-v0.3.json`
- Codec Tables: `spec/isa/generated/codecs/linxisa*.decode`
- Test Programs: `impl/compiler/llvm/tests/c/*.c`
- Coverage Tool: `impl/compiler/llvm/tests/analyze_coverage.py`
