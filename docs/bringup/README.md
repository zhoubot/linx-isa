# LinxISA Bring-up (Public v0.3)

This directory tracks v0.3 architecture/implementation alignment and public bring-up checkpoints.

## Start Here

- Onboarding and workspace setup: `docs/bringup/GETTING_STARTED.md`

## Normative Contract

- Architecture contract: `docs/architecture/v0.3-architecture-contract.md`
- check26 contract file: `docs/bringup/check26_contract.yaml`
- contract gate: `python3 tools/bringup/check26_contract.py --root .`

## Key References

- `docs/bringup/CHECK26_CONTRACT.md`
- `docs/bringup/CPP_BRINGUP_CONTRACT.md`
- `docs/bringup/PROGRESS.md`
- `docs/bringup/gates/latest.json` (canonical machine-readable gate report)
- `docs/bringup/GATE_STATUS.md` (generated from gate report JSON)
- `docs/bringup/LINX_ASM_ABI_UNWIND_CONTEXT_CHECKLIST.md`
- `docs/bringup/CROSSSTACK_SKILLS_SUMMARY.md`
- `docs/reference/linxisa-call-ret-contract.md`
- `docs/bringup/phases/`
- `docs/bringup/contracts/`

Gate status markdown refresh command:

`python3 tools/bringup/gate_report.py render --report docs/bringup/gates/latest.json --out-md docs/bringup/GATE_STATUS.md`

Release-strict bring-up consistency checks:

- `python3 tools/bringup/check_check26_coverage.py --matrix avs/linx_avs_v1_test_matrix.yaml --contract docs/bringup/check26_contract.yaml --status avs/linx_avs_v1_test_matrix_status.json --profile release-strict`
- `python3 tools/bringup/run_model_diff_suite.py --root . --suite avs/model/linx_model_diff_suite.yaml --profile release-strict --trace-schema-version 1.0 --report-out docs/bringup/gates/model_diff_summary.json`
- `python3 tools/bringup/check_gate_consistency.py --report docs/bringup/gates/latest.json --progress docs/bringup/PROGRESS.md --gate-status docs/bringup/GATE_STATUS.md --libc-status docs/bringup/libc_status.md --profile release-strict --lane-policy external+pin-required --trace-schema-version 1.0 --max-age-hours 24`
