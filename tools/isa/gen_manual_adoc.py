#!/usr/bin/env python3
"""
Generate AsciiDoc fragments for the Linx ISA manual from a compiled LinxISA JSON spec
(`spec/isa/spec/current/linxisa-v*.json`).

This tool is intentionally lightweight: it emits tables and summaries that are easier to keep
in sync with the canonical machine-readable catalog than handwritten listings.

Outputs (into an output directory, typically `docs/architecture/isa-manual/src/generated/`):
  - registers_reg5.adoc
  - instruction_group_summary.adoc
  - mnemonic_index.adoc
  - instruction_reference.adoc
  - instruction_details.adoc
"""

from __future__ import annotations

import argparse
import json
import os
import re
import tempfile
from collections import Counter, OrderedDict
from typing import Any, Dict, Iterable, List, Optional, Tuple

def _read_json(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _mkdirp(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def _collapse_ws(s: str) -> str:
    return re.sub(r"\s+", " ", s).strip()


def _has_non_ascii(s: str) -> bool:
    return any(ord(ch) > 127 for ch in s)


def _normalize_asm(s: str) -> str:
    # The draft tables sometimes render arrows as "- >" because of column spacing.
    s = re.sub(r"\s*-\s*>\s*", " ->", s)
    return _collapse_ws(s)


_BITFIELD_MNEMONICS_M_IMMS_N_IMMLP1 = {
    "BXS",
    "BXU",
    "BIC",
    "BIS",
    "CTZ",
    "CLZ",
    "BCNT",
    "V.BXS",
    "V.BXU",
    "V.BIC",
    "V.BIS",
    "V.CTZ",
    "V.CLZ",
    "V.BCNT",
}


def _fixup_asm_for_docs(mnemonic: str, asm: str) -> str:
    """
    Adjust selected catalog assembly templates for readability in the manual.

    For bitfield-style operations that conceptually take `(M=start bit, N=bit-count)` but encode the
    operands as `(imml=N-1, imms=M)`, use explicit placeholder names so the assembly form matches the
    operand order shown by the toolchain.
    """
    if not asm:
        return asm
    if mnemonic in _BITFIELD_MNEMONICS_M_IMMS_N_IMMLP1:
        asm = re.sub(r",\s*M\s*,\s*N\s*,", ", Nminus1, M,", asm)
    return asm


def _escape_table_cell(s: str) -> str:
    # In AsciiDoc tables, "|" has special meaning. Escape it inside cell text.
    s = s.replace("|", r"\|")
    return s


def _decode_tag(inst: Dict[str, Any]) -> str:
    parts: List[Dict[str, Any]] = list(inst.get("parts", []))
    tags: List[str] = []
    for p in parts:
        t = str(p.get("decode") or "").strip()
        if t and t not in tags:
            tags.append(t)
    if not tags:
        return "-"
    return " + ".join(tags) if len(tags) > 1 else tags[0]


def _group_instructions(instructions: List[Dict[str, Any]]) -> "OrderedDict[str, List[Dict[str, Any]]]":
    """
    Group instructions by `group`, preserving:
      - group first-seen order (matches catalog order)
      - instruction order within each group (matches catalog order)
    """
    out: "OrderedDict[str, List[Dict[str, Any]]]" = OrderedDict()
    for inst in instructions:
        g = str(inst.get("group") or "").strip()
        if not g:
            g = "Ungrouped"
        out.setdefault(g, []).append(inst)
    return out


def _filter_canonical_instructions(
    instructions: List[Dict[str, Any]], spec_version: str
) -> List[Dict[str, Any]]:
    out = list(instructions)
    if spec_version.startswith("0.3"):
        out = [inst for inst in out if str(inst.get("mnemonic") or "").strip() != "BSTART.PAR"]
    return out


def _format_expr(expr: str) -> str:
    expr = _collapse_ws(expr)
    # Normalize common operators/spaces.
    expr = expr.replace("<<", " << ")
    expr = expr.replace("+", " + ")
    expr = expr.replace("-", " - ")
    expr = expr.replace("*", " * ")
    expr = re.sub(r"\s*=\s*", " = ", expr)
    expr = _collapse_ws(expr)
    # Parenthesize shift terms when used as addends (purely cosmetic).
    expr = re.sub(r"\+\s*([A-Za-z0-9_]+(?:\s*<<\s*[A-Za-z0-9_]+))", r"+ (\1)", expr)
    return expr


def _translate_note(note: str) -> str:
    note = _collapse_ws(note)
    if not note or note == "-":
        return ""
    if note.startswith("BNextOffset"):
        return _format_expr(note)
    if note.startswith("Ra =") or note.startswith("ra ="):
        out = note.replace("Ra", "ra")
        out = out.replace("tpc", "TPC").replace("pc", "PC")
        return _format_expr(out)
    if note.startswith("RegDst") or note.startswith("LB"):
        return _format_expr(note)
    return note


def _collect_part_explains(insts: List[Dict[str, Any]]) -> List[str]:
    out: List[str] = []
    seen: set[str] = set()
    for inst in insts:
        for p in inst.get("parts", []):
            ex = str(p.get("explain") or "").strip()
            ex = _collapse_ws(ex) if ex else ""
            if not ex:
                continue
            if ex in seen:
                continue
            seen.add(ex)
            out.append(ex)
    return out


def _collect_notes(insts: List[Dict[str, Any]]) -> List[str]:
    out: List[str] = []
    seen: set[str] = set()
    for inst in insts:
        note = str(inst.get("note") or "").strip()
        note = _translate_note(note)
        note = _collapse_ws(note) if note else ""
        if not note:
            continue
        if note in seen:
            continue
        seen.add(note)
        out.append(note)
    return out


def _anchorize(s: str) -> str:
    s = s.strip().lower()
    s = re.sub(r"[^a-z0-9]+", "-", s).strip("-")
    return s or "x"


def _mnemonic_core(mnemonic: str) -> Tuple[str, str, List[str]]:
    """
    Split a catalog mnemonic into:
      - encoding prefix: one of {"", "C", "HL", "V"}
      - core mnemonic string (without encoding prefix)
      - dot-separated core parts (space is treated as a dot)
    """
    m = mnemonic.strip()
    enc = ""
    for p in ("C.", "HL.", "V."):
        if m.startswith(p):
            enc = p[:-1]
            m = m[len(p) :]
            break
    core = m.strip()
    core_norm = core.replace(" ", ".")
    parts = [p for p in core_norm.split(".") if p]
    return enc, core, parts


def _addr_mode_from_group(group: str) -> str:
    g = group.strip()
    if "Register Offset" in g:
        return "register-offset addressing"
    if "Immediate Offset" in g:
        return "immediate-offset addressing"
    if "UnScaled" in g:
        return "unscaled immediate-offset addressing"
    if "Long Offset" in g:
        return "extended immediate-offset addressing"
    if "PC-Relative" in g or "Symbol" in g:
        return "PC-relative/symbol addressing"
    if "Pre-Index" in g:
        return "pre-index addressing with writeback"
    if "Post-Index" in g:
        return "post-index addressing with writeback"
    if "Pair" in g:
        return "paired access (two values) with the addressing form shown in the syntax"
    return "the addressing form shown in the syntax"


def _infer_mem_width_signed(core: str, is_load: bool) -> Optional[Tuple[int, str]]:
    """
    Infer access width and signedness from a load/store mnemonic core.
    Returns (width_bits, signedness) where signedness is {"signed","unsigned","none"}.
    """
    c = core.strip().lower()
    if c.startswith("prf"):
        return None

    if is_load:
        if c.startswith("lbu"):
            return (8, "unsigned")
        if c.startswith("lhu"):
            return (16, "unsigned")
        if c.startswith("lwu"):
            return (32, "unsigned")
        if c.startswith("lb"):
            return (8, "signed")
        if c.startswith("lh"):
            return (16, "signed")
        if c.startswith("lw"):
            return (32, "signed")
        if c.startswith("ld"):
            return (64, "none")
        return None

    # Stores: signedness is not applicable.
    if c.startswith("sb"):
        return (8, "none")
    if c.startswith("sh"):
        return (16, "none")
    if c.startswith("sw"):
        return (32, "none")
    if c.startswith("sd"):
        return (64, "none")
    return None


def _cond_from_suffix(suffix: str) -> Optional[str]:
    s = suffix.strip().upper()
    if not s:
        return None
    cond_map = {
        "EQ": "equal",
        "NE": "not equal",
        "LT": "less-than (signed)",
        "GE": "greater-or-equal (signed)",
        "LTU": "less-than (unsigned)",
        "GEU": "greater-or-equal (unsigned)",
        "Z": "zero",
        "NZ": "non-zero",
    }
    return cond_map.get(s)


def _atomic_op_from_suffix(suffix: str) -> Optional[str]:
    s = suffix.strip().upper()
    if not s:
        return None
    op_map = {
        "ADD": "+",
        "AND": "&",
        "OR": "|",
        "XOR": "^",
        "SMAX": "smax",
        "SMIN": "smin",
        "UMAX": "umax",
        "UMIN": "umin",
        "MAX": "max",
        "MIN": "min",
    }
    return op_map.get(s)


def _note_rhs(note: str, lhs: str) -> Optional[str]:
    m = re.match(rf"^{re.escape(lhs)}\s*=\s*(.+)$", note.strip())
    return m.group(1).strip() if m else None


def _describe_mnemonic(group: str, mnemonic: str, asm_forms: List[str]) -> str:
    m = mnemonic.strip()
    g = group.strip()

    enc, core, parts = _mnemonic_core(m)

    prefix = ""
    if enc == "C":
        prefix = "This mnemonic has a 16-bit compressed encoding."
    elif enc == "HL":
        prefix = "This mnemonic has a 48-bit composed encoding (prefix + main)."
    elif enc == "V":
        prefix = "This mnemonic has a 64-bit vector encoding (prefix + main)."

    root = parts[0].upper() if parts else core.upper()
    sub = parts[1].upper() if len(parts) > 1 else ""

    # High-signal explicit descriptions for key architectural instructions.
    if root == "BSTOP":
        desc = "Terminates the current basic block."
    elif root == "BSTART":
        desc = "Terminates the current block and begins the next block, encoding both block type and transition kind."
    elif root == "SETC":
        desc = "Sets the block-commit condition/argument consumed by subsequent conditional block transitions."
    elif root in {"MOVR", "MOVI", "MOV"}:
        desc = "Moves an immediate or register value into the selected destination (GPR or queue push)."
    elif root == "SETRET":
        desc = "Materializes a return address into `ra` using a PC-relative offset."
    elif root in {"FENTRY", "FEXIT", "FRET", "FRET.RA", "FRET.STK"} or m.startswith("FRET"):
        desc = "Template-style function prologue/epilogue instruction for saving/restoring register ranges and adjusting the stack."
    elif root in {"MCOPY", "MSET"}:
        desc = "Performs a bulk memory operation as specified by the operands (copy or set)."
    elif root == "B" and sub and g.startswith("Block"):
        if sub == "TEXT":
            desc = (
                "In a decoupled block header, selects the out-of-line body entrypoint (`BodyTPC`) "
                "for header→body→return execution. In coupled blocks, may be used as a tooling label/annotation."
            )
        elif sub in {"IOR", "IOD", "IOT", "IOTI"}:
            desc = (
                "Declares block input/output bindings for registers/tiles (header metadata for accelerated blocks; "
                "tooling and scheduling metadata)."
            )
        elif sub == "ATTR":
            desc = "Sets block attributes (e.g., ordering/atomic qualifiers such as aq/rl) as encoded by the operands."
        elif sub == "HINT":
            desc = "Sets block execution hints (e.g., branch likelihood, temperature, prefetch sizing)."
        elif sub == "DIM":
            desc = "Sets a block loop bound/dimension register for structured execution."
        else:
            desc = "Block metadata instruction (sub-operation selected by the mnemonic suffix)."
    elif g == "Branch":
        if root == "J":
            desc = "Unconditional PC-relative jump to `label`."
        elif root == "JR":
            desc = "Jump to an address formed from a base register plus an immediate offset."
        elif root == "B":
            cond = _cond_from_suffix(sub)
            if cond:
                desc = f"Conditional branch. The branch is taken when the selected condition holds ({cond})."
            else:
                desc = "Conditional branch. The branch condition is selected by the mnemonic."
        else:
            desc = "Control-transfer instruction. The control-transfer kind is selected by the mnemonic."
    elif g == "Cache Maintain" or root in {"IC", "DC", "TLB", "BC"}:
        op = sub or (parts[0].upper() if len(parts) == 1 else "")
        if root == "IC":
            desc = (
                "Instruction-cache maintenance operation (invalidate by address or invalidate all), selected by the mnemonic suffix."
            )
        elif root == "DC":
            desc = (
                "Data-cache maintenance operation (invalidate/clean/zero, by address or by set/way), selected by the mnemonic suffix."
            )
        elif root == "TLB":
            desc = "TLB maintenance operation, selected by the mnemonic suffix."
        elif root == "BC":
            desc = "Branch cache/predictor maintenance operation, selected by the mnemonic suffix."
        else:
            desc = "Cache/TLB maintenance operation, selected by the mnemonic suffix."
    elif root in {"EBREAK", "ASSERT", "FENCE"}:
        desc = "Execution-control instruction used for debugging, ordering, or bring-up."
    elif root in {"BSE", "BWE", "BWI", "BWT"}:
        if root == "BWT":
            desc = "Requests the processor to sleep for a duration specified by the operand."
        else:
            desc = "Execution-control wait/event primitive (exact behavior is platform-defined)."
    elif root in {"ACRC", "ACRE"}:
        desc = "Architectural control operation (sub-operation selected by the operand encoding)."
    elif root in {"SSRGET", "SSRSET", "SSRSWAP", "LSRGET"} or (enc == "HL" and root in {"SSRGET", "SSRSET"}):
        if root.endswith("GET"):
            desc = "Reads a system register identified by the ID operand and writes the value to the selected destination."
        elif root.endswith("SET"):
            desc = "Writes a system register identified by the ID operand with the provided value."
        elif root.endswith("SWAP"):
            desc = "Atomically swaps a system register value with the provided value."
        else:
            desc = "System register access instruction."
    elif root in {
        "ADD",
        "SUB",
        "AND",
        "OR",
        "XOR",
        "SLL",
        "SRL",
        "SRA",
        "ADDI",
        "SUBI",
        "ANDI",
        "ORI",
        "XORI",
        "SLLI",
        "SRLI",
        "SRAI",
        "ADDW",
        "SUBW",
        "ANDW",
        "ORW",
        "XORW",
        "SLLW",
        "SRLW",
        "SRAW",
        "ADDIW",
        "SUBIW",
        "ANDIW",
        "ORIW",
        "XORIW",
        "SLLIW",
        "SRLIW",
        "SRAIW",
        "MUL",
        "MULU",
        "MULW",
        "MULUW",
        "MADD",
        "MADDW",
        "DIV",
        "DIVU",
        "DIVW",
        "DIVUW",
        "REM",
        "REMU",
        "REMW",
        "REMUW",
        "CTZ",
        "CLZ",
        "BCNT",
        "REV",
        "BXS",
        "BXU",
        "BIC",
        "BIS",
        "BFI",
        "CSEL",
        "LUI",
        "ADDTPC",
    }:
        arith = {
            "ADD": "Integer addition.",
            "SUB": "Integer subtraction.",
            "AND": "Bitwise AND.",
            "OR": "Bitwise OR.",
            "XOR": "Bitwise XOR.",
            "SLL": "Logical left shift.",
            "SRL": "Logical right shift.",
            "SRA": "Arithmetic right shift.",
            "ADDI": "Integer add-immediate.",
            "SUBI": "Integer subtract-immediate.",
            "ANDI": "Bitwise AND (immediate).",
            "ORI": "Bitwise OR (immediate).",
            "XORI": "Bitwise XOR (immediate).",
            "SLLI": "Logical left shift (immediate).",
            "SRLI": "Logical right shift (immediate).",
            "SRAI": "Arithmetic right shift (immediate).",
            "ADDW": "Word (32-bit) integer addition.",
            "SUBW": "Word (32-bit) integer subtraction.",
            "ANDW": "Word (32-bit) bitwise AND.",
            "ORW": "Word (32-bit) bitwise OR.",
            "XORW": "Word (32-bit) bitwise XOR.",
            "SLLW": "Word (32-bit) logical left shift.",
            "SRLW": "Word (32-bit) logical right shift.",
            "SRAW": "Word (32-bit) arithmetic right shift.",
            "ADDIW": "Word (32-bit) add-immediate.",
            "SUBIW": "Word (32-bit) subtract-immediate.",
            "ANDIW": "Word (32-bit) AND-immediate.",
            "ORIW": "Word (32-bit) OR-immediate.",
            "XORIW": "Word (32-bit) XOR-immediate.",
            "SLLIW": "Word (32-bit) logical left shift (immediate).",
            "SRLIW": "Word (32-bit) logical right shift (immediate).",
            "SRAIW": "Word (32-bit) arithmetic right shift (immediate).",
            "MUL": "Integer multiply.",
            "MULU": "Integer multiply (unsigned).",
            "MULW": "Word (32-bit) integer multiply.",
            "MULUW": "Word (32-bit) integer multiply (unsigned).",
            "MADD": "Multiply-add (three-source).",
            "MADDW": "Word (32-bit) multiply-add (three-source).",
            "DIV": "Integer division (signed).",
            "DIVU": "Integer division (unsigned).",
            "DIVW": "Word (32-bit) integer division (signed).",
            "DIVUW": "Word (32-bit) integer division (unsigned).",
            "REM": "Integer remainder (signed).",
            "REMU": "Integer remainder (unsigned).",
            "REMW": "Word (32-bit) integer remainder (signed).",
            "REMUW": "Word (32-bit) integer remainder (unsigned).",
            "CTZ": "Count trailing zeros.",
            "CLZ": "Count leading zeros.",
            "BCNT": "Population count (count set bits).",
            "REV": "Bit-reversal operation.",
            "BXS": "Bit-field extract (signed).",
            "BXU": "Bit-field extract (unsigned).",
            "BIC": "Bit clear / AND-NOT operation.",
            "BIS": "Bit set / OR operation.",
            "BFI": "Bit-field insert.",
            "CSEL": "Conditional select.",
            "LUI": "Load upper immediate (constant materialization).",
            "ADDTPC": "PC-relative addition (adds to the current PC/TPC).",
        }
        desc = arith.get(root, "Integer/bit-manipulation operation.")
    elif "Atomic" in g or root in {"LR", "SC"} or root.startswith("SWAP") or (_atomic_op_from_suffix(sub) and root in {"LD", "LW", "SD", "SW"}):
        if root == "LR":
            desc = "Load-reserved: performs an atomic load and establishes a reservation for a subsequent store-conditional."
        elif root == "SC":
            desc = "Store-conditional: attempts an atomic store that succeeds only if the reservation is still valid; returns a status code."
        elif root.startswith("SWAP"):
            desc = "Atomic swap: atomically exchanges a memory value with a register value and returns the old value."
        elif root in {"LD", "LW"} and sub:
            desc = "Atomic read-modify-write operation that returns the original memory value."
        elif root in {"SD", "SW"} and sub:
            desc = "Atomic read-modify-write operation that updates memory without returning a value."
        else:
            desc = "Atomic memory operation."
    elif root in {"PRF", "PRFI"} or root.startswith("PRF"):
        desc = "Issues a prefetch request for the address computed by the selected addressing form."
    elif any("Load" in g2 for g2 in (g,)) or any("Store" in g2 for g2 in (g,)) or any("[" in a for a in asm_forms):
        # Try to infer a more specific load/store description when the mnemonic name follows common width/signedness
        # conventions (e.g. `lb`, `lbu`, `sw`, `sd.u`, ...).
        is_store = ("Store" in g) or root.startswith("S")
        if ("Load" in g) and ("Store" in g):
            is_store = root.startswith("S")

        mem_info = _infer_mem_width_signed(core, is_load=not is_store)
        addr_mode = _addr_mode_from_group(g)
        # Many memory families use a `.U`/`U*` suffix to indicate *unscaled* addressing.
        if any(p.upper().startswith("U") for p in parts[1:]) and "unscaled" not in addr_mode:
            addr_mode = f"unscaled {addr_mode}"
        if mem_info:
            width_bits, signedness = mem_info
            if is_store:
                desc = f"Stores a {width_bits}-bit value to memory using {addr_mode}."
            else:
                if signedness == "signed":
                    desc = f"Loads a signed {width_bits}-bit value from memory using {addr_mode} and writes the result to the selected destination."
                elif signedness == "unsigned":
                    desc = f"Loads an unsigned {width_bits}-bit value from memory using {addr_mode} and writes the result to the selected destination."
                else:
                    desc = f"Loads a {width_bits}-bit value from memory using {addr_mode} and writes the result to the selected destination."
        else:
            desc = (
                f"Stores a value to memory using {addr_mode}."
                if is_store
                else f"Loads a value from memory using {addr_mode} and writes the result to the selected destination."
            )
    elif "Compare" in g or root.startswith("CMP") or root in {"FEQ", "FNE", "FLT", "FGE"}:
        desc = "Performs a comparison as selected by the mnemonic and writes a boolean result to the selected destination."
    elif "Floating" in g or root.startswith("F"):
        desc = "Performs a floating-point operation as selected by the mnemonic and operand type qualifiers."
    else:
        desc = (
            "Performs the operation indicated by the mnemonic on the operands specified by the selected form. "
            "Operand-size, signedness, and addressing modifiers are encoded via suffixes and qualifiers in the assembly syntax."
        )

    if prefix:
        return f"{prefix} {desc}"
    return desc


def _infer_operation_pseudocode(group: str, mnemonic: str, asm_forms: List[str], notes: List[str]) -> Optional[List[str]]:
    """
    Produce a short, informative pseudocode block for a mnemonic. This is intentionally schematic: it is derived from
    mnemonic naming conventions + the catalog's assembly templates and notes.
    """
    g = group.strip()
    m = mnemonic.strip()

    enc, core, parts = _mnemonic_core(m)
    root = parts[0].upper() if parts else core.upper()
    sub = parts[1].upper() if len(parts) > 1 else ""

    addr_rhs = next((_note_rhs(n, "Address") for n in notes if _note_rhs(n, "Address")), None)
    br_rhs = next((_note_rhs(n, "Branch target") for n in notes if _note_rhs(n, "Branch target")), None)
    ra_rhs = next((_note_rhs(n, "ra") for n in notes if _note_rhs(n, "ra")), None)
    regdst_rhs = next((_note_rhs(n, "RegDst") for n in notes if _note_rhs(n, "RegDst")), None)
    lb0_rhs = next((_note_rhs(n, "LB0") for n in notes if _note_rhs(n, "LB0")), None)
    lb1_rhs = next((_note_rhs(n, "LB1") for n in notes if _note_rhs(n, "LB1")), None)
    lb2_rhs = next((_note_rhs(n, "LB2") for n in notes if _note_rhs(n, "LB2")), None)

    # PC-relative / return-address materialization.
    if ra_rhs:
        return [f"ra = {ra_rhs}"]
    if regdst_rhs and root in {"ADDTPC", "HL.ADDTPC"}:
        return [f"RegDst = {regdst_rhs}"]

    # Branches and jumps.
    if g == "Branch":
        target = br_rhs or "<target>"
        if root == "J":
            return [f"TPC = {target}"]
        if root == "JR":
            return [f"TPC = {target}"]
        if root == "B":
            cond = _cond_from_suffix(sub)
            if cond:
                has_src_operands = any(("SrcL" in a or "SrcR" in a) for a in asm_forms)
                pred = f"branch condition holds ({cond})"
                if has_src_operands and sub in {"EQ", "NE"}:
                    pred = "Read(SrcL) == Read(SrcR)" if sub == "EQ" else "Read(SrcL) != Read(SrcR)"
                elif has_src_operands and sub in {"LT", "GE"}:
                    pred = "Read(SrcL) < Read(SrcR)  // signed" if sub == "LT" else "Read(SrcL) >= Read(SrcR)  // signed"
                elif has_src_operands and sub in {"LTU", "GEU"}:
                    pred = "Read(SrcL) < Read(SrcR)  // unsigned" if sub == "LTU" else "Read(SrcL) >= Read(SrcR)  // unsigned"
                return [f"if ({pred}):", f"  TPC = {target}"]
            return [f"if (branch condition holds):", f"  TPC = {target}"]

    # Block markers.
    if root == "BSTOP":
        return ["EndBlock()"]
    if root == "BSTART":
        return ["EndBlock()", "BeginNextBlock(/* type/kind/target as encoded */)"]

    # Block metadata.
    if root == "B" and sub and g.startswith("Block"):
        if sub == "DIM":
            lines: List[str] = []
            if lb0_rhs:
                lines.append(f"LB0 = {lb0_rhs}")
            if lb1_rhs:
                lines.append(f"LB1 = {lb1_rhs}")
            if lb2_rhs:
                lines.append(f"LB2 = {lb2_rhs}")
            if lines:
                return lines
            return ["LBx = Read(RegSrc) + uimm  // x selected by encoding (LB0/LB1/LB2)"]
        if sub == "IOR":
            return ["DeclareBlockGprIO(/* GPR IO bindings as encoded */)"]
        if sub == "IOD":
            return ["DeclareBlockDependencies(/* dependency bindings as encoded */)"]
        if sub in {"IOT", "IOTI"}:
            return ["DeclareBlockTileIO(/* tile IO bindings as encoded */)"]
        if sub == "ATTR":
            return ["SetBlockAttributes(/* fields as encoded */)"]
        if sub == "HINT":
            return ["SetBlockHints(/* fields as encoded */)"]
        if sub == "TEXT":
            return ["AnnotateBlockText(/* label/offset as encoded */)"]
        return ["UpdateBlockMetadata(/* fields as encoded */)"]

    # Template instructions.
    if root in {"FENTRY", "FEXIT"} or m.startswith("FRET"):
        return ["AdjustSPAndSaveRestoreRange(/* register range + uimm */)"]

    # Bulk memory ops.
    if root == "MCOPY":
        return ["MemCopy(dst=RegSrc0, src=RegSrc1, size=RegSrc2)"]
    if root == "MSET":
        return ["MemSet(dst=RegSrc0, value=RegSrc1, size=RegSrc2)"]

    # System register access.
    if root in {"SSRGET", "HL.SSRGET", "LSRGET"} or root.endswith("SSRGET"):
        return ["value = ReadSystemRegister(ID)", "Write(Dst, value)"]
    if root in {"SSRSET", "HL.SSRSET"} or root.endswith("SSRSET"):
        return ["WriteSystemRegister(ID, Read(SrcL))"]
    if root == "SSRSWAP":
        return ["old = ReadSystemRegister(ID)", "WriteSystemRegister(ID, Read(SrcL))", "Write(Dst, old)"]
    if root == "SETC" and sub == "TGT":
        return ["SetCommitTarget(Read(SrcL))"]

    # Cache/TLB maintenance.
    if g == "Cache Maintain" or root in {"IC", "DC", "TLB", "BC"}:
        if any("SrcL" in a for a in asm_forms):
            return ["Maintain(op, address=Read(SrcL))"]
        return ["Maintain(op)"]

    # Execution-control wait/event primitives.
    if root == "BWT":
        return ["Sleep(duration=Read(SrcL))"]
    if root in {"BSE", "BWE", "BWI"}:
        return ["WaitEventOrInterrupt(Read(SrcL))"]
    if root == "ASSERT":
        return ["if (Read(SrcL) == 0):", "  Trap(ASSERT)"]
    if root == "EBREAK":
        return ["Trap(EBREAK)  // debugger break / environment break"]
    if root == "FENCE" and sub == "I":
        return ["FenceI()  // synchronize instruction fetch with prior stores"]
    if root == "FENCE" and sub == "D":
        return ["FenceD(pred_imm, succ_imm)  // memory ordering fence"]
    if root == "ACRE":
        return ["EnterACR(rra_type)  // privileged ring transition (implementation-defined)"]
    if root == "ACRC":
        return ["CallACR(rst_type)  // privileged ring call (implementation-defined)"]

    # Atomics.
    if "Atomic" in g or root in {"LR", "SC"} or root.startswith("SWAP") or (_atomic_op_from_suffix(sub) and root in {"LD", "LW", "SD", "SW"}):
        lane_prefix = ["for each active element:"] if enc == "V" else []
        if root == "LR":
            w_map = {"B": 8, "H": 16, "W": 32, "D": 64}
            w = w_map.get(sub, 0)
            return lane_prefix + [
                "addr = Read(SrcL)",
                f"value = LoadReserved{w} (addr)" if w else "value = LoadReserved(addr)",
                "Write(Dst, value)",
            ]
        if root == "SC":
            w_map = {"B": 8, "H": 16, "W": 32, "D": 64}
            w = w_map.get(sub, 0)
            return lane_prefix + [
                "addr = Read(SrcR)",
                f"status = StoreConditional{w}(addr, Read(SrcL))" if w else "status = StoreConditional(addr, Read(SrcL))",
                "Write(Dst, status)",
            ]
        if root.startswith("SWAP"):
            w_map = {"B": 8, "H": 16, "W": 32, "D": 64}
            w = w_map.get(root[-1].upper(), 0)
            return lane_prefix + [
                "addr = Read(SrcL)",
                f"old = AtomicSwap{w}(addr, Read(SrcR))" if w else "old = AtomicSwap(addr, Read(SrcR))",
                "Write(Dst, old)",
            ]
        if root in {"LD", "LW", "SD", "SW"} and sub:
            w = 64 if root in {"LD", "SD"} else 32
            op = _atomic_op_from_suffix(sub) or "op"
            lines = lane_prefix + [
                "addr = Read(SrcL)",
                f"old = AtomicLoad{w}(addr)",
            ]
            if op in {"+", "&", "|", "^"}:
                lines.append(f"new = old {op} Read(SrcR)")
            else:
                lines.append(f"new = {op}(old, Read(SrcR))")
            lines.append(f"AtomicStore{w}(addr, new)")
            if root in {"LD", "LW"}:
                lines.append("Write(Dst, old)")
            return lines

    # Loads/stores.
    if "Load" in g or "Store" in g:
        is_store = "Store" in g
        mem_info = _infer_mem_width_signed(core, is_load=not is_store)
        addr_expr = addr_rhs or "ComputeAddress(/* per syntax */)"
        store_data = "SrcD" if any("SrcD" in a for a in asm_forms) else "SrcL"

        if not mem_info:
            if is_store:
                return [f"addr = {addr_expr}", f"Store(addr, Read({store_data}))"]
            return [f"addr = {addr_expr}", "value = Load(addr)", "Write(Dst, value)"]

        width_bits, signedness = mem_info
        width_bytes = max(1, width_bits // 8)

        if "Pre-Index" in g:
            if is_store:
                return [
                    f"updated = {addr_expr}",
                    f"Store{width_bits}(updated, Read({store_data}))",
                    "Write(Dst, updated)  // when a destination is present",
                ]
            return [
                f"updated = {addr_expr}",
                f"value = Load{width_bits}(updated)",
                "Write(Dst0, value)",
                "Write(Dst1, updated)",
            ]
        if "Post-Index" in g:
            if is_store:
                return [
                    "base = Read(SrcL)",
                    f"Store{width_bits}(base, Read({store_data}))",
                    f"updated = {addr_expr}",
                    "Write(Dst, updated)  // when a destination is present",
                ]
            return [
                "base = Read(SrcL)",
                f"value = Load{width_bits}(base)",
                f"updated = {addr_expr}",
                "Write(Dst0, value)",
                "Write(Dst1, updated)",
            ]

        if "Pair" in g and not is_store:
            return [
                f"addr = {addr_expr}",
                f"val0 = Load{width_bits}(addr)",
                f"val1 = Load{width_bits}(addr + {width_bytes})",
                "Write(Dst0, val0)",
                "Write(Dst1, val1)",
            ]
        if "Pair" in g and is_store:
            return [
                f"addr = {addr_expr}",
                f"Store{width_bits}(addr, Read(SrcD))",
                f"Store{width_bits}(addr + {width_bytes}, Read(SrcD1))",
            ]

        if is_store:
            return [f"addr = {addr_expr}", f"Store{width_bits}(addr, Read({store_data}))"]

        # Regular (single-dest) loads.
        lines = [f"addr = {addr_expr}", f"value = Load{width_bits}(addr)"]
        if signedness == "signed" and width_bits < 64:
            lines.append("value = SignExtend(value)")
        elif signedness == "unsigned" and width_bits < 64:
            lines.append("value = ZeroExtend(value)")
        lines.append("Write(Dst, value)")
        return lines

    # Integer comparisons / condition setting.
    if root == "CMP" or root.startswith("CMP"):
        if sub in {"EQ", "NE", "LT", "GE", "LTU", "GEU"}:
            expr_map = {
                "EQ": "Read(SrcL) == Read(SrcR)",
                "NE": "Read(SrcL) != Read(SrcR)",
                "LT": "Read(SrcL) < Read(SrcR)  // signed",
                "GE": "Read(SrcL) >= Read(SrcR)  // signed",
                "LTU": "Read(SrcL) < Read(SrcR)  // unsigned",
                "GEU": "Read(SrcL) >= Read(SrcR)  // unsigned",
            }
            return [f"result = ({expr_map[sub]}) ? 1 : 0", "Write(Dst, result)"]
        if sub in {"EQI", "NEI", "LTI", "GEI", "LTUI", "GEUI"}:
            return ["result = CompareImmediate(/* per suffix */) ? 1 : 0", "Write(Dst, result)"]
        if sub in {"AND", "OR"}:
            return ["result = (SrcL AND/OR SrcR) ? 1 : 0", "Write(Dst, result)"]
        return ["result = Compare(/* per mnemonic */)", "Write(Dst, result)"]

    if root == "SETC" and sub and sub != "TGT":
        if sub in {"EQ", "NE", "LT", "GE", "LTU", "GEU"}:
            expr_map = {
                "EQ": "Read(SrcL) == Read(SrcR)",
                "NE": "Read(SrcL) != Read(SrcR)",
                "LT": "Read(SrcL) < Read(SrcR)  // signed",
                "GE": "Read(SrcL) >= Read(SrcR)  // signed",
                "LTU": "Read(SrcL) < Read(SrcR)  // unsigned",
                "GEU": "Read(SrcL) >= Read(SrcR)  // unsigned",
            }
            return [f"commit_arg = ({expr_map[sub]}) ? 1 : 0", "SetCommitArgument(commit_arg)"]
        return ["commit_arg = EvaluateCondition(/* per mnemonic */)", "SetCommitArgument(commit_arg)"]

    # Arithmetic / ALU fallbacks.
    if root in {"ADD", "SUB", "AND", "OR", "XOR", "SLL", "SRL", "SRA"}:
        op_map = {"ADD": "+", "SUB": "-", "AND": "&", "OR": "|", "XOR": "^", "SLL": "<<", "SRL": ">>", "SRA": ">>"}
        op = op_map[root]
        return [
            "lhs = Read(SrcL)",
            "rhs = Read(SrcR)  // apply modifiers/shift as encoded",
            f"result = lhs {op} rhs",
            "Write(Dst, result)",
        ]
    if root.endswith("I") and root[:-1] in {"ADD", "SUB", "AND", "OR", "XOR", "SLL", "SRL", "SRA"}:
        op_map = {"ADD": "+", "SUB": "-", "AND": "&", "OR": "|", "XOR": "^", "SLL": "<<", "SRL": ">>", "SRA": ">>"}
        op = op_map.get(root[:-1], "op")
        return [
            "lhs = Read(SrcL)",
            "rhs = imm  // immediate as encoded",
            f"result = lhs {op} rhs",
            "Write(Dst, result)",
        ]

    # Provide a generic schematic when we have at least one assembly form (so the page isn't empty).
    if asm_forms:
        return ["Execute operation as selected by the mnemonic and encoded modifiers."]

    return None


def _write_registers_reg5(spec: Dict[str, Any], out_path: str, source_comment: str) -> None:
    regs = spec.get("registers", {}).get("reg5", {})
    entries: List[Dict[str, Any]] = list(regs.get("entries", []))

    lines: List[str] = []
    lines.append("// Generated file; do not edit by hand.")
    lines.append(source_comment)
    gen_on = str(spec.get("generated_on") or "").strip()
    if gen_on:
        lines.append(f"// Catalog generated_on: {gen_on}")
    lines.append("")
    lines.append('[cols="1,1,2,4",options="header"]')
    lines.append("|===")
    lines.append("|Code |Draft name |Preferred asm |Accepted spellings")

    for e in sorted(entries, key=lambda x: int(x.get("code", 0))):
        code = int(e.get("code", 0))
        name = str(e.get("name", "")).strip()
        asm = str(e.get("asm", "")).strip()
        aliases = e.get("aliases") or []
        if isinstance(aliases, list):
            aliases_s = ", ".join(str(a) for a in aliases)
        else:
            aliases_s = str(aliases)

        lines.append(f"|{code}")
        lines.append(f"|`{_escape_table_cell(name)}`" if name else "|-")
        lines.append(f"|`{_escape_table_cell(asm)}`" if asm else "|-")
        lines.append(f"|{_escape_table_cell(aliases_s)}" if aliases_s else "|-")

    lines.append("|===")
    lines.append("")

    _mkdirp(os.path.dirname(out_path))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def _write_instruction_group_summary(groups: "OrderedDict[str, List[Dict[str, Any]]]", out_path: str) -> None:
    lines: List[str] = []
    lines.append("// Generated file; do not edit by hand.")
    lines.append("")
    lines.append("[[insnref-groups]]")
    lines.append("=== Instruction groups")
    lines.append("")
    lines.append('[cols="3,1,1",options="header"]')
    lines.append("|===")
    lines.append("|Group |Forms |Unique mnemonics")

    for group, insts in groups.items():
        forms = len(insts)
        uniq = len({str(i.get("mnemonic", "")).strip() for i in insts})
        lines.append(f"|{_escape_table_cell(group)} |{forms} |{uniq}")

    lines.append("|===")
    lines.append("")

    _mkdirp(os.path.dirname(out_path))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def _write_instruction_reference(groups: "OrderedDict[str, List[Dict[str, Any]]]", out_path: str) -> None:
    lines: List[str] = []
    lines.append("// Generated file; do not edit by hand.")
    lines.append("")

    used_anchors: set[str] = set()
    for group, insts in groups.items():
        base_anchor = re.sub(r"[^a-z0-9]+", "-", group.strip().lower()).strip("-")
        if not base_anchor:
            base_anchor = "group"
        anchor = base_anchor
        suffix = 2
        while anchor in used_anchors:
            anchor = f"{base_anchor}-{suffix}"
            suffix += 1
        used_anchors.add(anchor)
        lines.append(f"[[insnref-{anchor}]]")
        lines.append(f"=== {group}")
        lines.append("")
        lines.append('[cols="1,1,2,4,6",options="header"]')
        lines.append("|===")
        lines.append("|Mnemonic |Len |Decode |Assembly |Notes")

        for inst in insts:
            mnem = str(inst.get("mnemonic", "")).strip()
            asm = str(inst.get("asm") or "").strip()
            note = str(inst.get("note") or "").strip()
            length = int(inst.get("length_bits", 0))
            decode = _decode_tag(inst)

            asm = _fixup_asm_for_docs(mnem, asm)
            asm = _normalize_asm(asm) if asm else ""
            note = _translate_note(note)
            note = _collapse_ws(note) if note else ""

            mnem_cell = f"`{_escape_table_cell(mnem)}`" if mnem else "-"
            asm_cell = f"`{_escape_table_cell(asm)}`" if asm else "-"
            note_cell = _escape_table_cell(note) if note else "-"
            decode_cell = _escape_table_cell(decode)

            lines.append(f"|{mnem_cell} |{length} |{decode_cell} |{asm_cell} |{note_cell}")

        lines.append("|===")
        lines.append("")

    _mkdirp(os.path.dirname(out_path))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def _write_instruction_details(
    groups: "OrderedDict[str, List[Dict[str, Any]]]", out_path: str, spec_version: str
) -> None:
    lines: List[str] = []
    lines.append("// Generated file; do not edit by hand.")
    lines.append("")
    lines.append("[[insnref-details]]")
    lines.append("=== Instruction descriptions (by mnemonic)")
    lines.append("")
    lines.append(
        "This section provides per-mnemonic descriptions. The tables above remain the authoritative listing of encodable "
        f"forms and decode layouts in the v{spec_version} catalog. Descriptions and pseudocode here are derived from mnemonic naming "
        "conventions plus catalog assembly templates/notes and are intended to be *informative*."
    )
    lines.append("")

    used_group_anchors: set[str] = set()
    for group, insts in groups.items():
        base_group_anchor = _anchorize(group)
        group_anchor = base_group_anchor
        suffix = 2
        while group_anchor in used_group_anchors:
            group_anchor = f"{base_group_anchor}-{suffix}"
            suffix += 1
        used_group_anchors.add(group_anchor)

        # Group header (level 4 so it appears in the ToC; mnemonics are level 5 so they do not).
        lines.append(f"[[insndesc-{group_anchor}]]")
        lines.append(f"==== {group}")
        lines.append("")

        # Preserve mnemonic first-seen order within the group.
        m2: "OrderedDict[str, List[Dict[str, Any]]]" = OrderedDict()
        for inst in insts:
            m = str(inst.get("mnemonic") or "").strip()
            if not m:
                continue
            m2.setdefault(m, []).append(inst)

        for mnemonic, forms in m2.items():
            asm_forms: List[str] = []
            seen_asm: set[str] = set()
            lengths: List[int] = []
            decodes: List[str] = []
            for inst in forms:
                a = str(inst.get("asm") or "").strip()
                a = _fixup_asm_for_docs(mnemonic, a)
                a = _normalize_asm(a) if a else ""
                if a and a not in seen_asm:
                    seen_asm.add(a)
                    asm_forms.append(a)
                l = int(inst.get("length_bits", 0))
                if l and l not in lengths:
                    lengths.append(l)
                d = _decode_tag(inst)
                if d and d not in decodes:
                    decodes.append(d)

            lengths_s = ", ".join(str(x) for x in sorted(lengths)) if lengths else "?"
            forms_count = len(forms)

            expls = _collect_part_explains(forms)
            notes = _collect_notes(forms)
            op_lines = _infer_operation_pseudocode(group, mnemonic, asm_forms, notes)

            m_anchor = _anchorize(f"{group_anchor}-{mnemonic}")
            lines.append(f"[[insndesc-{m_anchor}]]")
            lines.append(f"===== {mnemonic}")
            lines.append("")
            lines.append(f"{_describe_mnemonic(group, mnemonic, asm_forms)}")
            lines.append("")
            lines.append(f"Catalog forms:: {forms_count} form(s) ({lengths_s} bits).")
            if decodes and any(d != "-" for d in decodes):
                dec_s = ", ".join(_escape_table_cell(d) for d in decodes if d and d != "-")
                if dec_s:
                    lines.append(f"Decode tags:: {dec_s}.")

            if expls:
                lines.append("")
                lines.append("Notes (informative)::")
                lines.append("+")
                for ex in expls:
                    lines.append(f"* {_escape_table_cell(ex)}")

            if asm_forms:
                lines.append("")
                lines.append("Assembly forms::")
                lines.append("+")
                for a in asm_forms:
                    lines.append(f"* `{_escape_table_cell(a)}`")

            if notes:
                lines.append("")
                lines.append("Encoding notes (informative)::")
                lines.append("+")
                for n in notes:
                    lines.append(f"* {_escape_table_cell(n)}")

            if op_lines:
                lines.append("")
                lines.append("Operation (informative)::")
                lines.append("+")
                lines.append("[source]")
                lines.append("----")
                for l in op_lines:
                    if l.strip():
                        lines.append(l)
                lines.append("----")

            lines.append("")

    _mkdirp(os.path.dirname(out_path))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

def _write_mnemonic_index(instructions: List[Dict[str, Any]], out_path: str, spec_version: str) -> None:
    """
    Emit a unique-mnemonic index to help readers navigate the catalog.

    Note: the catalog is form-based (variants are separate entries). This index collapses forms by mnemonic.
    """
    mnem2: Dict[str, Dict[str, Any]] = {}
    for inst in instructions:
        m = str(inst.get("mnemonic", "")).strip()
        if not m:
            continue
        entry = mnem2.setdefault(
            m, {"groups": set(), "lengths": set(), "forms": 0}
        )
        entry["forms"] += 1
        g = str(inst.get("group") or "").strip()
        if g:
            entry["groups"].add(g)
        entry["lengths"].add(int(inst.get("length_bits", 0)))

    lines: List[str] = []
    lines.append("// Generated file; do not edit by hand.")
    lines.append("")
    lines.append("[[mnemonic-index]]")
    lines.append("=== Mnemonic index")
    lines.append("")
    lines.append(
        f"This table lists unique mnemonics in the v{spec_version} catalog. Each mnemonic may have multiple encodable forms "
        "(different lengths, operand layouts, or suffix variants)."
    )
    lines.append("")
    lines.append('[cols="2,3,1,1",options="header"]')
    lines.append("|===")
    lines.append("|Mnemonic |Groups |Lengths (bits) |Forms")

    for m in sorted(mnem2.keys(), key=lambda s: s.lower()):
        entry = mnem2[m]
        groups = sorted(entry["groups"])
        lengths = sorted(entry["lengths"])
        forms = int(entry["forms"])

        groups_s = ", ".join(groups) if groups else "-"
        lengths_s = ", ".join(str(x) for x in lengths if x) if lengths else "-"

        lines.append(
            f"|`{_escape_table_cell(m)}` |{_escape_table_cell(groups_s)} |{lengths_s} |{forms}"
        )

    lines.append("|===")
    lines.append("")

    _mkdirp(os.path.dirname(out_path))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--profile",
        choices=["v0.3"],
        default="v0.3",
        help="ISA profile for default --spec path (v0.3 only)",
    )
    ap.add_argument("--spec", default=None, help="Path to ISA catalog JSON")
    ap.add_argument(
        "--out-dir",
        default="docs/architecture/isa-manual/src/generated",
        help="Output directory for generated .adoc fragments",
    )
    ap.add_argument("--check", action="store_true", help="Fail if outputs are not up-to-date")
    args = ap.parse_args(argv)

    spec_path = args.spec or "spec/isa/spec/current/linxisa-v0.3.json"
    spec = _read_json(spec_path)
    spec_version = str(spec.get("version") or "").strip() or "?"
    golden_hint = f"spec/isa/golden/v{spec_version}/" if spec_version != "?" else "spec/isa/golden/v*/"
    spec_label = os.path.basename(os.path.normpath(spec_path))
    source_comment = f"// Source: {spec_label} (built from {golden_hint})"
    instructions: List[Dict[str, Any]] = _filter_canonical_instructions(
        list(spec.get("instructions", [])), spec_version
    )
    groups = _group_instructions(instructions)

    outputs = [
        "registers_reg5.adoc",
        "instruction_group_summary.adoc",
        "instruction_reference.adoc",
        "mnemonic_index.adoc",
        "instruction_details.adoc",
    ]

    def _emit(out_dir: str) -> None:
        _mkdirp(out_dir)
        _write_registers_reg5(spec, os.path.join(out_dir, "registers_reg5.adoc"), source_comment)
        _write_instruction_group_summary(groups, os.path.join(out_dir, "instruction_group_summary.adoc"))
        _write_instruction_reference(groups, os.path.join(out_dir, "instruction_reference.adoc"))
        _write_mnemonic_index(instructions, os.path.join(out_dir, "mnemonic_index.adoc"), spec_version)
        _write_instruction_details(groups, os.path.join(out_dir, "instruction_details.adoc"), spec_version)

    if args.check:
        out_dir = args.out_dir
        with tempfile.TemporaryDirectory() as td:
            _emit(td)
            for name in outputs:
                want_p = os.path.join(out_dir, name)
                got_p = os.path.join(td, name)
                if not os.path.exists(want_p):
                    raise SystemExit(f"MISSING {want_p} (run gen_manual_adoc.py)")
                want = open(want_p, "r", encoding="utf-8").read()
                got = open(got_p, "r", encoding="utf-8").read()
                if want != got:
                    raise SystemExit(f"OUTDATED {want_p} (regenerate with gen_manual_adoc.py)")
        print("OK")
        return 0

    _emit(args.out_dir)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
