# PTO Kernels for LinxISA (v0.3)

This folder contains PTO tile kernels compiled through the LinxISA LLVM backend.

Naming policy:
- kernel source names do not use the legacy `pto_` prefix.
- kernel source names do not use the legacy `_auto` suffix.

Kernels:
- `tload_store.cpp`
- `mamulb.cpp`
- `tmatmul_acc.cpp`
- `gemm.cpp`
- `gemm_basic.cpp`
- `gemm_demo.cpp`
- `gemm_performance.cpp`
- `add_custom.cpp`
- `flash_attention.cpp`
- `flash_attention_demo.cpp`
- `flash_attention_masked.cpp`
- `fa_performance.cpp`
- `mla_attention_demo.cpp`

All kernels:
- include `common/pto_tileop.hpp` from `lib/pto/include`,
- use `global_tensor` + `global_iterator` addressing for TLOAD/TSTORE shape/stride inference,
- iterate over large tensors with nested tile loops,
- obey strict tile-byte legality (`<=4KB`) for both tile descriptors and TMATMUL footprints using
  `tile_bytes = ceil(dim0*dim1*dim2*elem_bits/8)` (`dim2=1` when absent).
- use strict-v0.3 DataType encoding (`FP64/FP32/FP16/FP8/BF16/FPL8/FP4/FPL4`,
  `INT64/INT32/INT16/INT8/INT4`, `UINT64/UINT32/UINT16/UINT8/UINT4`) in compiler and runtime checks.

Runtime profile policy:
- default full profile keeps original larger tensor domains for compile/asm bring-up.
- `PTO_QEMU_SMOKE=1` enables reduced runtime domains for QEMU execution while preserving tile-op and loop-path coverage.
- masked kernels keep non-zero remainder paths in smoke profile.

Parity gate:
- host-vs-QEMU parity runner: `/Users/zhoubot/linx-isa/tools/pto/run_pto_kernel_parity.py`
- report artifacts:
  - `/Users/zhoubot/linx-isa/workloads/generated/pto_kernel_parity_latest.json`
  - `/Users/zhoubot/linx-isa/workloads/generated/pto_kernel_parity_latest.md`

Objdump artifacts:
- per-kernel objects and disassembly are generated under:
  - `/Users/zhoubot/linx-isa/workloads/generated/pto_objdump/obj`
  - `/Users/zhoubot/linx-isa/workloads/generated/pto_objdump/dis`
