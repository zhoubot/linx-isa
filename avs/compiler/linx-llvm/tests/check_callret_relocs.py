#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ADDR_RE = re.compile(r"^\s*([0-9a-fA-F]+):")
RELOC_RE = re.compile(r"0x([0-9a-fA-F]+)\s+R_LINX_([A-Z0-9_]+)\b")
CALL_RE = re.compile(r"\b(?:HL\.)?BSTART(?:\.STD)?\s+CALL,")

CALL_RELOC_TYPES = {
    "R_LINX_B17_PCREL",
    "R_LINX_B17_PLT",
    "R_LINX_HL_BSTART30_PCREL",
}

SETRET_RELOC_TYPES = {
    "R_LINX_CSETRET5_PCREL",
    "R_LINX_SETRET20_PCREL",
    "R_LINX_HL_SETRET32_PCREL",
}


def parse_calls(text: str) -> list[tuple[int, bool, bool, str]]:
    calls: list[tuple[int, bool, bool, str]] = []
    for line in text.splitlines():
        m = ADDR_RE.match(line)
        if not m:
            continue
        if "CALL" not in line or "ICALL" in line or "BSTART" not in line:
            continue
        if not CALL_RE.search(line):
            continue
        off = int(m.group(1), 16)
        is_hl = "HL.BSTART" in line
        has_ra = "ra=" in line
        calls.append((off, is_hl, has_ra, line.strip()))
    return calls


def parse_relocs(text: str) -> dict[int, set[str]]:
    relocs: dict[int, set[str]] = {}
    for line in text.splitlines():
        m = RELOC_RE.search(line)
        if not m:
            continue
        off = int(m.group(1), 16)
        typ = "R_LINX_" + m.group(2)
        relocs.setdefault(off, set()).add(typ)
    return relocs


def has_type(relocs: dict[int, set[str]], off: int, allowed: set[str]) -> bool:
    types = relocs.get(off)
    if not types:
        return False
    return bool(types & allowed)


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        description="Verify Linx CALL headers in objects keep call/setret relocations."
    )
    ap.add_argument("--objdump", required=True, help="Path to llvm-objdump disassembly output")
    ap.add_argument("--relocs", required=True, help="Path to llvm-readobj -r output")
    ap.add_argument("--label", default="", help="Human-readable test label")
    ap.add_argument(
        "--strict-relocs",
        action="store_true",
        help=(
            "Require CALL and SETRET relocations on all CALL headers. "
            "Without this flag, local fused headers without relocations are accepted."
        ),
    )
    args = ap.parse_args(argv)

    objdump_path = Path(args.objdump)
    relocs_path = Path(args.relocs)

    if not objdump_path.exists():
        print(f"error: missing objdump file: {objdump_path}", file=sys.stderr)
        return 2
    if not relocs_path.exists():
        print(f"error: missing reloc file: {relocs_path}", file=sys.stderr)
        return 2

    calls = parse_calls(objdump_path.read_text(encoding="utf-8", errors="replace"))
    relocs = parse_relocs(relocs_path.read_text(encoding="utf-8", errors="replace"))

    missing_call: list[tuple[int, str]] = []
    missing_setret: list[tuple[int, int, str]] = []
    missing_ra: list[tuple[int, str]] = []

    for off, is_hl, has_ra, line in calls:
        has_call_reloc = has_type(relocs, off, CALL_RELOC_TYPES)
        if not has_ra:
            # Compatibility path for older Linx LLVM lowering:
            # local CALL headers may omit fused `ra=` and carry no relocations.
            if args.strict_relocs or has_call_reloc:
                missing_ra.append((off, line))
            continue
        if args.strict_relocs and not has_call_reloc:
            missing_call.append((off, line))
            continue
        if not has_call_reloc:
            # Local fused CALL headers may resolve both call/setret immediates
            # without relocations. Keep strict mode for toolchain auditing.
            continue
        setret_off = off + (6 if is_hl else 4)
        if not has_type(relocs, setret_off, SETRET_RELOC_TYPES):
            missing_setret.append((off, setret_off, line))

    if not missing_ra and not missing_call and not missing_setret:
        return 0

    label = f"[{args.label}] " if args.label else ""
    if missing_ra:
        print(f"error: {label}missing fused ra=... in CALL headers:", file=sys.stderr)
        for off, line in missing_ra[:20]:
            print(f"  0x{off:x}: {line}", file=sys.stderr)
        if len(missing_ra) > 20:
            print(f"  ... and {len(missing_ra) - 20} more", file=sys.stderr)
    if missing_call:
        print(f"error: {label}missing CALL relocations:", file=sys.stderr)
        for off, line in missing_call[:20]:
            print(f"  0x{off:x}: {line}", file=sys.stderr)
        if len(missing_call) > 20:
            print(f"  ... and {len(missing_call) - 20} more", file=sys.stderr)
    if missing_setret:
        print(f"error: {label}missing SETRET relocations for fused CALL headers:", file=sys.stderr)
        for call_off, setret_off, line in missing_setret[:20]:
            print(
                f"  call@0x{call_off:x} expected setret reloc at 0x{setret_off:x}: {line}",
                file=sys.stderr,
            )
        if len(missing_setret) > 20:
            print(f"  ... and {len(missing_setret) - 20} more", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
