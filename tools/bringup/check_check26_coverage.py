#!/usr/bin/env python3
"""
Validate check26-to-AVS directed coverage linkage.

Release-strict policy:
  - every AVS test entry must carry `domain`, `check26_ids`, `must_pass_in_profile`
  - every check26 ID (1..26) must be linked to at least one AVS test that is
    marked required in the selected profile and has status `pass`
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

ALLOWED_DOMAINS = {"Compiler", "Emulator", "ISA", "Kernel", "Library", "Regression", "Model"}
ALLOWED_VALIDATED = {"pass", "fail", "partial", "not_run"}


def _load_yaml_or_json(path: Path) -> dict[str, Any]:
    text = path.read_text(encoding="utf-8")
    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        try:
            import yaml  # type: ignore
        except Exception as exc:  # pragma: no cover - fatal path
            # Keep this checker self-contained on minimal hosts: parse the AVS
            # matrix through a simple YAML line parser when PyYAML is absent.
            if path.name == "linx_avs_v1_test_matrix.yaml":
                return _parse_matrix_yaml_without_pyyaml(text, path)
            raise SystemExit(
                f"error: failed to parse {path} as JSON and PyYAML unavailable: {exc}"
            ) from exc
        data = yaml.safe_load(text)
    if not isinstance(data, dict):
        raise SystemExit(f"error: expected mapping object in {path}")
    return data


def _parse_scalar(value: str) -> Any:
    v = value.strip()
    if not v:
        return ""
    if v.startswith('"') and v.endswith('"') and len(v) >= 2:
        return v[1:-1]
    if v.startswith("'") and v.endswith("'") and len(v) >= 2:
        return v[1:-1]
    if v.startswith("[") and v.endswith("]"):
        body = v[1:-1].strip()
        if not body:
            return []
        out: list[Any] = []
        for item in body.split(","):
            tok = item.strip().strip('"').strip("'")
            if re.fullmatch(r"-?\d+", tok):
                out.append(int(tok))
            elif tok:
                out.append(tok)
        return out
    if re.fullmatch(r"-?\d+", v):
        return int(v)
    return v


def _parse_matrix_yaml_without_pyyaml(text: str, path: Path) -> dict[str, Any]:
    tests: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None
    in_tests = False
    for raw in text.splitlines():
        line = raw.rstrip()
        if not in_tests:
            if re.match(r"^\s*tests:\s*$", line):
                in_tests = True
            continue

        m_id = re.match(r"^\s*-\s*id:\s*([A-Za-z0-9._:-]+)\s*$", line)
        if m_id:
            if current is not None:
                tests.append(current)
            current = {"id": m_id.group(1)}
            continue

        if current is None:
            continue

        m_kv = re.match(r"^\s{4}([A-Za-z0-9_]+):\s*(.+?)\s*$", line)
        if not m_kv:
            continue
        key = m_kv.group(1)
        value = _parse_scalar(m_kv.group(2))
        current[key] = value

    if current is not None:
        tests.append(current)

    if not tests:
        raise SystemExit(f"error: failed to parse tests from {path} without PyYAML")
    return {"tests": tests}


def _load_status(path: Path) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    statuses = data.get("statuses")
    if not isinstance(statuses, dict):
        raise SystemExit(f"error: {path} missing object field 'statuses'")
    out: dict[str, dict[str, Any]] = {}
    for test_id, meta in statuses.items():
        if isinstance(meta, dict):
            out[str(test_id)] = meta
    return out


def _must_pass(test: dict[str, Any], profile: str) -> bool:
    profiles = test.get("must_pass_in_profile")
    return isinstance(profiles, list) and profile in {str(x) for x in profiles}


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Validate check26 directed AVS coverage")
    ap.add_argument("--matrix", default="avs/linx_avs_v1_test_matrix.yaml")
    ap.add_argument("--contract", default="docs/bringup/check26_contract.yaml")
    ap.add_argument("--status", default="avs/linx_avs_v1_test_matrix_status.json")
    ap.add_argument("--profile", default="release-strict", choices=["dev", "release-strict"])
    ap.add_argument("--report-out", default="", help="Optional JSON report output path")
    args = ap.parse_args(argv)

    matrix_path = Path(args.matrix)
    contract_path = Path(args.contract)
    status_path = Path(args.status)

    matrix = _load_yaml_or_json(matrix_path)
    contract = _load_yaml_or_json(contract_path)
    statuses = _load_status(status_path)

    tests = matrix.get("tests")
    if not isinstance(tests, list):
        raise SystemExit(f"error: {matrix_path} missing list field 'tests'")
    checks = contract.get("checks")
    if not isinstance(checks, list):
        raise SystemExit(f"error: {contract_path} missing list field 'checks'")

    check_ids = sorted(
        {int(chk.get("id")) for chk in checks if isinstance(chk, dict) and isinstance(chk.get("id"), int)}
    )
    if check_ids != list(range(1, 27)):
        raise SystemExit(f"error: contract check IDs not contiguous 1..26 (got {check_ids})")

    errors: list[str] = []
    coverage: dict[int, list[str]] = {cid: [] for cid in check_ids}
    required_tests: list[str] = []

    for i, test in enumerate(tests):
        if not isinstance(test, dict):
            errors.append(f"matrix.tests[{i}] must be a mapping")
            continue
        tid = str(test.get("id", "")).strip()
        if not tid:
            errors.append(f"matrix.tests[{i}] missing non-empty id")
            continue

        domain = str(test.get("domain", "")).strip()
        if domain not in ALLOWED_DOMAINS:
            errors.append(f"{tid}: invalid domain {domain!r}, expected one of {sorted(ALLOWED_DOMAINS)}")

        must_pass_profiles = test.get("must_pass_in_profile")
        if not isinstance(must_pass_profiles, list) or not must_pass_profiles:
            errors.append(f"{tid}: missing non-empty must_pass_in_profile list")
            must_pass_profiles = []

        check26_ids = test.get("check26_ids")
        if not isinstance(check26_ids, list):
            errors.append(f"{tid}: missing check26_ids list")
            check26_ids = []
        normalized_ids: list[int] = []
        for item in check26_ids:
            if not isinstance(item, int):
                errors.append(f"{tid}: check26_ids contains non-int value {item!r}")
                continue
            if item not in coverage:
                errors.append(f"{tid}: check26_ids contains unknown id {item}")
                continue
            normalized_ids.append(item)

        status_meta = statuses.get(tid)
        if status_meta is None:
            errors.append(f"{tid}: missing status entry in {status_path}")
            continue
        validated = str(status_meta.get("validated", "not_run"))
        if validated not in ALLOWED_VALIDATED:
            errors.append(f"{tid}: invalid status.validated={validated!r}")

        if _must_pass(test, args.profile):
            required_tests.append(tid)
            if validated != "pass":
                errors.append(
                    f"{tid}: required in profile {args.profile} but status is {validated!r} (expected 'pass')"
                )
            else:
                for cid in normalized_ids:
                    coverage[cid].append(tid)

    uncovered = [cid for cid, tids in coverage.items() if not tids]
    if uncovered:
        errors.append(
            "check26 coverage incomplete for profile "
            f"{args.profile}: missing IDs {uncovered}"
        )

    report = {
        "profile": args.profile,
        "required_tests": sorted(required_tests),
        "coverage": {str(cid): sorted(tids) for cid, tids in coverage.items()},
        "uncovered": uncovered,
        "ok": not errors,
    }

    if args.report_out:
        out_path = Path(args.report_out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if errors:
        for err in errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    print(
        "ok: check26 directed coverage complete "
        f"(profile={args.profile}, required_tests={len(required_tests)}, checks={len(check_ids)})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
