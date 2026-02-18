# Phase 7: Toolchain/glibc Bring-up

Canonical source repository:

- `lib/glibc` (`git@github.com:LinxISA/glibc.git`)

## Objective

Track and validate Linx glibc bring-up for `linx64-unknown-linux-gnu` in the forked glibc repository.

## Role in the bring-up sequence

- This phase closes Linux userspace toolchain blockers after compiler/emulator/kernel basics.
- It is support work for phases 4-6 and does not replace RTL/Linux validation gates.

## Workflow

From the `lib/glibc` submodule:

```bash
cd lib/glibc
bash tools/linx/build_linx64_glibc.sh
bash tools/linx/build_linx64_glibc_g1b.sh
```

Artifacts and logs:

- Default logs: `out/libc/glibc/logs/02-configure.log`, `out/libc/glibc/logs/03-make.log`, `out/libc/glibc/logs/summary.txt`.
- `G1b` summary: `out/libc/glibc/logs/g1b-summary.txt` (explicit `status` + `classification`).
- `G1a` gate proves `configure` + `csu/subdir_lib` and produces startup objects (`crt*.o`).
- `G1b` tracks shared `libc.so` build status and blocker signature if blocked.

## Current gates

- `G1a`: configure + `csu/subdir_lib` + startup object production (`crt1.o`).
- `G1b`: full shared `libc.so` build proof.

## Exit criteria

- `G1a` passes on the reference bring-up host/toolchain.
- `G1b` is measured by `build_linx64_glibc_g1b.sh` and status is tracked in `docs/bringup/libc_status.md`.
- Toolchain/libc no longer blocks Linux shell/userland gates.
- Remaining issues are tracked explicitly in `docs/bringup/libc_status.md`.
