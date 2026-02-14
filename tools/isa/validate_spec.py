#!/usr/bin/env python3
"""
Validate basic invariants of the compiled ISA JSON spec.

This is intentionally lightweight and does not attempt to validate semantics.
It checks that the derived `encoding` view is internally consistent with the
raw `parts[].segments` view.
"""

from __future__ import annotations

import argparse
import json
import sys
from typing import Any, Dict, List, Tuple


def _parse_hex(s: str) -> int:
    s = s.strip().lower()
    if not s.startswith("0x"):
        raise ValueError(f"expected hex string, got {s!r}")
    return int(s, 16)


def _mask_for_width(width_bits: int) -> int:
    return (1 << width_bits) - 1 if width_bits > 0 else 0


def _pattern_to_mask_match(pattern: str) -> Tuple[int, int]:
    # pattern is MSB->LSB with '0','1','.'
    width_bits = len(pattern)
    mask = 0
    match = 0
    for i, ch in enumerate(pattern):
        bit = width_bits - 1 - i  # convert to bit index
        if ch == ".":
            continue
        if ch not in ("0", "1"):
            raise ValueError(f"invalid pattern char {ch!r}")
        mask |= 1 << bit
        if ch == "1":
            match |= 1 << bit
    return mask, match


def validate(path: str) -> List[str]:
    with open(path, "r", encoding="utf-8") as f:
        spec = json.load(f)

    errors: List[str] = []

    # ---------------------------------------------------------------------
    # v0.2 bring-up profile sanity checks (system/privileged contract)
    # ---------------------------------------------------------------------
    version = str(spec.get("version", "")).strip()
    if version == "0.2":
        state = spec.get("state", {})
        sysregs = state.get("system_registers")
        if not isinstance(sysregs, dict):
            errors.append("v0.2: missing state.system_registers (expected dict)")
        else:
            legacy = {"EBPC_ACRn", "ETPC_ACRn", "EBPCN_ACRn"}

            def _walk_names(obj: Any) -> List[str]:
                out: List[str] = []
                if isinstance(obj, dict):
                    n = obj.get("name")
                    if isinstance(n, str):
                        out.append(n)
                    fmt = obj.get("name_fmt")
                    if isinstance(fmt, str):
                        out.append(fmt)
                    for v in obj.values():
                        out.extend(_walk_names(v))
                elif isinstance(obj, list):
                    for it in obj:
                        out.extend(_walk_names(it))
                return out

            names = set(_walk_names(sysregs))
            for bad in sorted(legacy):
                if bad in names:
                    errors.append(f"v0.2: forbidden legacy SSR name present in system_registers: {bad}")

            ebarg = sysregs.get("ebarg_group") or {}
            if not isinstance(ebarg, dict):
                errors.append("v0.2: system_registers.ebarg_group missing/invalid")

    for inst in spec.get("instructions", []):
        inst_id = inst.get("id", inst.get("mnemonic", "<missing-id>"))
        mnemonic = str(inst.get("mnemonic", "")).strip().upper()

        # Historical cleanup guard: the vector block headers are VPAR/VSEQ.
        # If an older mnemonic spelling ("BSTART.VEC") reappears in golden/spec,
        # treat it as a hard error so it cannot silently regress.
        if mnemonic == "BSTART.VEC":
            errors.append(f"{inst_id}: forbidden mnemonic present in spec: BSTART.VEC (use BSTART.VPAR/VSEQ)")

        parts = inst.get("parts", [])
        enc = inst.get("encoding", {})
        enc_parts = enc.get("parts", [])

        if len(parts) != len(enc_parts):
            errors.append(f"{inst_id}: parts count {len(parts)} != encoding.parts count {len(enc_parts)}")
            continue

        for i, (part, enc_part) in enumerate(zip(parts, enc_parts)):
            width_bits = int(part.get("width_bits", 0))
            if int(enc_part.get("width_bits", -1)) != width_bits:
                errors.append(
                    f"{inst_id}: part[{i}] width_bits {width_bits} != encoding.width_bits {enc_part.get('width_bits')}"
                )
                continue

            # Segments should cover full width.
            segs = part.get("segments", [])
            seg_sum = sum(int(s.get("width", 0)) for s in segs)
            if seg_sum != width_bits:
                errors.append(f"{inst_id}: part[{i}] segments cover {seg_sum} bits, expected {width_bits}")

            # Derived mask/match should be within width.
            mask = _parse_hex(enc_part.get("mask", "0x0"))
            match = _parse_hex(enc_part.get("match", "0x0"))
            width_mask = _mask_for_width(width_bits)
            if (mask & ~width_mask) != 0:
                errors.append(f"{inst_id}: part[{i}] mask has bits outside width")
            if (match & ~width_mask) != 0:
                errors.append(f"{inst_id}: part[{i}] match has bits outside width")
            if (match & ~mask) != 0:
                errors.append(f"{inst_id}: part[{i}] match sets bits not covered by mask")

            pattern = enc_part.get("pattern", "")
            if len(pattern) != width_bits:
                errors.append(f"{inst_id}: part[{i}] pattern length {len(pattern)} != width {width_bits}")
            else:
                pmask, pmatch = _pattern_to_mask_match(pattern)
                if pmask != mask or pmatch != match:
                    errors.append(
                        f"{inst_id}: part[{i}] pattern-derived mask/match disagree "
                        f"(mask {pmask:#x} vs {mask:#x}, match {pmatch:#x} vs {match:#x})"
                    )

    return errors


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--profile",
        choices=["v0.3"],
        default="v0.3",
        help="ISA profile for default --spec path (v0.3 only)",
    )
    ap.add_argument(
        "--spec",
        default=None,
        help="Path to the generated ISA spec JSON",
    )
    args = ap.parse_args()

    default_spec = "spec/isa/spec/current/linxisa-v0.3.json"
    errors = validate(args.spec or default_spec)
    if errors:
        for e in errors[:200]:
            print(e, file=sys.stderr)
        if len(errors) > 200:
            print(f"... {len(errors) - 200} more", file=sys.stderr)
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
