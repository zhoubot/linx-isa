#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def _parse_mode_spec(spec: str) -> tuple[str, Path]:
    if "=" not in spec:
        raise SystemExit(f"error: mode spec must be <label>=<path>: {spec}")
    label, raw = spec.split("=", 1)
    label = label.strip()
    if not label:
        raise SystemExit(f"error: empty label in mode spec: {spec}")
    path = Path(raw.strip())
    if not path.exists():
        raise SystemExit(f"error: mode log not found for {label}: {path}")
    return label, path


def _read_kernel_list(path: Path | None) -> list[str] | None:
    if path is None:
        return None
    if not path.exists():
        raise SystemExit(f"error: kernel list not found: {path}")
    kernels: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        name = raw.strip()
        if not name or name.startswith("#"):
            continue
        kernels.append(name)
    if not kernels:
        raise SystemExit(f"error: empty kernel list: {path}")
    return kernels


def _parse_kernel_checksums(stdout_text: str, kernels: list[str] | None) -> dict[str, str]:
    wanted = set(kernels) if kernels else None
    parsed: dict[str, str] = {}
    for raw in stdout_text.splitlines():
        line = raw.strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) < 3:
            continue
        kernel = parts[0]
        checksum = parts[2]
        if kernel == "Loop":
            continue
        if wanted is not None and kernel not in wanted:
            continue
        if kernel not in parsed:
            parsed[kernel] = checksum
    return parsed


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Compare per-kernel TSVC checksums between modes.")
    ap.add_argument("--baseline", required=True, help="Baseline mode spec: <label>=<stdout_log>")
    ap.add_argument(
        "--candidate",
        action="append",
        default=[],
        help="Candidate mode spec: <label>=<stdout_log> (repeatable)",
    )
    ap.add_argument("--kernel-list", default=None, help="Optional kernel list path")
    ap.add_argument("--report", required=True, help="Markdown report output path")
    args = ap.parse_args(argv)

    if not args.candidate:
        raise SystemExit("error: at least one --candidate is required")

    kernel_list = _read_kernel_list(Path(args.kernel_list)) if args.kernel_list else None

    baseline_label, baseline_path = _parse_mode_spec(args.baseline)
    baseline_map = _parse_kernel_checksums(
        baseline_path.read_text(encoding="utf-8", errors="replace"),
        kernel_list,
    )
    if kernel_list is not None:
        missing_in_baseline = [k for k in kernel_list if k not in baseline_map]
        if missing_in_baseline:
            raise SystemExit(
                f"error: baseline log missing kernels ({len(missing_in_baseline)}): "
                f"{', '.join(missing_in_baseline[:8])}"
            )
        kernel_order = kernel_list
    else:
        kernel_order = sorted(baseline_map.keys())

    report_lines = [
        "# TSVC mode checksum comparison",
        "",
        f"- Baseline: `{baseline_label}` (`{baseline_path}`)",
        f"- Kernels compared: `{len(kernel_order)}`",
    ]

    all_ok = True
    for spec in args.candidate:
        label, path = _parse_mode_spec(spec)
        cand_map = _parse_kernel_checksums(path.read_text(encoding="utf-8", errors="replace"), kernel_order)

        missing = [k for k in kernel_order if k not in cand_map]
        mismatched = [k for k in kernel_order if k in cand_map and cand_map[k] != baseline_map.get(k)]
        ok = not missing and not mismatched
        if not ok:
            all_ok = False

        report_lines.append("")
        report_lines.append(f"## {label}")
        report_lines.append(f"- Log: `{path}`")
        report_lines.append(f"- Status: `{'PASS' if ok else 'FAIL'}`")
        report_lines.append(f"- Missing kernels: `{len(missing)}`")
        report_lines.append(f"- Checksum mismatches: `{len(mismatched)}`")
        if missing:
            report_lines.extend([f"- missing `{k}`" for k in missing[:64]])
        if mismatched:
            for k in mismatched[:64]:
                report_lines.append(
                    f"- mismatch `{k}`: baseline `{baseline_map.get(k, '<none>')}`, candidate `{cand_map.get(k, '<none>')}`"
                )
        if len(missing) > 64:
            report_lines.append(f"- ... ({len(missing) - 64} more missing)")
        if len(mismatched) > 64:
            report_lines.append(f"- ... ({len(mismatched) - 64} more mismatches)")

    report_path = Path(args.report)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(report_lines) + "\n", encoding="utf-8")

    if not all_ok:
        print(f"error: TSVC mode comparison failed (see {report_path})", file=sys.stderr)
        return 2
    print(f"ok: TSVC mode comparison passed -> {report_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
