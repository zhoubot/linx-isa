#!/usr/bin/env python3
"""
Cross-check bring-up status consistency and freshness.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def _parse_utc(ts: str, *, field: str) -> datetime:
    try:
        return datetime.strptime(ts, "%Y-%m-%d %H:%M:%SZ").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        raise SystemExit(f"error: invalid UTC timestamp in {field}: {ts!r}") from exc


@dataclass(frozen=True)
class LaneSummary:
    lane: str
    run_id: str
    generated_at_utc: str
    required_gate_count: int
    required_gate_keys: frozenset[tuple[str, str]]
    profile: str
    lane_policy: str
    trace_schema_version: str


def _required_gates_pass(
    run: dict[str, Any], *, required_profile: str, required_lane_policy: str
) -> tuple[bool, int, frozenset[tuple[str, str]]]:
    profile = str(run.get("profile", "")).strip()
    if profile != required_profile:
        return False, 0, frozenset()
    lane_policy = str(run.get("lane_policy", "")).strip()
    if required_lane_policy and lane_policy != required_lane_policy:
        return False, 0, frozenset()
    gates = run.get("gates")
    if not isinstance(gates, list):
        return False, 0, frozenset()
    required_count = 0
    keys: set[tuple[str, str]] = set()
    for gate in gates:
        if not isinstance(gate, dict):
            continue
        required = bool(gate.get("required", True))
        waived = bool(gate.get("waived", False))
        status = str(gate.get("status", "not_run"))
        if not required or waived:
            continue
        required_count += 1
        domain = str(gate.get("domain", "")).strip()
        gate_name = str(gate.get("gate", "")).strip()
        keys.add((domain, gate_name))
        if status != "pass":
            return False, required_count, frozenset()
    return required_count > 0, required_count, frozenset(keys)


def _best_lane_runs(
    report: dict[str, Any], *, required_profile: str, required_lane_policy: str
) -> dict[str, LaneSummary]:
    runs = report.get("runs")
    if not isinstance(runs, list):
        raise SystemExit("error: report.runs must be a list")
    best: dict[str, tuple[datetime, LaneSummary]] = {}
    for run in runs:
        if not isinstance(run, dict):
            continue
        lane = str(run.get("lane", "")).strip()
        if lane not in {"pin", "external"}:
            continue
        ok, req_count, gate_keys = _required_gates_pass(
            run, required_profile=required_profile, required_lane_policy=required_lane_policy
        )
        if not ok:
            continue
        ts_raw = str(run.get("generated_at_utc", "")).strip()
        if not ts_raw:
            continue
        ts = _parse_utc(ts_raw, field=f"run[{lane}].generated_at_utc")
        summary = LaneSummary(
            lane=lane,
            run_id=str(run.get("run_id", "unknown")),
            generated_at_utc=ts_raw,
            required_gate_count=req_count,
            required_gate_keys=gate_keys,
            profile=str(run.get("profile", "")),
            lane_policy=str(run.get("lane_policy", "")),
            trace_schema_version=str(run.get("trace_schema_version", "")),
        )
        prev = best.get(lane)
        if prev is None or ts > prev[0]:
            best[lane] = (ts, summary)
    return {lane: item[1] for lane, item in best.items()}


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Validate bring-up gate/doc consistency")
    ap.add_argument("--report", default="docs/bringup/gates/latest.json")
    ap.add_argument("--progress", default="docs/bringup/PROGRESS.md")
    ap.add_argument("--gate-status", default="docs/bringup/GATE_STATUS.md")
    ap.add_argument("--libc-status", default="docs/bringup/libc_status.md")
    ap.add_argument("--max-age-hours", type=float, default=24.0)
    ap.add_argument("--profile", default="release-strict")
    ap.add_argument("--lane-policy", default="external+pin-required")
    ap.add_argument("--trace-schema-version", default="1.0")
    args = ap.parse_args(argv)

    report_path = Path(args.report)
    progress_path = Path(args.progress)
    gate_status_path = Path(args.gate_status)
    libc_status_path = Path(args.libc_status)

    report = json.loads(report_path.read_text(encoding="utf-8"))
    schema_version = report.get("schema_version")
    if schema_version != 2:
        raise SystemExit(
            f"error: {report_path} schema_version must be 2 for release-strict checks (got {schema_version!r})"
        )

    now = datetime.now(timezone.utc)
    report_ts_raw = str(report.get("generated_at_utc", "")).strip()
    if not report_ts_raw:
        raise SystemExit(f"error: {report_path} missing generated_at_utc")
    report_ts = _parse_utc(report_ts_raw, field="report.generated_at_utc")
    age_hours = (now - report_ts).total_seconds() / 3600.0
    if age_hours > args.max_age_hours:
        raise SystemExit(
            f"error: {report_path} is stale ({age_hours:.2f}h old, max {args.max_age_hours:.2f}h)"
        )

    lane_runs = _best_lane_runs(
        report, required_profile=args.profile, required_lane_policy=args.lane_policy
    )
    missing_lanes = sorted({"external", "pin"} - set(lane_runs.keys()))
    if missing_lanes:
        raise SystemExit(
            "error: missing required fully-passing lanes in report "
            f"(profile={args.profile}, lane_policy={args.lane_policy}): " + ", ".join(missing_lanes)
        )

    expected_version = str(args.trace_schema_version).strip()
    m = re.fullmatch(r"(\d+)\.(\d+)", expected_version)
    if not m:
        raise SystemExit(f"error: invalid --trace-schema-version {expected_version!r} (expected MAJOR.MINOR)")
    expected_major = int(m.group(1))
    expected_minor = int(m.group(2))
    for lane in ("external", "pin"):
        trace_ver = lane_runs[lane].trace_schema_version
        m_lane = re.fullmatch(r"(\d+)\.(\d+)", trace_ver)
        if not m_lane:
            raise SystemExit(
                f"error: lane {lane} run {lane_runs[lane].run_id} has invalid trace schema version {trace_ver!r}"
            )
        major = int(m_lane.group(1))
        minor = int(m_lane.group(2))
        if major != expected_major or minor < expected_minor:
            raise SystemExit(
                "error: lane "
                f"{lane} run {lane_runs[lane].run_id} trace schema {trace_ver} "
                f"incompatible with required {expected_version}"
            )

    ext_keys = lane_runs["external"].required_gate_keys
    pin_keys = lane_runs["pin"].required_gate_keys
    if ext_keys != pin_keys:
        only_ext = sorted(ext_keys - pin_keys)
        only_pin = sorted(pin_keys - ext_keys)
        raise SystemExit(
            "error: required gate set mismatch between lanes "
            f"(only_external={only_ext}, only_pin={only_pin})"
        )

    gate_status_text = gate_status_path.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"Last generated \(UTC\): `([^`]+)`", gate_status_text)
    if not m:
        raise SystemExit(f"error: {gate_status_path} missing last-generated header")
    gate_status_ts = m.group(1).strip()
    if gate_status_ts != report_ts_raw:
        raise SystemExit(
            f"error: {gate_status_path} timestamp {gate_status_ts!r} != report {report_ts_raw!r}"
        )

    progress_text = progress_path.read_text(encoding="utf-8", errors="replace")
    if "⚠ Partial" in progress_text or re.search(r"\bpending\b", progress_text, flags=re.I):
        raise SystemExit(f"error: {progress_path} still contains partial/pending phase markers")

    libc_text = libc_status_path.read_text(encoding="utf-8", errors="replace")
    if not re.search(r"glibc `G1b`:\s+pass", libc_text, flags=re.I):
        raise SystemExit(f"error: {libc_status_path} missing strict pass line for glibc G1b")
    if not re.search(r"musl runtime `R2`:\s+pass", libc_text, flags=re.I):
        raise SystemExit(f"error: {libc_status_path} missing strict pass line for musl runtime R2")
    if re.search(r"musl runtime `R2[^`]*`:\s+fail", libc_text, flags=re.I):
        raise SystemExit(f"error: {libc_status_path} still reports failing musl R2 status")

    print(
        "ok: gate consistency/freshness passed "
        f"(report_age={age_hours:.2f}h, profile={args.profile}, "
        f"external={lane_runs['external'].run_id}, pin={lane_runs['pin'].run_id})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
