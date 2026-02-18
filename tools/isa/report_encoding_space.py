#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple


def _parse_hex(s: str) -> int:
    s = s.strip().lower()
    if not s.startswith("0x"):
        raise ValueError(f"expected hex string, got {s!r}")
    return int(s, 16)


def _parse_int(s: str) -> int:
    s = str(s).strip()
    if s.lower().startswith("0x"):
        return int(s, 16)
    if s.lower().startswith("0b"):
        return int(s, 2)
    return int(s, 10)


@dataclass(frozen=True)
class FieldPiece:
    insn_lsb: int
    insn_msb: int
    value_lsb: int
    width: int


@dataclass(frozen=True)
class Constraint:
    field: str
    op: str
    value_raw: str
    value: int


@dataclass(frozen=True)
class PartPat:
    inst_id: str
    mnemonic: str
    length_bits: int
    part_index: int
    width_bits: int
    mask: int
    match: int
    field_pieces: Dict[str, List[FieldPiece]]
    field_width: Dict[str, int]
    constraints: List[Constraint]


@dataclass(frozen=True)
class InstPat:
    inst_id: str
    mnemonic: str
    length_bits: int
    parts: Tuple[PartPat, ...]


def _overlap(a: PartPat, b: PartPat) -> bool:
    # Two fixedmask patterns overlap if they agree on all bits both constrain.
    common = a.mask & b.mask
    return (a.match & common) == (b.match & common)


def _extract_bits(val: int, msb: int, lsb: int) -> int:
    width = msb - lsb + 1
    return (val >> lsb) & ((1 << width) - 1)


def _eval_field(p: PartPat, insn: int, field: str) -> Optional[int]:
    pieces = p.field_pieces.get(field)
    if not pieces:
        return None
    out = 0
    for pc in pieces:
        bits = _extract_bits(insn, pc.insn_msb, pc.insn_lsb)
        out |= bits << pc.value_lsb
    return out


def _eval_constraints(p: PartPat, insn: int) -> bool:
    for c in p.constraints:
        fv = _eval_field(p, insn, c.field)
        if fv is None:
            return False
        if c.op == "==":
            if fv != c.value:
                return False
        elif c.op == "!=":
            if fv == c.value:
                return False
        elif c.op == "<":
            if not (fv < c.value):
                return False
        elif c.op == "<=":
            if not (fv <= c.value):
                return False
        elif c.op == ">":
            if not (fv > c.value):
                return False
        elif c.op == ">=":
            if not (fv >= c.value):
                return False
        else:
            return False
    return True


def _eval_constraint_op(op: str, lhs: int, rhs: int) -> bool:
    if op == "==":
        return lhs == rhs
    if op == "!=":
        return lhs != rhs
    if op == "<":
        return lhs < rhs
    if op == "<=":
        return lhs <= rhs
    if op == ">":
        return lhs > rhs
    if op == ">=":
        return lhs >= rhs
    raise ValueError(f"unsupported constraint operator: {op!r}")


def _match_with_constraints(p: PartPat, insn: int) -> bool:
    if (insn & p.mask) != p.match:
        return False
    return _eval_constraints(p, insn)


def _mask_for_bits(bits: Iterable[int]) -> int:
    out = 0
    for bit in bits:
        out |= 1 << bit
    return out


def _bits_msb_to_lsb(msb: int, lsb: int) -> List[int]:
    return list(range(lsb, msb + 1))


