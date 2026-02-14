#!/usr/bin/env python3
"""
Generate a single assembly file containing a decode vector for every instruction
encoding in the LinxISA spec.

This is meant for regression:
- Assemble the output as raw bytes in `.text`.
- Disassemble with `llvm-objdump -d` and verify that the LLVM disassembler
  recognizes the full spec (or at least doesn't regress).

The emitted bytes are derived from each form's `match` value with all variable
fields set to zero.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def _parse_int(s: str) -> int:
    # JSON currently stores `mask`/`match` as hex strings (e.g. "0x50160002").
    return int(str(s), 0)


def _canon_mnemonic(mnem: str) -> str:
    # Spec includes a tiny number of mnemonics with spaces ("BSTART CALL").
    return str(mnem).strip().replace(" ", ".")


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", type=Path, required=True, help="Path to spec/isa/spec/*.json")
    ap.add_argument("--out", type=Path, required=True, help="Output assembly file")
    args = ap.parse_args(argv)

    raw = json.loads(args.spec.read_text())
    instructions = raw.get("instructions", [])

    out: list[str] = []
    out.append(f"# Auto-generated from {args.spec}")
    out.append("# DO NOT EDIT BY HAND")
    out.append("")
    out.append("    .text")
    out.append("    .p2align 1")
    out.append("    .globl linxisa_decode_vectors")
    out.append("linxisa_decode_vectors:")

    for inst in instructions:
        inst_id = str(inst.get("id", "")).strip()
        mnem = _canon_mnemonic(inst.get("mnemonic", ""))
        enc = inst.get("encoding", {})
        length_bits = int(enc.get("length_bits", inst.get("length_bits", 0)) or 0)
        parts = list(enc.get("parts", []))

        if length_bits not in (16, 32, 48, 64) or not parts:
            continue

        out.append(f"    # {mnem} ({inst_id}) [{length_bits}]")

        if length_bits == 16:
            val = _parse_int(parts[0].get("match", "0")) & 0xFFFF
            out.append(f"    .2byte 0x{val:04x}")
        elif length_bits == 32:
            val = _parse_int(parts[0].get("match", "0")) & 0xFFFFFFFF
            out.append(f"    .4byte 0x{val:08x}")
        elif length_bits == 48:
            val = _parse_int(parts[0].get("match", "0")) & ((1 << 48) - 1)
            lo = val & 0xFFFFFFFF
            hi = (val >> 32) & 0xFFFF
            out.append(f"    .4byte 0x{lo:08x}")
            out.append(f"    .2byte 0x{hi:04x}")
        elif length_bits == 64:
            if len(parts) != 2:
                continue
            lo = _parse_int(parts[0].get("match", "0")) & 0xFFFFFFFF
            hi = _parse_int(parts[1].get("match", "0")) & 0xFFFFFFFF
            out.append(f"    .4byte 0x{lo:08x}")
            out.append(f"    .4byte 0x{hi:08x}")

        out.append("")

    out.append("    .p2align 1")
    out.append("linxisa_decode_vectors_end:")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(out) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(__import__("sys").argv[1:]))

