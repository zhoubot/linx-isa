#!/usr/bin/env python3
"""
Validate commit-trace schema compatibility and required fields.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

DEFAULT_REQUIRED_FIELDS = [
    "cycle",
    "pc",
    "insn",
    "wb_valid",
    "wb_rd",
    "wb_data",
    "mem_valid",
    "mem_addr",
    "mem_wdata",
    "mem_rdata",
    "mem_size",
    "trap_valid",
    "trap_cause",
    "next_pc",
]

VECTOR_BLOCK_KINDS = {"vpar", "vseq"}
TILE_BLOCK_KINDS = {"tma", "cube", "tepl"}
VECTOR_REQUIRED_FIELDS = ["block_kind", "lane_id"]
TILE_REQUIRED_FIELDS = ["block_kind", "tile_meta", "tile_ref_src", "tile_ref_dst"]


def _parse_version(ver: str, *, field: str) -> tuple[int, int]:
    m = re.fullmatch(r"\s*(\d+)\.(\d+)\s*", ver)
    if not m:
        raise SystemExit(f"error: invalid version format for {field}: {ver!r} (expected MAJOR.MINOR)")
    return int(m.group(1)), int(m.group(2))


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for ln, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1):
        line = raw.strip()
        if not line:
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError as exc:
            raise SystemExit(f"error: {path}:{ln}: invalid JSON: {exc}") from exc
        if not isinstance(obj, dict):
            raise SystemExit(f"error: {path}:{ln}: expected object per JSONL line")
        rows.append(obj)
    return rows


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Validate Linx commit-trace schema compatibility")
    ap.add_argument("--trace", required=True, help="Trace JSONL file")
    ap.add_argument("--expected-version", default="1.0", help="Consumer schema version (MAJOR.MINOR)")
    ap.add_argument(
        "--assume-trace-version",
        default="1.0",
        help="Used when rows do not contain `schema_version`",
    )
    ap.add_argument("--min-rows", type=int, default=1)
    ap.add_argument(
        "--required-field",
        action="append",
        default=[],
        help="Additional required field (repeatable)",
    )
    ap.add_argument(
        "--require-vector-fields",
        action="store_true",
        help="Require vector commit fields even when vector block kinds are absent",
    )
    ap.add_argument(
        "--require-tile-fields",
        action="store_true",
        help="Require tile commit fields even when tile block kinds are absent",
    )
    ap.add_argument(
        "--check-ordering",
        action="store_true",
        help="Fail when cycle values decrease (basic ordering check)",
    )
    args = ap.parse_args(argv)

    trace_path = Path(args.trace)
    rows = _load_jsonl(trace_path)
    if len(rows) < args.min_rows:
        raise SystemExit(
            f"error: trace too short ({len(rows)} rows, required >= {args.min_rows}) at {trace_path}"
        )

    expected_major, expected_minor = _parse_version(args.expected_version, field="--expected-version")
    schema_raw = rows[0].get("schema_version", args.assume_trace_version)
    actual_major, actual_minor = _parse_version(str(schema_raw), field="trace schema_version")
    if actual_major != expected_major:
        raise SystemExit(
            "error: trace schema major mismatch "
            f"(trace={actual_major}.{actual_minor}, expected={expected_major}.{expected_minor})"
        )
    if actual_minor < expected_minor:
        raise SystemExit(
            "error: trace schema minor too old "
            f"(trace={actual_major}.{actual_minor}, expected at least {expected_major}.{expected_minor})"
        )

    required = list(DEFAULT_REQUIRED_FIELDS)
    for extra in args.required_field:
        norm = str(extra).strip()
        if norm and norm not in required:
            required.append(norm)

    for idx, row in enumerate(rows):
        for field in required:
            if field not in row:
                raise SystemExit(f"error: {trace_path}: row {idx} missing required field {field!r}")

    has_vector = False
    has_tile = False
    prev_cycle: int | None = None
    for idx, row in enumerate(rows):
        block_kind = str(row.get("block_kind", "")).strip().lower()
        if block_kind in VECTOR_BLOCK_KINDS:
            has_vector = True
            for field in VECTOR_REQUIRED_FIELDS:
                if field not in row:
                    raise SystemExit(
                        f"error: {trace_path}: row {idx} (block_kind={block_kind}) missing required vector field {field!r}"
                    )
        if block_kind in TILE_BLOCK_KINDS:
            has_tile = True
            for field in TILE_REQUIRED_FIELDS:
                if field not in row:
                    raise SystemExit(
                        f"error: {trace_path}: row {idx} (block_kind={block_kind}) missing required tile field {field!r}"
                    )
        if args.check_ordering:
            try:
                cycle = int(row["cycle"])
            except Exception as exc:
                raise SystemExit(
                    f"error: {trace_path}: row {idx} has non-integer cycle value {row.get('cycle')!r}"
                ) from exc
            if prev_cycle is not None and cycle < prev_cycle:
                raise SystemExit(
                    f"error: {trace_path}: row {idx} cycle regressed ({cycle} < {prev_cycle})"
                )
            prev_cycle = cycle

    if args.require_vector_fields and not has_vector:
        raise SystemExit(
            f"error: {trace_path}: vector fields required but no vector block kinds observed ({sorted(VECTOR_BLOCK_KINDS)})"
        )
    if args.require_tile_fields and not has_tile:
        raise SystemExit(
            f"error: {trace_path}: tile fields required but no tile block kinds observed ({sorted(TILE_BLOCK_KINDS)})"
        )

    print(
        "ok: trace schema compatible "
        f"(rows={len(rows)}, trace={actual_major}.{actual_minor}, expected={expected_major}.{expected_minor})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
