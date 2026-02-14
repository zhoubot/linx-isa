#!/usr/bin/env python3
"""
Generate C encode/decode tables from the compiled ISA JSON spec.

The generated tables are intended as a low-friction bridge for:
  - LLVM MC disassembler/encoder implementations
  - binutils opcodes/disassembler implementations
  - standalone reference tooling

Outputs into `spec/isa/generated/codecs/`:
  - linxisa_opcodes.h
  - linxisa_opcodes.c
"""

from __future__ import annotations

import argparse
import json
import os
from typing import Any, Dict, List, Optional, Tuple


def _c_string(s: str) -> str:
    s = s.replace("\\", "\\\\")
    s = s.replace('"', '\\"')
    s = s.replace("\n", "\\n")
    return f'"{s}"'


def _repo_root() -> str:
    return os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))


def _normalize_spec_label(spec_path: str) -> str:
    spec_abs = os.path.abspath(spec_path)
    root = _repo_root()
    try:
        rel = os.path.relpath(spec_abs, root)
    except ValueError:
        rel = None
    if rel and not rel.startswith(os.pardir + os.sep):
        return os.path.normpath(rel)
    return os.path.normpath(spec_path)


def _pattern_to_mask_match(pattern: str) -> Tuple[int, int]:
    # pattern is MSB->LSB with '0','1','.'
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


def _build_combined_encoding(inst: Dict[str, Any]) -> Tuple[int, str, Dict[str, Any]]:
    """
    Return (length_bits, pattern_msb_to_lsb, combined_fields_by_base).

    For multi-part instructions, we treat the instruction as a single bit-vector with:
      - part0 at bit offset 0 (first in stream)
      - part1 at bit offset width(part0)
    """
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

    combined_pattern = "".join(
        str(parts[i].get("pattern", "")).replace(" ", "") for i in reversed(range(len(parts)))
    )
    if len(combined_pattern) != length_bits:
        combined_pattern = (("." * length_bits) + combined_pattern)[-length_bits:]

    fields: Dict[str, Any] = {}
    for part_index, part in enumerate(parts):
        part_off = offsets[part_index]
        for f in part.get("fields", []):
            base = str(f.get("name", ""))
            if not base:
                continue
            existing = fields.get(base)
            if existing is None:
                existing = {"name": base, "signed": f.get("signed", None), "pieces": []}
                fields[base] = existing
            if existing.get("signed") is None and f.get("signed") is not None:
                existing["signed"] = f.get("signed")

            for piece in f.get("pieces", []):
                p = dict(piece)
                p["insn_lsb"] = int(piece.get("insn_lsb", 0)) + part_off
                p["insn_msb"] = int(piece.get("insn_msb", 0)) + part_off
                existing["pieces"].append(p)

    # Deterministic ordering: sort pieces by value slice (lsb), then insn bits.
    for f in fields.values():
        pieces = list(f.get("pieces", []))
        pieces.sort(
            key=lambda p: (
                int(p.get("value_lsb", 0) if p.get("value_lsb") is not None else 0),
                int(p.get("insn_lsb", 0)),
            )
        )
        f["pieces"] = pieces

    return length_bits, combined_pattern, fields


