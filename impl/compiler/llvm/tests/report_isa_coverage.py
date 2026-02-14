#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


def _extract_mnemonics_from_asm(path: Path) -> set[str]:
    mnems: set[str] = set()
    for line in path.read_text(errors="replace").splitlines():
        s = line.lstrip()
        if not s:
            continue
        if s.startswith((".", "#")):
            continue
        if s.endswith(":"):
            continue
        tok = s.split(maxsplit=1)[0]
        mnems.add(tok.upper())
    return mnems


def _extract_mnemonics_from_linxisamcinstlower(path: Path) -> set[str]:
    # Best-effort: cover both direct getSpecOpcode("...") calls and the common
    # `Mnem = "..."` / `RegMnem = "..."` assignments feeding getSpecOpcode(Mnem).
    text = path.read_text(errors="replace")
    mnems: set[str] = set()

    for m in re.finditer(r'getSpecOpcode\("([^"]+)"', text):
        mnems.add(m.group(1).upper())

    for m in re.finditer(r'\b(?:Mnem|RegMnem)\s*=\s*"([^"]+)"', text):
        mnems.add(m.group(1).upper())

    return mnems


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Report which LinxISA mnemonics appear in compiled test assembly."
    )
    ap.add_argument(
        "--out-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "out",
        help="Directory containing per-test output folders (default: impl/compiler/llvm/tests/out).",
    )
    ap.add_argument(
        "--spec",
        type=Path,
        default=Path(__file__).resolve().parents[4] / "spec/isa/spec/current/linxisa-v0.3.json",
        help="Path to linxisa-v0.3.json (default: spec/isa/spec/current/linxisa-v0.3.json).",
    )
    ap.add_argument(
        "--llvm-project",
        type=Path,
        default=Path(__file__).resolve().parents[5] / "llvm-project",
        help="Path to llvm-project checkout (default: ../../../../../llvm-project).",
    )
    ap.add_argument(
        "--list",
        action="store_true",
        help="List all mnemonics found in test output.",
    )
    args = ap.parse_args()

    if not args.out_dir.exists():
        print(f"error: out dir not found: {args.out_dir}", file=sys.stderr)
        print(
            "hint: run ./impl/compiler/llvm/tests/run.sh first (or set --out-dir)",
            file=sys.stderr,
        )
        return 2

    asm_files = sorted(args.out_dir.glob("**/*.s"))
    if not asm_files:
        print(f"error: no .s files found under: {args.out_dir}", file=sys.stderr)
        return 2

    emitted: set[str] = set()
    for p in asm_files:
        emitted |= _extract_mnemonics_from_asm(p)

    spec_data = json.loads(args.spec.read_text())
    spec_mnems = {str(i["mnemonic"]).upper() for i in spec_data["instructions"]}

    lower_path = (
        args.llvm_project / "llvm/lib/Target/LinxISA/LinxISAMCInstLower.cpp"
    )
    implemented: set[str] = set()
    if lower_path.exists():
        implemented = _extract_mnemonics_from_linxisamcinstlower(lower_path)

    print(f"Spec mnemonics:      {len(spec_mnems):4d}")
    print(f"Emitted mnemonics:   {len(emitted):4d}  (from {len(asm_files)} .s files)")

    if implemented:
        covered = emitted & implemented
        missing = sorted(implemented - emitted)
        print(f"Backend mnemonics:   {len(implemented):4d}  (from LinxISAMCInstLower.cpp)")
        print(f"Covered by tests:    {len(covered):4d}")
        if missing:
            print("Missing (backend-but-not-in-tests):")
            for m in missing:
                print(f"  {m}")

    if args.list:
        print("Emitted (sorted):")
        for m in sorted(emitted):
            print(f"  {m}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
