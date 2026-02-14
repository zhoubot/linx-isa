#!/usr/bin/env python3
"""
Reference LinxISA disassembler driven by the compiled ISA JSON spec.

This is intentionally lightweight and meant for spec validation and bring-up.
It does not attempt to implement full label/relocation printing; it focuses on
mask/match decode + field extraction.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from typing import Any, Dict, List, Optional, Tuple


_HEX_RE = re.compile(r"^(?:0x)?[0-9a-fA-F_]+$")


def _parse_hex_word(s: str) -> Tuple[int, int]:
    """
    Parse a hex token and return (value, bit_width), inferring width from digits.
    Accepted widths: 16/32/48/64.
    """
    s = s.strip()
    if not _HEX_RE.match(s):
        raise ValueError(f"invalid hex token: {s!r}")
    s = s.lower().removeprefix("0x").replace("_", "")
    if len(s) == 0:
        raise ValueError("empty hex token")
    bit_width = len(s) * 4
    if bit_width not in (16, 32, 48, 64):
        raise ValueError(f"hex token width {bit_width} not in (16,32,48,64): {s!r}")
    return int(s, 16), bit_width


def _pattern_to_mask_match(pattern: str) -> Tuple[int, int]:
    width_bits = len(pattern)
    mask = 0
    match = 0
    for i, ch in enumerate(pattern):
        bit = width_bits - 1 - i
        if ch == ".":
            continue
        if ch not in ("0", "1"):
            raise ValueError(f"invalid pattern char {ch!r}")
        mask |= 1 << bit
        if ch == "1":
            match |= 1 << bit
    return mask, match


@dataclass(frozen=True)
class FieldPiece:
    insn_lsb: int
    width: int
    value_lsb: int


@dataclass
class Field:
    name: str
    signed: Optional[bool]
    pieces: List[FieldPiece]

    @property
    def bit_width(self) -> int:
        if not self.pieces:
            return 0
        return max(p.value_lsb + p.width for p in self.pieces)


@dataclass(frozen=True)
class Form:
    id: str
    mnemonic: str
    asm_fmt: str
    length_bits: int
    mask: int
    match: int
    fixed_bits: int
    fields: Dict[str, Field]


def _load_reg5(spec: Dict[str, Any]) -> Dict[int, str]:
    reg5 = spec.get("registers", {}).get("reg5", {})
    entries = reg5.get("entries", [])
    code_to_asm: Dict[int, str] = {}
    for e in entries:
        try:
            code = int(e.get("code"))
        except Exception:
            continue
        asm = str(e.get("asm", "")).strip()
        if asm:
            code_to_asm[code] = asm
    return code_to_asm


def _build_combined(inst: Dict[str, Any]) -> Tuple[int, str, Dict[str, Field]]:
    enc = inst.get("encoding", {})
    parts: List[Dict[str, Any]] = list(enc.get("parts", []))
    length_bits = int(enc.get("length_bits", inst.get("length_bits", 0)))
    if not parts:
        return length_bits, "." * length_bits, {}

    offsets: List[int] = []
    off = 0
    for p in parts:
        offsets.append(off)
        off += int(p.get("width_bits", 0))

    pattern = "".join(
        str(parts[i].get("pattern", "")).replace(" ", "") for i in reversed(range(len(parts)))
    )
    if len(pattern) != length_bits:
        pattern = (("." * length_bits) + pattern)[-length_bits:]

    fields: Dict[str, Field] = {}
    for part_index, part in enumerate(parts):
        part_off = offsets[part_index]
        for f in part.get("fields", []):
            name = str(f.get("name", "")).strip()
            if not name:
                continue
            existing = fields.get(name)
            if existing is None:
                existing = Field(name=name, signed=f.get("signed", None), pieces=[])
                fields[name] = existing
            if existing.signed is None and f.get("signed") is not None:
                existing.signed = f.get("signed")

            for p in f.get("pieces", []):
                insn_lsb = int(p.get("insn_lsb", 0)) + part_off
                width = int(p.get("width", 0))
                value_lsb = int(p.get("value_lsb", 0) if p.get("value_lsb") is not None else 0)
                existing.pieces.append(FieldPiece(insn_lsb=insn_lsb, width=width, value_lsb=value_lsb))

    # Stable piece ordering for deterministic output.
    for fld in fields.values():
        fld.pieces.sort(key=lambda p: (p.value_lsb, p.insn_lsb))

    return length_bits, pattern, fields


def _load_forms(spec: Dict[str, Any]) -> Dict[int, List[Form]]:
    out: Dict[int, List[Form]] = {}
    for inst in spec.get("instructions", []):
        length_bits, pattern, fields = _build_combined(inst)
        mask, match = _pattern_to_mask_match(pattern)
        fixed_bits = mask.bit_count()
        form = Form(
            id=str(inst.get("id", "")),
            mnemonic=str(inst.get("mnemonic", "")),
            asm_fmt=str(inst.get("asm", "") or ""),
            length_bits=length_bits,
            mask=mask,
            match=match,
            fixed_bits=fixed_bits,
            fields=fields,
        )
        out.setdefault(length_bits, []).append(form)

    # Prefer more specific encodings first.
    for k in list(out.keys()):
        out[k].sort(key=lambda f: (-f.fixed_bits, f.id))
    return out


def _extract_fields(val: int, form: Form) -> Dict[str, int]:
    out: Dict[str, int] = {}
    for name, field in form.fields.items():
        v = 0
        for p in field.pieces:
            bits = (val >> p.insn_lsb) & ((1 << p.width) - 1)
            v |= bits << p.value_lsb

        if field.signed is True and field.bit_width > 0:
            sign_bit = 1 << (field.bit_width - 1)
            if v & sign_bit:
                v -= 1 << field.bit_width

        out[name] = v
    return out


def _format_reg(code_to_asm: Dict[int, str], code: int) -> str:
    return code_to_asm.get(code, f"r{code}")

def _format_regdst(code_to_asm: Dict[int, str], code: int) -> str:
    """
    Format a RegDst-like field.

    LinxISA uses special destination selectors for queue pushes:
      - `->u` (encoded as reg5 code 30)
      - `->t` (encoded as reg5 code 31)
    These are not the same as reading `u#3`/`u#4` as source operands.
    """
    if code == 30:
        return "u"
    if code == 31:
        return "t"
    return _format_reg(code_to_asm, code)


def _format_inst_pretty(form: Form, fields: Dict[str, int], reg5: Dict[int, str]) -> str:
    # Mnemonic: prefer the asm template's first token (already lowercase in the ISA catalog).
    asm = form.asm_fmt.strip()
    mnem = asm.split()[0] if asm else form.mnemonic.lower()

    # Very small heuristic-based operand ordering.
    ops: List[str] = []

    for rname in ("SrcL", "SrcR", "SrcD", "SrcP", "SrcA"):
        if rname in fields:
            ops.append(_format_reg(reg5, int(fields[rname]) & 0x1F))

    # Immediates / misc fields (avoid register-like fields).
    for fname, v in sorted(fields.items()):
        if fname in {"SrcL", "SrcR", "SrcD", "SrcP", "SrcA", "RegDst"}:
            continue
        if fname.startswith(("simm", "uimm", "imm")) or fname.endswith(("imm",)):
            ops.append(str(v))

    # Destination (arrow style) when present.
    if "RegDst" in fields:
        ops.append(f"->" + _format_regdst(reg5, int(fields["RegDst"]) & 0x1F))
    else:
        # Handle "implicit dst" forms from the draft asm strings (e.g. C.LDI ->t, C.SETRET ->ra).
        asm_compact = asm.lower().replace(" ", "")
        if "->t" in asm_compact:
            ops.append("->t")
        elif "->u" in asm_compact:
            ops.append("->u")
        elif "->ra" in asm_compact:
            ops.append("->ra")

    if ops:
        return f"{mnem}\t" + ", ".join(ops)
    return mnem


def _decode_one(forms_by_len: Dict[int, List[Form]], val: int, length_bits: int) -> Optional[Form]:
    val &= (1 << length_bits) - 1
    for form in forms_by_len.get(length_bits, []):
        if (val & form.mask) == form.match:
            return form
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", default="spec/isa/spec/current/linxisa-v0.3.json")
    ap.add_argument("--hex", nargs="*", default=[], help="Hex instruction words (e.g. 5316 000fcf87)")
    ap.add_argument("--format", choices=("pretty", "fields"), default="pretty")
    args = ap.parse_args()

    with open(args.spec, "r", encoding="utf-8") as f:
        spec = json.load(f)

    reg5 = _load_reg5(spec)
    forms_by_len = _load_forms(spec)

    if not args.hex:
        ap.error("provide --hex words (binary stream mode not implemented yet)")

    for token in args.hex:
        val, bits = _parse_hex_word(token)
        form = _decode_one(forms_by_len, val, bits)
        if form is None:
            print(f"{token}\t<invalid>")
            continue
        extracted = _extract_fields(val, form)
        if args.format == "fields":
            items = []
            for k in sorted(extracted.keys()):
                v = extracted[k]
                if k in {"SrcL", "SrcR", "SrcD", "SrcP", "SrcA", "RegDst"}:
                    items.append(f"{k}={_format_reg(reg5, int(v) & 0x1F)}")
                else:
                    items.append(f"{k}={v}")
            print(f"{token}\t{form.mnemonic}\t" + " ".join(items))
        else:
            print(f"{token}\t" + _format_inst_pretty(form, extracted, reg5))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
