#!/usr/bin/env python3
"""
Canonical Linx bring-up gate report utility.

This tool keeps one machine-readable JSON artifact as source-of-truth and
renders docs/bringup/GATE_STATUS.md from that artifact.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 2
LEGACY_SCHEMA_VERSION = "linx-gate-report-v1"
DEFAULT_REPORT = Path("docs/bringup/gates/latest.json")
DEFAULT_MARKDOWN = Path("docs/bringup/GATE_STATUS.md")

VALID_STATUSES = {"pass", "fail", "partial", "not_run"}
VALID_PROFILES = {"dev", "release-strict"}


def _utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%d %H:%M:%SZ")


def _run_git_rev(path: Path) -> str | None:
    if not path.exists():
        return None
    try:
        out = subprocess.check_output(
            ["git", "-C", str(path), "rev-parse", "HEAD"],
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except subprocess.CalledProcessError:
        return None
    return out.strip() or None


def _parse_bool_flag(value: str) -> bool:
    norm = value.strip().lower()
    if norm in {"1", "true", "yes", "y", "on"}:
        return True
    if norm in {"0", "false", "no", "n", "off"}:
        return False
    raise SystemExit(f"error: invalid boolean value {value!r} (expected yes/no)")


def _infer_evidence_type(evidence: list[str]) -> str:
    if not evidence:
        return "log"
    first = evidence[0]
    if first.startswith("log:"):
        return "log"
    if first.startswith("summary:"):
        return "summary"
    if first.startswith("terminal:"):
        return "terminal"
    if first.startswith("note:"):
        return "note"
    return "mixed"


def _normalize_gate(gate: dict[str, Any]) -> dict[str, Any]:
    evidence = gate.get("evidence")
    if not isinstance(evidence, list):
        evidence = [str(evidence)] if evidence else []
    gate["evidence"] = [str(item) for item in evidence]
    gate.setdefault("required", True)
    gate["required"] = bool(gate["required"])
    gate.setdefault("waived", False)
    gate["waived"] = bool(gate["waived"])
    owner = str(gate.get("owner", "unowned")).strip() or "unowned"
    gate["owner"] = owner
    evidence_type = str(gate.get("evidence_type", "")).strip()
    gate["evidence_type"] = evidence_type if evidence_type else _infer_evidence_type(gate["evidence"])
    status = str(gate.get("status", "not_run")).strip()
    if status not in VALID_STATUSES:
        raise SystemExit(f"error: invalid gate status {status!r}")
    gate["status"] = status
    return gate


def _normalize_run(run: dict[str, Any]) -> dict[str, Any]:
    profile = str(run.get("profile", "dev")).strip()
    if profile not in VALID_PROFILES:
        raise SystemExit(f"error: invalid run profile {profile!r}")
    run["profile"] = profile
    lane_policy = str(run.get("lane_policy", "default")).strip() or "default"
    run["lane_policy"] = lane_policy
    trace_schema_version = str(run.get("trace_schema_version", "1.0")).strip() or "1.0"
    run["trace_schema_version"] = trace_schema_version

    gates = run.get("gates")
    if not isinstance(gates, list):
        gates = []
    run["gates"] = [_normalize_gate(g) for g in gates if isinstance(g, dict)]
    return run


def _migrate_legacy_report(data: dict[str, Any]) -> dict[str, Any]:
    report = dict(data)
    report["schema_version"] = SCHEMA_VERSION
    runs = report.get("runs")
    if not isinstance(runs, list):
        runs = []
    out_runs: list[dict[str, Any]] = []
    for run in runs:
        if not isinstance(run, dict):
            continue
        run = dict(run)
        run.setdefault("profile", "dev")
        run.setdefault("lane_policy", "legacy")
        run.setdefault("trace_schema_version", "1.0")
        gates = run.get("gates")
        if not isinstance(gates, list):
            gates = []
        migrated_gates: list[dict[str, Any]] = []
        for gate in gates:
            if not isinstance(gate, dict):
                continue
            g = dict(gate)
            g.setdefault("required", True)
            g.setdefault("waived", False)
            g.setdefault("owner", "unowned")
            evidence = g.get("evidence")
            if not isinstance(evidence, list):
                evidence = [str(evidence)] if evidence else []
            g["evidence"] = [str(item) for item in evidence]
            g.setdefault("evidence_type", _infer_evidence_type(g["evidence"]))
            migrated_gates.append(g)
        run["gates"] = migrated_gates
        out_runs.append(run)
    report["runs"] = out_runs
    return report


def _load_report(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {
            "schema_version": SCHEMA_VERSION,
            "generated_at_utc": _utc_now(),
            "runs": [],
        }
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise SystemExit(f"error: report must be a JSON object: {path}")
    schema_version = data.get("schema_version")
    if schema_version == LEGACY_SCHEMA_VERSION:
        data = _migrate_legacy_report(data)
    elif schema_version != SCHEMA_VERSION:
        raise SystemExit(
            f"error: unsupported schema_version {schema_version!r}, expected {SCHEMA_VERSION!r}"
        )
    if not isinstance(data.get("runs"), list):
        raise SystemExit("error: report.runs must be a list")
    runs = []
    for run in data.get("runs", []):
        if not isinstance(run, dict):
            continue
        runs.append(_normalize_run(run))
    data["runs"] = runs
    return data


def _save_report(path: Path, report: dict[str, Any]) -> None:
    report["schema_version"] = SCHEMA_VERSION
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _pin_paths(root: Path) -> dict[str, Path]:
    return {
        "linx-isa": root,
        "llvm": root / "compiler" / "llvm",
        "qemu": root / "emulator" / "qemu",
        "linux": root / "kernel" / "linux",
        "linxcore": root / "rtl" / "LinxCore",
        "pycircuit": root / "tools" / "pyCircuit",
        "glibc": root / "lib" / "glibc",
        "musl": root / "lib" / "musl",
    }


def _external_paths(external_root: Path) -> dict[str, Path]:
    return {
        "llvm": external_root / "llvm-project",
        "qemu": external_root / "qemu",
        "linux": external_root / "linux",
        "linxcore": external_root / "LinxCore",
        "pycircuit": external_root / "pyCircuit",
        "glibc": external_root / "glibc",
        "musl": external_root / "musl",
    }


def _collect_sha_manifest(lane: str, root: Path, external_root: Path) -> dict[str, Any]:
    if lane == "pin":
        repos = _pin_paths(root)
    elif lane == "external":
        repos = _external_paths(external_root)
    else:
        raise SystemExit(f"error: unsupported lane: {lane}")

    out: dict[str, Any] = {}
    for repo, path in repos.items():
        sha = _run_git_rev(path)
        out[repo] = {
            "path": str(path),
            "sha": sha if sha is not None else "missing",
        }
    return out


def _find_run(report: dict[str, Any], lane: str, run_id: str) -> dict[str, Any]:
    runs = report["runs"]
    for run in runs:
        if run.get("lane") == lane and run.get("run_id") == run_id:
            return run
    run = {
        "run_id": run_id,
        "lane": lane,
        "generated_at_utc": _utc_now(),
        "profile": "dev",
        "lane_policy": "default",
        "trace_schema_version": "1.0",
        "sha_manifest": {},
        "gates": [],
    }
    runs.append(run)
    return run


def _run_sort_key(run: dict[str, Any]) -> tuple[str, str]:
    return (str(run.get("lane", "")), str(run.get("run_id", "")))


def _gate_sort_key(gate: dict[str, Any]) -> tuple[str, str]:
    return (str(gate.get("domain", "")), str(gate.get("gate", "")))


def _find_gate_index(gates: list[dict[str, Any]], domain: str, gate_name: str) -> int | None:
    for i, gate in enumerate(gates):
        if str(gate.get("domain", "")) == domain and str(gate.get("gate", "")) == gate_name:
            return i
    return None


def _split_csv_items(items: list[str] | None) -> list[str]:
    if not items:
        return []
    out: list[str] = []
    for item in items:
        for piece in item.split(","):
            norm = piece.strip()
            if norm:
                out.append(norm)
    return out


def cmd_capture_sha(args: argparse.Namespace) -> int:
    report_path = Path(args.report).resolve()
    root = Path(args.root).resolve()
    external_root = Path(args.external_root).resolve()

    report = _load_report(report_path)
    run = _find_run(report, args.lane, args.run_id)
    run["generated_at_utc"] = _utc_now()
    run["profile"] = args.profile
    run["lane_policy"] = args.lane_policy
    run["trace_schema_version"] = args.trace_schema_version
    run["sha_manifest"] = _collect_sha_manifest(args.lane, root, external_root)
    report["generated_at_utc"] = _utc_now()
    _save_report(report_path, report)
    print(f"ok: updated SHA manifest for lane={args.lane} run_id={args.run_id} ({report_path})")
    return 0


def cmd_reset_run(args: argparse.Namespace) -> int:
    report_path = Path(args.report).resolve()
    report = _load_report(report_path)
    run = _find_run(report, args.lane, args.run_id)
    run["generated_at_utc"] = _utc_now()
    run["gates"] = []
    if args.drop_sha:
        run["sha_manifest"] = {}
    report["runs"] = sorted(report.get("runs", []), key=_run_sort_key)
    report["generated_at_utc"] = _utc_now()
    _save_report(report_path, report)
    print(f"ok: reset gates for lane={args.lane} run_id={args.run_id} ({report_path})")
    return 0


def cmd_upsert_gate(args: argparse.Namespace) -> int:
    report_path = Path(args.report).resolve()
    report = _load_report(report_path)
    run = _find_run(report, args.lane, args.run_id)
    gates = run.get("gates")
    if not isinstance(gates, list):
        gates = []
        run["gates"] = gates

    status = str(args.status)
    gate_entry: dict[str, Any] = {
        "domain": args.domain,
        "gate": args.gate,
        "command": args.command,
        "status": status,
        "classification": args.classification,
        "evidence": _split_csv_items(args.evidence),
        "required": _parse_bool_flag(args.required),
        "waived": bool(args.waived),
        "owner": args.owner,
        "evidence_type": args.evidence_type,
    }
    gate_entry = _normalize_gate(gate_entry)
    idx = _find_gate_index(gates, args.domain, args.gate)
    if idx is None:
        gates.append(gate_entry)
    else:
        gates[idx] = gate_entry

    gates.sort(key=_gate_sort_key)
    run["generated_at_utc"] = _utc_now()
    report["runs"] = sorted(report.get("runs", []), key=_run_sort_key)
    report["generated_at_utc"] = _utc_now()
    _save_report(report_path, report)
    print(
        "ok: upserted gate "
        f"lane={args.lane} run_id={args.run_id} domain={args.domain} gate={args.gate} status={status}"
    )
    return 0


def _status_cell(gate: dict[str, Any]) -> str:
    status = str(gate.get("status", "unknown"))
    icon = {
        "pass": "✅",
        "fail": "❌",
        "partial": "⚠",
        "not_run": "⏸",
    }.get(status, "❔")
    cls = str(gate.get("classification", "")).strip()
    waived = bool(gate.get("waived", False))
    waiver_suffix = " (waived)" if waived else ""
    if cls:
        return f"{icon} {status}{waiver_suffix} (`{cls}`)"
    return f"{icon} {status}{waiver_suffix}"


def _render_markdown(report: dict[str, Any]) -> str:
    lines: list[str] = []
    lines.append("# Bring-up Gate Status (Canonical)")
    lines.append("")
    lines.append(
        "This file is generated from `docs/bringup/gates/latest.json` via "
        "`python3 tools/bringup/gate_report.py render`."
    )
    lines.append("")
    lines.append(f"Last generated (UTC): `{report.get('generated_at_utc', 'unknown')}`")
    lines.append("")

    runs: list[dict[str, Any]] = list(report.get("runs", []))
    runs.sort(key=lambda r: (str(r.get("lane", "")), str(r.get("run_id", ""))))

    for run in runs:
        lane = str(run.get("lane", "unknown"))
        run_id = str(run.get("run_id", "unknown"))
        run_ts = str(run.get("generated_at_utc", "unknown"))
        lines.append(f"## Lane `{lane}` (`{run_id}`)")
        lines.append("")
        lines.append(f"- Timestamp (UTC): `{run_ts}`")
        lines.append(f"- Profile: `{run.get('profile', 'dev')}`")
        lines.append(f"- Lane policy: `{run.get('lane_policy', 'default')}`")
        lines.append(f"- Trace schema version: `{run.get('trace_schema_version', '1.0')}`")
        lines.append("- SHA manifest:")
        manifest = run.get("sha_manifest", {})
        if isinstance(manifest, dict):
            for repo in sorted(manifest.keys()):
                meta = manifest.get(repo) or {}
                sha = str(meta.get("sha", "missing"))
                path = str(meta.get("path", ""))
                lines.append(f"  - `{repo}`: `{sha}` (`{path}`)")
        lines.append("")
        lines.append("| Domain | Gate | Required | Waived | Owner | Command | Result | Evidence |")
        lines.append("| --- | --- | --- | --- | --- | --- | --- | --- |")
        gates = list(run.get("gates", []))
        gates.sort(key=_gate_sort_key)
        for gate in gates:
            domain = str(gate.get("domain", ""))
            gate_name = str(gate.get("gate", ""))
            required = "yes" if bool(gate.get("required", True)) else "no"
            waived = "yes" if bool(gate.get("waived", False)) else "no"
            owner = str(gate.get("owner", "unowned"))
            command = str(gate.get("command", ""))
            evidence = gate.get("evidence", [])
            if not isinstance(evidence, list):
                evidence = [str(evidence)]
            evidence_cell = "; ".join(f"`{str(e)}`" for e in evidence)
            lines.append(
                f"| {domain} | {gate_name} | {required} | {waived} | `{owner}` | `{command}` | {_status_cell(gate)} | {evidence_cell} |"
            )
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def cmd_render(args: argparse.Namespace) -> int:
    report_path = Path(args.report).resolve()
    out_md = Path(args.out_md).resolve()
    report = _load_report(report_path)
    text = _render_markdown(report)
    out_md.write_text(text, encoding="utf-8")
    print(f"ok: wrote {out_md}")
    return 0


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Linx bring-up gate report utility")
    sub = ap.add_subparsers(dest="cmd", required=True)

    ap_capture = sub.add_parser("capture-sha", help="Update run timestamp + SHA manifest for one lane")
    ap_capture.add_argument("--report", default=str(DEFAULT_REPORT))
    ap_capture.add_argument("--root", default=".")
    ap_capture.add_argument("--external-root", default=str(Path.home()))
    ap_capture.add_argument("--lane", choices=["pin", "external"], required=True)
    ap_capture.add_argument("--run-id", required=True)
    ap_capture.add_argument("--profile", choices=["dev", "release-strict"], default="dev")
    ap_capture.add_argument("--lane-policy", default="default")
    ap_capture.add_argument("--trace-schema-version", default="1.0")
    ap_capture.set_defaults(func=cmd_capture_sha)

    ap_reset = sub.add_parser("reset-run", help="Reset one run's gate rows")
    ap_reset.add_argument("--report", default=str(DEFAULT_REPORT))
    ap_reset.add_argument("--lane", choices=["pin", "external"], required=True)
    ap_reset.add_argument("--run-id", required=True)
    ap_reset.add_argument(
        "--drop-sha",
        action="store_true",
        help="Also clear the run SHA manifest (default keeps last captured manifest).",
    )
    ap_reset.set_defaults(func=cmd_reset_run)

    ap_upsert = sub.add_parser("upsert-gate", help="Insert or update one gate row in one run")
    ap_upsert.add_argument("--report", default=str(DEFAULT_REPORT))
    ap_upsert.add_argument("--lane", choices=["pin", "external"], required=True)
    ap_upsert.add_argument("--run-id", required=True)
    ap_upsert.add_argument("--domain", required=True)
    ap_upsert.add_argument("--gate", required=True)
    ap_upsert.add_argument("--command", required=True)
    ap_upsert.add_argument("--status", choices=["pass", "fail", "partial", "not_run"], required=True)
    ap_upsert.add_argument("--classification", required=True)
    ap_upsert.add_argument("--required", default="yes", help="Whether this gate is required (yes/no)")
    ap_upsert.add_argument("--waived", action="store_true", help="Mark this gate as waived")
    ap_upsert.add_argument("--owner", default="unowned")
    ap_upsert.add_argument("--evidence-type", default="log")
    ap_upsert.add_argument(
        "--evidence",
        action="append",
        default=[],
        help="Evidence string(s); repeat or pass comma-separated values.",
    )
    ap_upsert.set_defaults(func=cmd_upsert_gate)

    ap_render = sub.add_parser("render", help="Render markdown status table from report JSON")
    ap_render.add_argument("--report", default=str(DEFAULT_REPORT))
    ap_render.add_argument("--out-md", default=str(DEFAULT_MARKDOWN))
    ap_render.set_defaults(func=cmd_render)

    args = ap.parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