def _extract_patterns(spec_path: Path) -> List[PartPat]:
    spec = json.loads(spec_path.read_text())
    # Resolve register aliases (used by a small number of constraints).
    reg_alias_to_code: Dict[str, int] = {}
    try:
        entries = (spec.get("registers") or {}).get("reg5", {}).get("entries", [])
        for e in entries:
            code = int(e.get("code"))
            for a in (e.get("aliases") or []):
                reg_alias_to_code[str(a).upper()] = code
            reg_alias_to_code[str(e.get("asm")).upper()] = code
            reg_alias_to_code[str(e.get("name")).upper()] = code
    except Exception:
        reg_alias_to_code = {}

    out: List[PartPat] = []
    for inst in spec.get("instructions", []):
        inst_id = str(inst.get("id") or inst.get("mnemonic") or "<missing-id>")
        mnemonic = str(inst.get("mnemonic") or "<missing-mnemonic>")
        length_bits = int(inst.get("length_bits", 0))
        enc_parts = (inst.get("encoding") or {}).get("parts") or []
        for i, p in enumerate(enc_parts):
            field_pieces: Dict[str, List[FieldPiece]] = {}
            field_width: Dict[str, int] = {}
            for f in p.get("fields") or []:
                name = str(f.get("name"))
                pieces: List[FieldPiece] = []
                w = 0
                for pc in f.get("pieces") or []:
                    width = int(pc.get("width"))
                    pieces.append(
                        FieldPiece(
                            insn_lsb=int(pc.get("insn_lsb")),
                            insn_msb=int(pc.get("insn_msb")),
                            value_lsb=int(pc.get("value_lsb", 0)),
                            width=width,
                        )
                    )
                    w += width
                field_pieces[name] = pieces
                field_width[name] = w

            constraints: List[Constraint] = []
            for c in p.get("constraints") or []:
                field = str(c.get("field"))
                op = str(c.get("op"))
                value_raw = str(c.get("value"))
                value_int: Optional[int] = None
                try:
                    value_int = _parse_int(value_raw)
                except Exception:
                    value_int = reg_alias_to_code.get(value_raw.upper())
                if value_int is None:
                    # Unknown symbolic value; keep raw and conservatively treat as 0.
                    value_int = 0
                constraints.append(
                    Constraint(
                        field=field,
                        op=op,
                        value_raw=value_raw,
                        value=value_int,
                    )
                )

            out.append(
                PartPat(
                    inst_id=inst_id,
                    mnemonic=mnemonic,
                    length_bits=length_bits,
                    part_index=i,
                    width_bits=int(p.get("width_bits", 0)),
                    mask=_parse_hex(p.get("mask", "0x0")),
                    match=_parse_hex(p.get("match", "0x0")),
                    field_pieces=field_pieces,
                    field_width=field_width,
                    constraints=constraints,
                )
            )
    return out


def _build_instructions(parts: List[PartPat]) -> List[InstPat]:
    by_inst: Dict[str, List[PartPat]] = defaultdict(list)
    for p in parts:
        by_inst[p.inst_id].append(p)
    out: List[InstPat] = []
    for inst_id, ps in by_inst.items():
        ps_sorted = sorted(ps, key=lambda x: x.part_index)
        if not ps_sorted:
            continue
        out.append(
            InstPat(
                inst_id=inst_id,
                mnemonic=ps_sorted[0].mnemonic,
                length_bits=ps_sorted[0].length_bits,
                parts=tuple(ps_sorted),
            )
        )
    return out


