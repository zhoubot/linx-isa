# Trace Schema Contract

Version: `1.0` (release-strict baseline)

All differential validation paths must emit a common architectural trace schema.

## Version compatibility policy

- Schema version format is `MAJOR.MINOR`.
- `MAJOR` mismatch is not allowed and must fail-fast.
- `MINOR` is forward-compatible within the same `MAJOR`:
  - consumer `X.Y` accepts producer `X.Z` when `Z >= Y`;
  - producer `X.Z` with `Z < Y` must be rejected.
- Producers may emit an explicit per-row `schema_version`; if omitted, gate tooling
  must use profile default (`1.0` for strict v0.3).

## Mandatory fields per commit/event

Scalar/base required fields:

- `cycle`
- `pc`
- `insn`
- `wb_valid`
- `wb_rd`
- `wb_data`
- `mem_valid`
- `mem_addr`
- `mem_wdata`
- `mem_rdata`
- `mem_size`
- `trap_valid`
- `trap_cause`
- `next_pc`

Extended vector/tile fields (when vector/tile subsets are under test):

- `block_kind` (`scalar|vpar|vseq|tma|cube|tepl|call|ret|sys`)
- `lane_id` (for lane-scoped vector commits)
- `tile_meta` (descriptor summary for tile issue/commit)
- `tile_ref_src` / `tile_ref_dst` (relative reference tokens)

Validation policy:

- `block_kind in {vpar,vseq}` requires vector fields (`lane_id`).
- `block_kind in {tma,cube,tepl}` requires tile fields (`tile_meta`, tile refs).
- Commit ordering must be non-decreasing in `cycle` within one trace stream.

## Producers required to conform

- QEMU reference execution
- pyCircuit C++ cycle model
- RTL simulation (Icarus/Verilator/VCS)
- FPGA reduced trace logger

## Comparison rules

- Compare traces in commit order using identical program image and boot PC.
- First mismatch is the triage anchor; do not skip ahead.
- If a field is unsupported in a path, mark it explicitly and treat as out-of-scope for that gate.
- Trace validators must run before semantic diff to catch schema violations early.

## Gate requirement

No gate can be marked `Passed` if unresolved schema-level divergence remains within the declared instruction subset.