def _render_header(spec_label: str) -> str:
    spec_label = os.path.normpath(spec_label)
    return "\n".join(
        [
            f"/* Auto-generated from {spec_label}. */",
            "/* DO NOT EDIT: run `python3 tools/isa/gen_c_codec.py` to regenerate. */",
            "",
            "#pragma once",
            "",
            "#include <stddef.h>",
            "#include <stdint.h>",
            "",
            "/* A single instruction form (unique encodable bit-pattern). */",
            "typedef struct {",
            "  const char *id;          /* stable identifier */",
            "  const char *mnemonic;    /* draft mnemonic (e.g. 'ADD', 'C.ADD', 'HL.ADDI') */",
            "  const char *asm_fmt;     /* assembly template from the ISA catalog (may be empty) */",
            "  uint16_t length_bits;    /* 16/32/48/64 */",
            "  uint64_t mask;           /* fixed-bit mask over the packed instruction bitvector */",
            "  uint64_t match;          /* fixed-bit value over the packed instruction bitvector */",
            "  uint32_t field_start;    /* index into linxisa_fields[] */",
            "  uint16_t field_count;    /* number of fields for this instruction */",
            "} linxisa_inst_form;",
            "",
            "/* A symbolic field (e.g. RegDst, SrcL, simm12, uimm24, ...). */",
            "typedef struct {",
            "  const char *name;",
            "  int8_t signed_hint;      /* -1 unspecified, 0 unsigned, 1 signed */",
            "  uint16_t bit_width;      /* field bit-width */",
            "  uint32_t piece_start;    /* index into linxisa_field_pieces[] */",
            "  uint8_t piece_count;",
            "} linxisa_field;",
            "",
            "/* A piece of a field (supports disjoint immediates). */",
            "typedef struct {",
            "  uint8_t insn_lsb;        /* bit position in packed instruction */",
            "  uint8_t width;           /* number of bits */",
            "  uint8_t value_lsb;       /* bit position within the logical field value */",
            "} linxisa_field_piece;",
            "",
            "extern const linxisa_inst_form linxisa_inst_forms[];",
            "extern const size_t linxisa_inst_forms_count;",
            "extern const linxisa_field linxisa_fields[];",
            "extern const size_t linxisa_fields_count;",
            "extern const linxisa_field_piece linxisa_field_pieces[];",
            "extern const size_t linxisa_field_pieces_count;",
            "",
        ]
    )


