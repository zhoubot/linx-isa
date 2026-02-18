# LinxISA Maturity Plan (strict v0.3 track)

Last updated: 2026-02-17

## Baseline policy

- `isa/v0.3/linxisa-v0.3.json` is the default architectural contract.
- `v0.2` remains available only as explicit legacy compatibility.
- 26 checks in `docs/bringup/check26_contract.yaml` are mandatory and machine-gated.

## Levels

### Level 0 (achieved): Bring-up stable

Required evidence:

- `bash tools/regression/full_stack.sh` passes.
- `bash tools/regression/run.sh` passes.
- `python3 tools/bringup/check26_contract.py --root .` passes.
- `python3 tools/isa/check_no_legacy_v03.py --root . --extra-root ~/qemu --extra-root ~/linux --extra-root ~/llvm-project` passes.
- Linux userspace QEMU boot scripts pass (smoke/full/virtio).
- LLVM Linx MC+CodeGen test suites pass.
- pyCircuit/Janus cpp tests and QEMU-vs-pyCircuit trace diff pass.

### Level 1 (next): Differential architecture closure

Required:

- Expand QEMU-vs-model difftest coverage from scalar/basic tile to full v0.3 vector/tile scenarios.
- Add contract-tagged tests for every check26 clause lacking explicit directed test IDs.
- Pin trace-schema compatibility gates for both Linx and Janus paths.
- Enforce release-strict profile (no required-gate waivers).

### Level 2 (next): Linux/system robustness

Required:

- Promote current Linux arch/linx checks from boot smoke to sustained userspace workloads.
- Add focused selftests for restartable tile-page-fault and bridged vector-memory behavior.
- Keep trap ABI baseline compatibility while extending v0.3-only behavior under explicit feature gates.

### Level 3 (next): Performance and packaging

Required:

- Stabilize benchmark methodology and archive reproducible reports under `workloads/generated/`.
- Add CI-like orchestration for full stack regressions across all 5 repos.
- Finalize skills docs as executable operational runbooks for v0.3.

## Immediate backlog

- Land a passing `pin` lane release-strict runtime convergence snapshot with `lane_policy=external+pin-required`.
- Bring pyCircuit toolchain (`pyc-compile`) online in the execution environment used by release-strict model gates.
- Resolve tile differential fixture runtime timeout (`MODEL-TILE-*` currently times out under QEMU in strict model suite).
- Promote vector-lane differential fixture from placeholder to directed, passing vector-body workload.
