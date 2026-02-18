#!/usr/bin/env python3
"""
Validate AVS matrix ID alignment with machine-readable implementation status.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


def _ids_from_matrix(matrix_path: Path) -> list[str]:
    text = matrix_path.read_text(encoding="utf-8", errors="replace")
    return re.findall(r"^\s*-\s*id:\s*(AVS-[A-Z]+-\d+)\s*$", text, flags=re.M)


def _ids_from_status(status_path: Path) -> list[str]:
    data = json.loads(status_path.read_text(encoding="utf-8"))
    statuses = data.get("statuses")
    if not isinstance(statuses, dict):
        raise SystemExit(f"error: {status_path} missing object field 'statuses'")
    return sorted(statuses.keys())


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Check AVS matrix/status ID alignment")
    ap.add_argument("--matrix", default="avs/linx_avs_v1_test_matrix.yaml")
    ap.add_argument("--status", default="avs/linx_avs_v1_test_matrix_status.json")
    args = ap.parse_args(argv)

    matrix_path = Path(args.matrix)
    status_path = Path(args.status)
    matrix_ids = sorted(_ids_from_matrix(matrix_path))
    status_ids = _ids_from_status(status_path)

    missing = [i for i in matrix_ids if i not in status_ids]
    extra = [i for i in status_ids if i not in matrix_ids]

    if missing:
        print("error: missing status entries for matrix IDs:", file=sys.stderr)
        for item in missing:
            print(f"  - {item}", file=sys.stderr)
    if extra:
        print("error: status file has IDs absent from matrix:", file=sys.stderr)
        for item in extra:
            print(f"  - {item}", file=sys.stderr)

    if missing or extra:
        return 1

    print(f"ok: AVS matrix/status aligned ({len(matrix_ids)} IDs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
