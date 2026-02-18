# Linx libc Bring-up Status

Canonical libc sources:

- `lib/glibc`
- `lib/musl`

## Repositories and pins

- `lib/glibc` @ `3cb936060f896ef88f888682a3159a73db1fe411`
- `lib/musl` @ `b761931e63b6cd6fdbbc7269d07ccd9e4fec20b2`

## Current policy

- Bring-up deltas live in fork history (`LinxISA/glibc`, `LinxISA/musl`).
- This repository provides orchestration, runtime smoke, and status tracking.
- Release-strict gating uses canonical artifacts from `docs/bringup/gates/latest.json`.

## Latest verified state (2026-02-17 12:02:37Z)

- glibc `G1a`: pass (`configure` + `csu/subdir_lib` + startup objects).
- glibc `G1b`: pass (shared `libc.so` gate) in canonical lane runs.
- musl `M1`: pass (`configure` accepts `linx64-unknown-linux-musl`).
- musl `M2`: pass in `phase-b` strict mode.
- musl `M3`: pass in `phase-b` strict mode (shared `lib/libc.so` built).
- musl runtime `R1`: pass (compile/link smoke for static+shared).
- musl runtime `R2`: pass (QEMU runtime smoke for static+shared).

## Evidence pointers

- Canonical gate artifact: `docs/bringup/gates/latest.json`
- Rendered gate table: `docs/bringup/GATE_STATUS.md`
- glibc logs:
  - `out/libc/glibc/logs/summary.txt`
  - `out/libc/glibc/logs/g1b-summary.txt`
- musl logs:
  - `out/libc/musl/logs/phase-b-summary.txt`
  - `avs/qemu/out/musl-smoke/summary_static.json`
  - `avs/qemu/out/musl-smoke/summary_shared.json`

## Notes

- Release-strict sign-off does not allow blocked waivers for required libc gates.
- Runtime numeric/benchmark parity remains outside libc bring-up scope.