def _constraints_disjoint(a: PartPat, b: PartPat) -> bool:
    """
    Best-effort disjointness check for constraint sets on part0.

    Returns True if we can prove there is no assignment satisfying both.
    Returns False if unknown / could overlap.
    """
    combined_mask = a.mask | b.mask
    combined_match = (a.match & a.mask) | (b.match & b.mask)

    fields = {c.field for c in a.constraints} | {c.field for c in b.constraints}
    if not fields:
        return False
    for field in fields:
        pieces = a.field_pieces.get(field) or b.field_pieces.get(field)
        if not pieces:
            return False

        width = a.field_width.get(field) or b.field_width.get(field) or 0
        if width <= 0 or width > 20:
            # Avoid big enumerations; treat as unknown.
            return False

        allowed = []
        for val in range(0, 1 << width):
            # Check compatibility with combined fixed bits for the bits that this field covers.
            ok = True
            for pc in pieces:
                field_slice = (val >> pc.value_lsb) & ((1 << pc.width) - 1)
                # The field slice maps to insn bits [insn_msb:insn_lsb].
                for k in range(pc.width):
                    insn_bit = pc.insn_lsb + k
                    bit_val = (field_slice >> k) & 1
                    if (combined_mask >> insn_bit) & 1:
                        if ((combined_match >> insn_bit) & 1) != bit_val:
                            ok = False
                            break
                if not ok:
                    break
            if not ok:
                continue

            # Apply constraints from both patterns on this field only.
            for c in a.constraints:
                if c.field != field:
                    continue
                try:
                    if not _eval_constraint_op(c.op, val, c.value):
                        ok = False
                        break
                except ValueError:
                    # Unknown operator: cannot prove disjointness.
                    return False
            if not ok:
                continue
            for c in b.constraints:
                if c.field != field:
                    continue
                try:
                    if not _eval_constraint_op(c.op, val, c.value):
                        ok = False
                        break
                except ValueError:
                    # Unknown operator: cannot prove disjointness.
                    return False
            if not ok:
                continue

            allowed.append(val)
            # Early exit: if we found at least one satisfying value for this field, we cannot
            # prove disjointness from this field alone.
            if len(allowed) >= 1:
                break

        if not allowed:
            return True

    return False


def _parts_overlap(a: PartPat, b: PartPat) -> bool:
    if a.width_bits != b.width_bits:
        return False
    if not _overlap(a, b):
        return False
    if a.width_bits == 16:
        for val in range(0x0000, 0x10000):
            if _match_with_constraints(a, val) and _match_with_constraints(b, val):
                return True
        return False
    return not _constraints_disjoint(a, b)


def _inst_overlap(a: InstPat, b: InstPat) -> bool:
    if len(a.parts) != len(b.parts):
        return False
    if tuple(p.width_bits for p in a.parts) != tuple(p.width_bits for p in b.parts):
        return False
    return all(_parts_overlap(pa, pb) for pa, pb in zip(a.parts, b.parts))


_ALLOWED_ALIAS_OVERLAP_MNEMONICS = {
    "BSTART.PAR",
    "BSTART.TEPL",
    "BSTART.MPAR",
    "BSTART.MSEQ",
    "BSTART.VPAR",
    "BSTART.VSEQ",
}


def _is_allowed_overlap(a: InstPat, b: InstPat) -> bool:
    mnems = {a.mnemonic, b.mnemonic}
    return mnems.issubset(_ALLOWED_ALIAS_OVERLAP_MNEMONICS)


def _conflicts_by_signature(insts: List[InstPat]) -> Dict[Tuple[int, ...], List[Tuple[InstPat, InstPat]]]:
    groups: Dict[Tuple[int, ...], List[InstPat]] = defaultdict(list)
    for it in insts:
        sig = tuple(p.width_bits for p in it.parts)
        groups[sig].append(it)

    out: Dict[Tuple[int, ...], List[Tuple[InstPat, InstPat]]] = {}
    for sig, xs in groups.items():
        conf: List[Tuple[InstPat, InstPat]] = []
        for i in range(len(xs)):
            for j in range(i + 1, len(xs)):
                if _inst_overlap(xs[i], xs[j]):
                    if _is_allowed_overlap(xs[i], xs[j]):
                        continue
                    conf.append((xs[i], xs[j]))
        out[sig] = conf
    return out


def _major_occupancy(
    pats: List[PartPat],
    *,
    width_bits: int,
    major_mask: int,
) -> Tuple[Dict[int, List[PartPat]], List[PartPat]]:
    """
    Return:
      - dict major_value -> list of patterns whose part0 fully constrains major_mask
      - list of patterns that do not fully constrain major_mask
    """
    buckets: Dict[int, List[PartPat]] = defaultdict(list)
    partial: List[PartPat] = []
    for p in pats:
        if p.part_index != 0 or p.width_bits != width_bits:
            continue
        if (p.mask & major_mask) != major_mask:
            partial.append(p)
            continue
        buckets[p.match & major_mask].append(p)
    return dict(buckets), partial


