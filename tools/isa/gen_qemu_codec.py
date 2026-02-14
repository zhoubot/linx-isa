#!/usr/bin/env python3
"""
Generate QEMU decodetree-style codec files from the compiled ISA JSON spec.

Outputs (by default) into `spec/isa/generated/codecs/`:
  - linxisa16.decode
  - linxisa32.decode
  - linxisa48.decode
  - linxisa64.decode

The generated files are intended to be *pure encoding/decoding tables*:
  - unique pattern identifiers per instruction form (from `instructions[].id`)
  - fixed-bit patterns (0/1/.) in MSB->LSB order
  - field definitions (%field ...) derived from `instructions[].encoding`

They can be used as a starting point for:
  - QEMU decoders (decodetree.py)
  - other tooling that wants a stable text encoding of mask/match + fields
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import os
import re
import tempfile
from collections import defaultdict
from typing import Any, Dict, Iterable, List, Optional, Tuple


_IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def _to_ident(s: str) -> str:
    s = s.strip()
    s = re.sub(r"[^A-Za-z0-9_]+", "_", s)
    s = re.sub(r"_+", "_", s)
    s = s.strip("_")
    if not s:
        return "field"
    if not re.match(r"^[A-Za-z_]", s):
        s = f"f_{s}"
    return s


def _group_pattern(pattern: str, group: int) -> str:
    pattern = pattern.strip()
    if group <= 0:
        return pattern
    chunks: List[str] = []
    for i in range(0, len(pattern), group):
        chunks.append(pattern[i : i + group])
    return " ".join(chunks)

def _repo_root() -> str:
    return os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))


def _normalize_spec_label(spec_path: str) -> str:
    """
    Render a stable, repo-relative spec label for generated headers.

    The generator output should not depend on whether the caller passed an
    absolute or relative --spec path. When the spec file lives under the repo
    root, emit a repo-relative path; otherwise fall back to a normalized path.
    """

    spec_abs = os.path.abspath(spec_path)
    root = _repo_root()
    try:
        rel = os.path.relpath(spec_abs, root)
    except ValueError:
        rel = None
    if rel and not rel.startswith(os.pardir + os.sep):
        return os.path.normpath(rel)
    return os.path.normpath(spec_path)


@dataclasses.dataclass(frozen=True)
class Piece:
    insn_lsb: int
    width: int
    value_msb: Optional[int] = None
    value_lsb: Optional[int] = None
    token: Optional[str] = None

    @property
    def insn_msb(self) -> int:
        return self.insn_lsb + self.width - 1


@dataclasses.dataclass
class Field:
    base: str
    signed: Optional[bool]
    pieces: List[Piece]

    def sorted_pieces_msb_to_lsb(self) -> List[Piece]:
        # Prefer explicit value-bit slice ordering if present.
        if all(p.value_msb is not None for p in self.pieces):
            return sorted(self.pieces, key=lambda p: int(p.value_msb), reverse=True)
        return sorted(self.pieces, key=lambda p: p.insn_msb, reverse=True)


@dataclasses.dataclass(frozen=True)
class FieldSignaturePiece:
    insn_lsb: int
    width: int
    signed: bool


@dataclasses.dataclass(frozen=True)
class FieldSignature:
    pieces: Tuple[FieldSignaturePiece, ...]

    def suffix(self) -> str:
        # Example: "20_s12__7_5"
        parts: List[str] = []
        for p in self.pieces:
            parts.append(f"{p.insn_lsb}_{'s' if p.signed else ''}{p.width}")
        return "__".join(parts) if parts else "empty"


def _build_combined_encoding(inst: Dict[str, Any]) -> Tuple[int, str, Dict[str, Field]]:
    """
    Return (length_bits, pattern_msb_to_lsb, fields_by_base).

    For multi-part instructions (e.g. 64-bit forms as 2x32 parts), we treat the
    instruction as a single bit-vector with:
      - part0 at bit offset 0 (first in stream)
      - part1 at bit offset width(part0)
      - etc
    This matches little-endian packing if the instruction words are loaded into
    an integer with the first word as the low bits.
    """

    enc = inst.get("encoding", {})
    parts: List[Dict[str, Any]] = list(enc.get("parts", []))

    length_bits = int(enc.get("length_bits", inst.get("length_bits", 0)))
    if not parts:
        return length_bits, "." * length_bits, {}

    # Stream-order offsets (part0 is first word in memory stream).
    offsets: List[int] = []
    off = 0
    for p in parts:
        offsets.append(off)
        off += int(p.get("width_bits", 0))

    # Combined pattern: MSB->LSB, so reverse stream order when concatenating.
    combined_pattern = "".join(
        str(parts[i].get("pattern", "")).replace(" ", "")
        for i in reversed(range(len(parts)))
    )
    if len(combined_pattern) != length_bits:
        # Fall back to best-effort padding.
        combined_pattern = (("." * length_bits) + combined_pattern)[-length_bits:]

    fields_by_base: Dict[str, Field] = {}

    for part_index, part in enumerate(parts):
        part_off = offsets[part_index]
        for f in part.get("fields", []):
            base = _to_ident(str(f.get("name", "")))
            signed = f.get("signed", None)
            field = fields_by_base.get(base)
            if field is None:
                field = Field(base=base, signed=signed, pieces=[])
                fields_by_base[base] = field
            # Preserve signed hint if any part marks it as signed/unsigned.
            if field.signed is None and signed is not None:
                field.signed = signed

            for piece in f.get("pieces", []):
                insn_lsb = int(piece.get("insn_lsb", 0)) + part_off
                width = int(piece.get("width", 0))
                vp_msb = piece.get("value_msb")
                vp_lsb = piece.get("value_lsb")
                field.pieces.append(
                    Piece(
                        insn_lsb=insn_lsb,
                        width=width,
                        value_msb=int(vp_msb) if vp_msb is not None else None,
                        value_lsb=int(vp_lsb) if vp_lsb is not None else None,
                        token=str(piece.get("token", "")) if piece.get("token") is not None else None,
                    )
                )

    return length_bits, combined_pattern, fields_by_base


def _field_signature(field: Field) -> FieldSignature:
    pieces = field.sorted_pieces_msb_to_lsb()
    signed_piece_index = 0 if (field.signed is True and pieces) else None
    sig_pieces: List[FieldSignaturePiece] = []
    for idx, p in enumerate(pieces):
        sig_pieces.append(
            FieldSignaturePiece(
                insn_lsb=p.insn_lsb,
                width=p.width,
                signed=(signed_piece_index == idx),
            )
        )
    return FieldSignature(tuple(sig_pieces))


def _render_field_def(name: str, field: Field, sig: FieldSignature) -> str:
    # QEMU decodetree concatenates in order given (MSB pieces first).
    pieces = field.sorted_pieces_msb_to_lsb()
    signed_piece_index = 0 if (field.signed is True and pieces) else None
    parts: List[str] = []
    for idx, p in enumerate(pieces):
        signed = signed_piece_index == idx
        parts.append(f"{p.insn_lsb}:{'s' if signed else ''}{p.width}")
    return f"%{name} " + " ".join(parts)


def _choose_field_def_names(
    by_base_sigs: Dict[str, List[FieldSignature]]
) -> Dict[Tuple[str, FieldSignature], str]:
    chosen: Dict[Tuple[str, FieldSignature], str] = {}
    for base, sigs in by_base_sigs.items():
        sigs_sorted = sorted(sigs, key=lambda s: s.suffix())
        if len(sigs_sorted) == 1 and _IDENT_RE.match(base):
            chosen[(base, sigs_sorted[0])] = base
        else:
            for sig in sigs_sorted:
                chosen[(base, sig)] = _to_ident(f"{base}__{sig.suffix()}")
    return chosen


def _generate_decode_file(
    instructions: List[Dict[str, Any]], out_path: str, spec_label: str
) -> None:
    inst_encodings: List[Tuple[Dict[str, Any], int, str, Dict[str, Field]]] = []

    for inst in instructions:
        length_bits, pattern, fields_by_base = _build_combined_encoding(inst)
        inst_encodings.append((inst, length_bits, pattern, fields_by_base))

    # Collect field signatures from per-instruction layouts.
    by_base_sigs: Dict[str, List[FieldSignature]] = defaultdict(list)
    rep: Dict[Tuple[str, FieldSignature], Field] = {}
    for _, _, _, fields_by_base in inst_encodings:
        for base, field in fields_by_base.items():
            sig = _field_signature(field)
            if sig not in by_base_sigs[base]:
                by_base_sigs[base].append(sig)
            rep.setdefault((base, sig), field)

    # Determine field definition names.
    chosen = _choose_field_def_names(by_base_sigs)

    # Build a deterministic list of (def_name, representative Field, signature).
    field_defs: List[Tuple[str, Field, FieldSignature]] = []
    for (base, sig), def_name in chosen.items():
        if (base, sig) not in rep:
            continue
        field_defs.append((def_name, rep[(base, sig)], sig))
    field_defs.sort(key=lambda t: t[0])

    lines: List[str] = []
    lines.append(f"# Auto-generated from {os.path.normpath(spec_label)}")
    lines.append(f"# DO NOT EDIT: run `python3 tools/isa/gen_qemu_codec.py` to regenerate.")
    lines.append("")

    # Field definitions.
    lines.append("# Fields")
    for def_name, field, sig in field_defs:
        lines.append(_render_field_def(def_name, field, sig))
    lines.append("")

    # Instruction patterns.
    lines.append("# Instruction forms")
    # Stable ordering: by mnemonic, then by id.
    inst_encodings.sort(key=lambda t: (str(t[0].get("mnemonic", "")), str(t[0].get("id", ""))))

    for inst, length_bits, pattern, fields_by_base in inst_encodings:
        inst_id = _to_ident(str(inst.get("id", "inst")))
        mnemonic = str(inst.get("mnemonic", ""))
        asm = str(inst.get("asm", "")) if inst.get("asm") is not None else ""
        src = inst.get("source", {}) or {}
        src_file = src.get("file")
        src_line = src.get("line")
        comment = f"# {mnemonic}"
        if asm:
            comment += f" | {asm}"
        if src_file and src_line:
            comment += f" | {src_file}:{src_line}"
        lines.append(comment)

        grouped_pat = _group_pattern(pattern, 4)

        # Field refs for this instruction.
        refs: List[str] = []
        for base, field in sorted(fields_by_base.items(), key=lambda kv: kv[0]):
            sig = _field_signature(field)
            def_name = chosen.get((base, sig))
            if def_name is None:
                continue
            if def_name == base:
                refs.append(f"%{base}")
            else:
                refs.append(f"{base}=%{def_name}")

        # Emit line: <id> <pattern> <refs...>
        if refs:
            lines.append(f"{inst_id} {grouped_pat} " + " ".join(refs))
        else:
            lines.append(f"{inst_id} {grouped_pat}")
        lines.append("")

    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines).rstrip() + "\n")


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
        help="Path to ISA JSON spec",
    )
    ap.add_argument(
        "--out-dir",
        default=os.path.join("spec", "isa", "generated", "codecs"),
        help="Output directory for codec files",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="Verify that files in --out-dir match the generator output",
    )
    args = ap.parse_args()

    default_spec = os.path.join("spec", "isa", "spec", "current", "linxisa-v0.3.json")
    spec_path = args.spec or default_spec

    with open(spec_path, "r", encoding="utf-8") as f:
        spec = json.load(f)

    spec_label = os.path.normpath(str(spec.get("_spec_path") or _normalize_spec_label(spec_path)))

    by_len: Dict[int, List[Dict[str, Any]]] = defaultdict(list)
    for inst in spec.get("instructions", []):
        length_bits = int(inst.get("length_bits", inst.get("encoding", {}).get("length_bits", 0)))
        by_len[length_bits].append(inst)

    targets = [
        (16, "linxisa16.decode"),
        (32, "linxisa32.decode"),
        (48, "linxisa48.decode"),
        (64, "linxisa64.decode"),
    ]
    if args.check:
        with tempfile.TemporaryDirectory() as td:
            for length_bits, filename in targets:
                tmp_path = os.path.join(td, filename)
                _generate_decode_file(by_len.get(length_bits, []), tmp_path, spec_label)

                out_path = os.path.join(args.out_dir, filename)
                if not os.path.exists(out_path):
                    print(f"MISSING {out_path}")
                    return 1
                with open(tmp_path, "r", encoding="utf-8") as f:
                    expected = f.read()
                with open(out_path, "r", encoding="utf-8") as f:
                    actual = f.read()
                if expected != actual:
                    print(f"OUTDATED {out_path} (regenerate with gen_qemu_codec.py)")
                    return 1
        print("OK")
    else:
        for length_bits, filename in targets:
            out_path = os.path.join(args.out_dir, filename)
            _generate_decode_file(by_len.get(length_bits, []), out_path, spec_label)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
