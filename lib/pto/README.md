# Vendored PTO Library (Linx)

This directory contains a vendored snapshot of PTO headers used by LinxISA tile
kernel bring-up.

## Upstream source

- Source repository: `/Users/zhoubot/pto-isa`
- Snapshot pin: `87997634327f73bfe69a26466052f3dc5b038c66`
- Vendored subtree: `include/pto/**`

## Local Linx deltas policy

- Keep upstream header layout under `include/pto/**`.
- Apply Linx-only integration changes in-place only where required for:
  - `__LINXISA__` compile support,
  - strict-v0.3 tile op legality/diagnostics,
  - LLVM Linx builtin/intrinsic backend mapping.
- Do not vendor `demos/`, `kernels/`, build outputs, or tool binaries.
- Keep this README updated when re-vendoring.

## Strict Tile Byte Policy (v0.3)

Linx strict-v0.3 enforces compile-time tile-byte limits in LLVM lowering for
tile block intrinsics.

Normative byte formula:

- `tile_bytes = ceil(dim0 * dim1 * dim2 * element_width_bits / 8)`
- if `dim2` is not explicitly present in the block descriptor, use `dim2 = 1`.

Strict v0.3 Tile DataType map (u5):

- floating: `FP64=0`, `FP32=1`, `FP16=2`, `FP8=3`, `BF16=6`, `FPL8=7`, `FP4=11`, `FPL4=12`
- signed int: `INT64=16`, `INT32=17`, `INT16=18`, `INT8=19`, `INT4=20`
- unsigned int: `UINT64=24`, `UINT32=25`, `UINT16=26`, `UINT8=27`, `UINT4=28`

Compiler checks:

- `tile_bytes` must not exceed strict hardware cap `4KB`.
- if a descriptor carries `SizeCode`, then
  `tile_bytes <= 2^(SizeCode+4)` must also hold.
- violations are hard errors (backend abort with actionable diagnostics).

CUBE/TMATMUL implication:

- `dim0=m`, `dim1=n`, `dim2=k`.
- example: `m=16, n=16, k=16, INT32(32b)` gives
  `ceil(16*16*16*32/8) = 16384B (16KB)` and is rejected in strict-v0.3.
- kernels must shrink dimensions and/or use narrower element width to stay
  within `4KB`.

## Legacy `pto::linx::TileOps.hpp` Defaults

Legacy pointer-only wrappers (used by bring-up tests) now use strict-safe
defaults when users do not pass explicit descriptor dims:

- dtype defaults to `INT32 (17)` (strict v0.3 mapping).
- `LB0/LB1` default from `SizeCode`:
  - `5 -> 16x8`
  - `6 -> 16x16`
  - `7 -> 32x16`
  - `8 -> 32x32`
- `stride_bytes` defaults to `LB0 * sizeof(int32_t)`.

This keeps legacy kernels legal under strict early-abort checks while preserving
the recommended path of using `common/pto_tileop.hpp` typed descriptors for
exact layout/stride control.

## PR6 Dual Profile and Host Sim

`common/pto_tileop.hpp` supports two execution profiles:

- full profile (default): compile/objdump flow for target bring-up.
- smoke profile (`-DPTO_QEMU_SMOKE=1`): reduced runtime tensor sizes for QEMU parity.

Host simulation mode is enabled with `-DPTO_HOST_SIM=1`:

- same PTO kernel sources compile and run on host CPU for oracle generation.
- strict size-code and tile-byte checks remain active (`size_code` in `5..8`, tile cap `<=4KB`).
- unsupported strict-v0.3 tile forms are treated as hard failures in backend/compiler paths.
