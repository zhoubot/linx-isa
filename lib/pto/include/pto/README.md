# include/pto/

This is the primary public header entry for PTO Tile Lib. It contains:

- The Tile type system and shared utilities
- PTO instruction API declarations (Auto/Manual forms)
- CPU simulation/stub support
- NPU instruction implementations (split by SoC generation)

## Recommended Include

- `include/pto/pto-inst.hpp`: Unified entry header (recommended for upper-layer code)

In CPU simulation scenarios, this header can include CPU stubs (for example, when `__CPU_SIM` is defined it pulls in `pto/common/cpu_stub.hpp`).

## Layout

- `common/`: Platform-independent Tile and instruction infrastructure
  - `pto_tile.hpp`: Core Tile types and layout
  - `pto_instr.hpp`, `pto_instr_impl.hpp`: Instruction declarations and shared implementations
  - `memory.hpp`, `constants.hpp`, `utils.hpp`, `type.hpp`: Common utilities and constants
- `cpu/`: CPU-side simulation/debug support (if enabled)
- `npu/`: NPU-side implementations, split by SoC version
  - `npu/a2a3/`: Ascend A2/A3 series
  - `npu/a5/`: Ascend A5 series

## Related Docs

- Instruction reference: `docs/isa/`