def _bruteforce_16bit(
    pats: List[PartPat],
) -> Tuple[int, int, List[Tuple[int, List[str]]], List[Tuple[int, int]]]:
    xs = [p for p in pats if p.part_index == 0 and p.width_bits == 16]
    holes = 0
    multi = 0
    examples: List[Tuple[int, List[str]]] = []
    hole_ranges: List[Tuple[int, int]] = []
    hole_start: Optional[int] = None
    for val in range(0x0000, 0x10000):
        hits = []
        for p in xs:
            if _match_with_constraints(p, val):
                hits.append(p.mnemonic)
        if not hits:
            holes += 1
            if hole_start is None:
                hole_start = val
            continue
        if hole_start is not None:
            hole_ranges.append((hole_start, val - 1))
            hole_start = None
        if len(hits) > 1:
            multi += 1
            if len(examples) < 50:
                examples.append((val, hits[:10]))
    if hole_start is not None:
        hole_ranges.append((hole_start, 0xFFFF))
    return holes, multi, examples, hole_ranges


def _masked_to_index(masked_value: int, bit_positions_lsb_to_msb: List[int]) -> int:
    """
    Convert a value with bits set at arbitrary instruction bit positions into a
    packed index where bit i corresponds to bit_positions_lsb_to_msb[i].
    """
    idx = 0
    for i, bit in enumerate(bit_positions_lsb_to_msb):
        if (masked_value >> bit) & 1:
            idx |= 1 << i
    return idx


def _index_to_masked(index: int, bit_positions_lsb_to_msb: List[int]) -> int:
    masked = 0
    for i, bit in enumerate(bit_positions_lsb_to_msb):
        if (index >> i) & 1:
            masked |= 1 << bit
    return masked


def _display_path(path: Path) -> str:
    try:
        repo_root = Path(__file__).resolve().parents[2]
        return str(path.resolve().relative_to(repo_root))
    except Exception:
        return str(path)


