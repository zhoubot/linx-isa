# include/pto/npu/a2a3/

Ascend A2/A3 series PTO instruction implementation headers.

## Overview

- Implementations are organized per instruction (or instruction family), for example: `TAdd.hpp`, `TMatmul.hpp`, `TLoad.hpp`, `TStore.hpp`
- Some shared operator patterns are also provided (for example, Reduce/Expand/PartOp helpers)

## Related

- ISA semantics and examples: `docs/isa/`
- A2/A3 NPU ST tests: `tests/npu/a2a3/src/st/`
