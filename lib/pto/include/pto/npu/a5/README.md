# include/pto/npu/a5/

Ascend A5 series PTO instruction implementation headers.

## Overview

- Implementations are organized per instruction (or instruction family), for example: `TAdd.hpp`, `TMatmul.hpp`, `TLoad.hpp`, `TStore.hpp`
- Includes A5-specific operator patterns and utilities where applicable

## Related

- ISA semantics and examples: `docs/isa/`
- A5 NPU ST tests: `tests/npu/a5/src/st/`