def _write_report(
    out_path: Path,
    *,
    spec_path: Path,
    conflicts_by_sig: Dict[Tuple[int, ...], List[Tuple[InstPat, InstPat]]],
    major_tables: Dict[str, Dict],
    holes16: int,
    multi16: int,
    multi16_examples: List[Tuple[int, List[str]]],
    hole16_ranges: List[Tuple[int, int]],
    prefix_conflicts: List[Tuple[InstPat, InstPat]],
) -> None:
    def fmt_ranges(indices: List[int], *, width_bits: int) -> str:
        """
        Format a sorted list of small indices (e.g. major opcode slots) as
        compact hex ranges like `0x00..0x03, 0x05, 0x07..0x09`.
        """
        if not indices:
            return ""
        # Defensive: ensure sorted unique.
        xs = sorted(set(indices))
        parts: List[str] = []
        i = 0
        while i < len(xs):
            start = xs[i]
            end = start
            while i + 1 < len(xs) and xs[i + 1] == end + 1:
                i += 1
                end = xs[i]
            if start == end:
                parts.append(f"0x{start:0{width_bits // 4}x}")
            else:
                parts.append(f"0x{start:0{width_bits // 4}x}..0x{end:0{width_bits // 4}x}")
            i += 1
        return ", ".join(parts)

    lines: List[str] = []
    lines.append("# LinxISA Encoding Space Report\n")
    lines.append(f"Spec: `{_display_path(spec_path)}`\n")

    lines.append("## Conflicts (part0 mask/match overlaps)\n")
    lines.append("Conflicts are reported at the **full instruction length** (all parts), not just part0.\n\n")
    any_conf = False
    for sig in sorted(conflicts_by_sig.keys(), key=lambda t: (len(t), t)):
        conf = conflicts_by_sig[sig]
        sig_name = " + ".join(str(x) for x in sig)
        lines.append(f"### parts: {sig_name}\n")
        if not conf:
            lines.append("- none\n")
            continue
        any_conf = True
        lines.append(f"- count: {len(conf)}\n")
        for a, b in conf[:50]:
            lines.append(
                f"- overlap: `{a.mnemonic}` ({a.inst_id}) vs `{b.mnemonic}` ({b.inst_id})\n"
            )
        if len(conf) > 50:
            lines.append(f"- ... and {len(conf) - 50} more\n")

    lines.append("\n## Prefix Ambiguity (multi-part prefix vs 32-bit single)\n")
    if not prefix_conflicts:
        lines.append("- none\n")
    else:
        any_conf = True
        lines.append(f"- count: {len(prefix_conflicts)}\n")
        for a, b in prefix_conflicts[:50]:
            lines.append(f"- overlap: `{a.mnemonic}` ({a.inst_id}) vs `{b.mnemonic}` ({b.inst_id})\n")

    lines.append("\n## Major Opcode Occupancy (summary)\n")
    for name, tbl in major_tables.items():
        width_bits = tbl["width_bits"]
        bits_desc = tbl["bits_desc"]
        covered = tbl["covered"]
        total = tbl["total"]
        partial = tbl["partial"]
        used_slots = tbl.get("used_slots", [])
        unused_slots = tbl.get("unused_slots", [])
        # Slot indices are a packed, synthetic index. For readability, format as ranges.
        slot_hex_width = 2 if total <= 0x100 else 4
        lines.append(f"\n### {name}\n")
        lines.append(f"- width_bits: {width_bits}\n")
        lines.append(f"- major bits: {bits_desc}\n")
        lines.append(f"- covered slots: {covered}/{total}\n")
        if partial:
            lines.append(f"- patterns not fully constraining major bits: {partial}\n")
        if used_slots:
            lines.append(f"- used slot indices: {fmt_ranges(used_slots, width_bits=slot_hex_width * 4)}\n")
        if unused_slots:
            lines.append(f"- unused slot indices: {fmt_ranges(unused_slots, width_bits=slot_hex_width * 4)}\n")

    lines.append("\n## 16-bit Exhaustive Coverage\n")
    lines.append(f"- unmatched encodings (holes): {holes16}\n")
    lines.append(f"- multiply-matched encodings (conflicts): {multi16}\n")
    if hole16_ranges:
        lines.append(f"- hole ranges: {len(hole16_ranges)}\n")
        # Show a small summary of the largest gaps.
        largest = sorted(hole16_ranges, key=lambda r: (r[1] - r[0], r[0]), reverse=True)[:25]
        lines.append("\n### Largest 16-bit hole ranges\n")
        for lo, hi in largest:
            n = hi - lo + 1
            lines.append(f"- `0x{lo:04x}..0x{hi:04x}` (count={n})\n")
    if multi16_examples:
        lines.append("\n### 16-bit conflict examples\n")
        for val, hits in multi16_examples[:25]:
            lines.append(f"- `0x{val:04x}` matches: {', '.join(hits)}\n")

    if any_conf or multi16:
        lines.append("\n## Status\n")
        lines.append("- FAIL: encoding conflicts detected\n")
    else:
        lines.append("\n## Status\n")
        lines.append("- OK: no encoding conflicts detected\n")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("".join(lines), encoding="utf-8")


