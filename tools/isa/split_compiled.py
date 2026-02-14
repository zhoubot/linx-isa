#!/usr/bin/env python3
"""
Split a compiled catalog JSON into golden source files under spec/isa/golden/<version>/.

This is intended for bootstrapping/review and is not authoritative. The authoritative
pipeline is build_golden.py (golden -> compiled).
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List


def _read_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def _write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _mnemonic_token(mnemonic: str) -> str:
    # Quote mnemonics containing spaces using a JSON string literal.
    if " " in mnemonic or mnemonic.startswith('"'):
        return json.dumps(mnemonic, ensure_ascii=True)
    return mnemonic


def _extract_constraints_part0(inst: Dict[str, Any]) -> List[str]:
    out: List[str] = []
    enc = inst.get("encoding") or {}
    parts = enc.get("parts") or []
    if not parts:
        return out
    c = parts[0].get("constraints") or []
    for it in c:
        field = str(it.get("field") or "").strip()
        op = str(it.get("op") or "").strip()
        value = str(it.get("value") or "").strip()
        if field and op and value:
            out.append(f"{field}{op}{value}")
    return out


def _segments_to_assignments(part: Dict[str, Any]) -> str:
    # Preserve segment token text verbatim for stable IDs.
    toks: List[str] = []
    for seg in part.get("segments", []):
        msb = int(seg["msb"])
        lsb = int(seg["lsb"])
        tok = str(seg["token"])
        # Strip inline constraint decorations like "RegDst≠RA" and "BrType!=0".
        # Constraints are emitted from encoding.part[0].constraints instead.
        if "const" not in seg:
            for op in ("≠", "!="):
                if op in tok:
                    tok = tok.split(op, 1)[0].strip()
                    break
            # Collapse whitespace inside token (some legacy field tokens include spaces).
            tok = "".join(tok.split())
        lhs = f"{msb}..{lsb}" if msb != lsb else f"{msb}"
        toks.append(f"{lhs}={tok}")
    return " ".join(toks)


def _operands_from_encoding(inst: Dict[str, Any]) -> List[str]:
    names: List[str] = []
    seen: set[str] = set()
    for ep in (inst.get("encoding") or {}).get("parts", []):
        for f in ep.get("fields", []):
            n = str(f.get("name") or "").strip()
            n = "".join(n.split())
            if not n or n in seen:
                continue
            seen.add(n)
            names.append(n)
    return names


def _opc_line(inst: Dict[str, Any]) -> str:
    mnemonic = str(inst["mnemonic"]).strip()
    length_bits = int(inst["length_bits"])
    asm = str(inst.get("asm") or "")
    group = str(inst.get("group") or "")

    meta = {"asm": asm, "group": group, "length_bits": length_bits}
    meta_s = json.dumps(meta, ensure_ascii=True, separators=(",", ":"))

    enc_s = " | ".join(_segments_to_assignments(p) for p in (inst.get("parts") or []))

    operands = _operands_from_encoding(inst)
    ops_s = " ".join(operands)

    cons = _extract_constraints_part0(inst)
    cons_s = ",".join(cons) if cons else "-"

    return f"{_mnemonic_token(mnemonic)} [{meta_s}] : {enc_s} ; {ops_s} ; {cons_s}"


def _bucket_opc_path(inst: Dict[str, Any], out_dir: Path) -> Path:
    length_bits = int(inst.get("length_bits") or 0)
    if length_bits == 16:
        return out_dir / "lx_c.opc"
    if length_bits == 48:
        return out_dir / "lx_hl48.opc"
    if length_bits == 64:
        return out_dir / "lx_64_prefix.opc"
    if length_bits != 32:
        raise ValueError(f"unexpected length_bits {length_bits} for {inst.get('mnemonic')}")
    return out_dir / "lx_32.opc"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", default="spec/isa/spec/current/linxisa-v0.3.json", help="Compiled spec JSON path")
    ap.add_argument("--out", default="spec/isa/golden/v0.3", help="Golden output directory")
    args = ap.parse_args()

    spec_path = Path(args.spec)
    out_root = Path(args.out)
    out_opc = out_root / "opcodes"

    spec = _read_json(spec_path)
    insts = list(spec.get("instructions", []))

    insts.sort(key=lambda i: (str(i.get("mnemonic")), int(i.get("length_bits", 0)), str(i.get("id"))))

    buckets: Dict[Path, List[str]] = {}
    for inst in insts:
        inst = dict(inst)
        inst.pop("note", None)
        inst["parts"] = [{k: v for (k, v) in p.items() if k != "explain"} for p in (inst.get("parts") or [])]
        p = _bucket_opc_path(inst, out_opc)
        buckets.setdefault(p, []).append(_opc_line(inst))

    for path, lines in sorted(buckets.items(), key=lambda kv: kv[0].name):
        header = [
            "# LinxISA opcode database (generated via tools/isa/split_compiled.py)",
            "#",
            "# Format:",
            "#   MNEMONIC [<json meta>] : <bit-assignments> [| <bit-assignments>] ; <operands> ; <constraints>",
            "#",
            "# Notes:",
            "#   - If MNEMONIC contains spaces, it is written as a JSON string literal.",
            "#   - Meta is a JSON object containing at least: {asm, group, length_bits}.",
            "#   - Constraints apply to encoding part 0 and are written as tokens like Field!=0.",
            "",
        ]
        _write_text(path, "\n".join(header + lines) + "\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