def _emit_tables(spec: Dict[str, Any], spec_label: str) -> Tuple[str, str]:
    insts = list(spec.get("instructions", []))
    # Stable ordering.
    insts.sort(key=lambda i: (str(i.get("mnemonic", "")), str(i.get("id", ""))))

    field_pieces: List[Dict[str, Any]] = []
    fields: List[Dict[str, Any]] = []
    forms: List[Dict[str, Any]] = []

    for inst in insts:
        length_bits, pattern, combined_fields = _build_combined_encoding(inst)
        mask, match = _pattern_to_mask_match(pattern)

        inst_field_start = len(fields)

        for fname in sorted(combined_fields.keys()):
            f = combined_fields[fname]
            signed = f.get("signed", None)
            signed_hint = -1
            if signed is True:
                signed_hint = 1
            elif signed is False:
                signed_hint = 0

            pieces = list(f.get("pieces", []))
            if pieces:
                # bit-width derives from value slice when present, otherwise piece width.
                if all(p.get("value_msb") is not None for p in pieces):
                    bit_width = max(int(p["value_msb"]) for p in pieces) + 1
                else:
                    bit_width = max(int(p.get("width", 0)) for p in pieces)
            else:
                bit_width = 0

            piece_start = len(field_pieces)
            for p in pieces:
                width = int(p.get("width", 0))
                value_lsb = int(p.get("value_lsb", 0) if p.get("value_lsb") is not None else 0)
                field_pieces.append(
                    {
                        "insn_lsb": int(p.get("insn_lsb", 0)),
                        "width": width,
                        "value_lsb": value_lsb,
                    }
                )

            fields.append(
                {
                    "name": fname,
                    "signed_hint": signed_hint,
                    "bit_width": bit_width,
                    "piece_start": piece_start,
                    "piece_count": len(pieces),
                }
            )

        forms.append(
            {
                "id": str(inst.get("id", "")),
                "mnemonic": str(inst.get("mnemonic", "")),
                "asm_fmt": str(inst.get("asm", "")) if inst.get("asm") is not None else "",
                "length_bits": length_bits,
                "mask": mask,
                "match": match,
                "field_start": inst_field_start,
                "field_count": len(fields) - inst_field_start,
            }
        )

    # Header.
    h = _render_header(spec_label)

    # C source.
    c_lines: List[str] = []
    c_lines.append(f"/* Auto-generated from {os.path.normpath(spec_label)}. */")
    c_lines.append("/* DO NOT EDIT: run `python3 tools/isa/gen_c_codec.py` to regenerate. */")
    c_lines.append("")
    c_lines.append('#include "linxisa_opcodes.h"')
    c_lines.append("")

    # Pieces.
    c_lines.append("const linxisa_field_piece linxisa_field_pieces[] = {")
    for p in field_pieces:
        c_lines.append(
            "  {"
            f" .insn_lsb = {int(p['insn_lsb'])},"
            f" .width = {int(p['width'])},"
            f" .value_lsb = {int(p['value_lsb'])}"
            " },"
        )
    c_lines.append("};")
    c_lines.append(f"const size_t linxisa_field_pieces_count = {len(field_pieces)};")
    c_lines.append("")

    # Fields.
    c_lines.append("const linxisa_field linxisa_fields[] = {")
    for f in fields:
        c_lines.append(
            "  {"
            f" .name = {_c_string(str(f['name']))},"
            f" .signed_hint = {int(f['signed_hint'])},"
            f" .bit_width = {int(f['bit_width'])},"
            f" .piece_start = {int(f['piece_start'])},"
            f" .piece_count = {int(f['piece_count'])}"
            " },"
        )
    c_lines.append("};")
    c_lines.append(f"const size_t linxisa_fields_count = {len(fields)};")
    c_lines.append("")

    # Instruction forms.
    c_lines.append("const linxisa_inst_form linxisa_inst_forms[] = {")
    for form in forms:
        c_lines.append(
            "  {"
            f" .id = {_c_string(form['id'])},"
            f" .mnemonic = {_c_string(form['mnemonic'])},"
            f" .asm_fmt = {_c_string(form['asm_fmt'])},"
            f" .length_bits = {int(form['length_bits'])},"
            f" .mask = 0x{int(form['mask']):016x}ULL,"
            f" .match = 0x{int(form['match']):016x}ULL,"
            f" .field_start = {int(form['field_start'])},"
            f" .field_count = {int(form['field_count'])}"
            " },"
        )
    c_lines.append("};")
    c_lines.append(f"const size_t linxisa_inst_forms_count = {len(forms)};")
    c_lines.append("")

    return h + "\n", "\n".join(c_lines) + "\n"


def _write_if_different(path: str, content: str, check: bool) -> None:
    if check:
        if not os.path.exists(path):
            raise SystemExit(f"MISSING {path} (run gen_c_codec.py)")
        with open(path, "r", encoding="utf-8") as f:
            old = f.read()
        if old != content:
            raise SystemExit(f"OUTDATED {path} (regenerate with gen_c_codec.py)")
        return

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--profile",
        choices=["v0.3"],
        default="v0.3",
        help="ISA profile for default --spec path (v0.3 only)",
    )
    ap.add_argument("--spec", default=None, help="Path to the ISA spec JSON")
    ap.add_argument("--out-dir", default="spec/isa/generated/codecs", help="Output directory")
    ap.add_argument("--check", action="store_true", help="Fail if outputs are not up-to-date")
    args = ap.parse_args()

    default_spec = "spec/isa/spec/current/linxisa-v0.3.json"
    spec_path = args.spec or default_spec

    with open(spec_path, "r", encoding="utf-8") as f:
        spec = json.load(f)
    spec_label = os.path.normpath(str(spec.get("_spec_path") or _normalize_spec_label(spec_path)))

    header, source = _emit_tables(spec, spec_label)

    out_h = os.path.join(args.out_dir, "linxisa_opcodes.h")
    out_c = os.path.join(args.out_dir, "linxisa_opcodes.c")

    _write_if_different(out_h, header, check=args.check)
    _write_if_different(out_c, source, check=args.check)

    if args.check:
        print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