def main(argv: List[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Report encoding space usage and detect conflicts.")
    ap.add_argument("--spec", type=Path, default=Path("isa/v0.3/linxisa-v0.3.json"))
    ap.add_argument("--out", type=Path, default=Path("docs/reference/encoding_space_report.md"))
    ap.add_argument(
        "--check",
        action="store_true",
        help="Exit non-zero if any encoding conflicts are found.",
    )
    args = ap.parse_args(argv)

    pats = _extract_patterns(args.spec)
    insts = _build_instructions(pats)
    conflicts_by_sig = _conflicts_by_signature(insts)

    # Prefix ambiguity: if a multi-part instruction begins with a 32-bit prefix part,
    # ensure no 32-bit single-part instruction overlaps that prefix encoding.
    prefix_conflicts: List[Tuple[InstPat, InstPat]] = []
    single32 = [it for it in insts if len(it.parts) == 1 and it.parts[0].width_bits == 32]
    multiprefix = [it for it in insts if len(it.parts) > 1 and it.parts[0].width_bits == 32]
    for mp in multiprefix:
        for sp in single32:
            if _parts_overlap(mp.parts[0], sp.parts[0]):
                prefix_conflicts.append((mp, sp))

    # Major-bit occupancy summaries (these are descriptive, not a proof of coverage).
    major_tables: Dict[str, Dict] = {}

    # 16-bit compressed: low bits [5:0] are used heavily as a major decode key.
    mask16 = _mask_for_bits(_bits_msb_to_lsb(5, 0))
    occ16, partial16 = _major_occupancy(pats, width_bits=16, major_mask=mask16)
    major_tables["C16-major[5:0]"] = {
        "width_bits": 16,
        "bits_desc": "[5:0] (6b)",
        "covered": len(occ16),
        "total": 1 << 6,
        "partial": len(partial16),
    }

    # 32-bit scalar: low bits [6:0] act as a major opcode key in the current catalog.
    mask32 = _mask_for_bits(_bits_msb_to_lsb(6, 0))
    occ32, partial32 = _major_occupancy(pats, width_bits=32, major_mask=mask32)
    major_tables["LX32-major[6:0]"] = {
        "width_bits": 32,
        "bits_desc": "[6:0] (7b)",
        "covered": len(occ32),
        "total": 1 << 7,
        "partial": len(partial32),
    }

    # 48-bit HL: use bits [19:17] + [16] + [3:0] as a stable "major" signature for the current opcode database.
    mask48 = _mask_for_bits(_bits_msb_to_lsb(19, 17) + [16] + _bits_msb_to_lsb(3, 0))
    occ48, partial48 = _major_occupancy(pats, width_bits=48, major_mask=mask48)
    major_tables["HL48-major[19:17,16,3:0]"] = {
        "width_bits": 48,
        "bits_desc": "[19:17],[16],[3:0] (8b)",
        "covered": len(occ48),
        "total": 1 << 8,
        "partial": len(partial48),
    }

    holes16, multi16, multi16_examples, hole16_ranges = _bruteforce_16bit(pats)

    # Augment major tables with concrete used/unused slot indices.
    def add_slot_lists(name: str, occ: Dict[int, List[PartPat]], bit_positions: List[int], total: int) -> None:
        used = sorted({_masked_to_index(v, bit_positions) for v in occ.keys()})
        used_set = set(used)
        unused = [i for i in range(total) if i not in used_set]
        major_tables[name]["used_slots"] = used
        major_tables[name]["unused_slots"] = unused

    add_slot_lists("C16-major[5:0]", occ16, list(range(0, 6)), 1 << 6)
    add_slot_lists("LX32-major[6:0]", occ32, list(range(0, 7)), 1 << 7)
    # For HL48-major use the packed order [3:0],[16],[19:17] => LSB..MSB positions.
    add_slot_lists("HL48-major[19:17,16,3:0]", occ48, [0, 1, 2, 3, 16, 17, 18, 19], 1 << 8)

    _write_report(
        args.out,
        spec_path=args.spec,
        conflicts_by_sig=conflicts_by_sig,
        major_tables=major_tables,
        holes16=holes16,
        multi16=multi16,
        multi16_examples=multi16_examples,
        hole16_ranges=hole16_ranges,
        prefix_conflicts=prefix_conflicts,
    )

    has_conf = any(conflicts_by_sig[s] for s in conflicts_by_sig) or (multi16 != 0) or bool(prefix_conflicts)
    if args.check and has_conf:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
