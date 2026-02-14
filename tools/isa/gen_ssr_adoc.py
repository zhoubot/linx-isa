#!/usr/bin/env python3
"""
Generate AsciiDoc fragments for LinxISA System Status Registers (SSR) and TRAPNO encoding.

Source of truth (v0.3): `state.system_registers` inside the compiled ISA JSON spec.

Outputs into an output directory (typically `docs/architecture/isa-manual/src/generated/`):
  - system_registers_ssr.adoc
  - trapno_encoding.adoc
"""

from __future__ import annotations

import argparse
import json
import os
import tempfile
from typing import Any, Dict, Iterable, List, Optional, Tuple


def _read_json(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _write_if_different(path: str, content: str, check: bool) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    if check:
        if not os.path.exists(path):
            raise FileNotFoundError(path)
        with open(path, "r", encoding="utf-8") as f:
            actual = f.read()
        if actual != content:
            raise ValueError(f"OUTDATED {path}")
        return
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def _as_int(x: Any) -> int:
    if isinstance(x, int):
        return x
    if isinstance(x, str):
        s = x.strip().lower()
        if s.startswith("0x"):
            return int(s, 16)
        return int(s, 10)
    raise TypeError(f"expected int/str, got {type(x).__name__}")


def _fmt_hex(val: int, width: int = 4) -> str:
    return f"0x{val:0{width}X}"


def _iter_entries(entries: List[Dict[str, Any]]) -> Iterable[Tuple[int, str, str, str]]:
    """
    Yield (id_or_idx, name, access, description) rows.

    Supports fixed entries with `id` or `idx`, and ranged entries with:
      - idx_base, stride, count, name_fmt
    """
    for e in entries:
        if "idx_base" in e:
            base = _as_int(e["idx_base"])
            stride = _as_int(e.get("stride", 1))
            count = _as_int(e.get("count", 0))
            name_fmt = str(e.get("name_fmt") or "").strip()
            access = str(e.get("access") or "").strip() or "-"
            desc = str(e.get("description") or "").strip()
            for i in range(count):
                yield base + i * stride, (name_fmt % i), access, desc
            continue

        key = "id" if "id" in e else "idx"
        val = _as_int(e.get(key))
        name = str(e.get("name") or "").strip()
        access = str(e.get("access") or "").strip() or "-"
        desc = str(e.get("description") or "").strip()
        yield val, name, access, desc


def _adoc_header(spec_path: str) -> str:
    spec_label = os.path.basename(os.path.normpath(spec_path))
    return (
        "// Generated file. Do not edit by hand.\n"
        f"// Source: {spec_label} (state.system_registers)\n"
        "\n"
    )


def gen_system_registers_ssr(spec_path: str, sysregs: Dict[str, Any]) -> str:
    ssr = sysregs.get("ssr", {}) or {}
    base = list(ssr.get("base", []) or [])
    mgr = ssr.get("manager_acr_family", {}) or {}
    mgr_entries = list(mgr.get("entries", []) or [])

    ebarg = sysregs.get("ebarg_group", {}) or {}
    ebarg_entries = list(ebarg.get("entries", []) or [])

    dbg = sysregs.get("debug_ssr", {}) or {}
    dbg_entries = list(dbg.get("entries", []) or [])

    lines: List[str] = []
    lines.append(_adoc_header(spec_path).rstrip("\n"))

    lines.append("[[ssr-table]]")
    lines.append("==== System Status Registers (SSR)")
    lines.append("")
    lines.append("The following **System Status Registers (SSR)** are defined by the current bring-up profile.")
    lines.append("Unless noted, SSR values are **XLEN** wide and are accessed with the SSR access instructions.")
    lines.append("")
    lines.append("SSR access instructions:")
    lines.append("")
    lines.append("* `SSRGET` / `SSRSET` / `SSRSWAP` (32-bit forms) encode a **12-bit** `SSR_ID[11:0]`.")
    lines.append("* `HL.SSRGET` / `HL.SSRSET` (48-bit forms) encode a **24-bit** `SSR_ID[23:0]` and are used when the full SSR ID does")
    lines.append("  not fit in 12 bits (e.g. manager-ACR IDs like `0x1Fxx`).")
    lines.append("* `C.SSRGET` (16-bit) reads a small set of commonly used registers; its `SSRID` encoding is a profile-defined remap.")
    lines.append("")
    lines.append("[%header,cols=\"1,1,1,4\"]")
    lines.append("|===")
    lines.append("|SSR_ID |Name |Access |Description")
    lines.append("")
    for ssrid, name, access, desc in sorted(_iter_entries(base), key=lambda r: r[0]):
        lines.append(f"|`{_fmt_hex(ssrid, 4)}` |`{name}` |{access} |{desc}")
    lines.append("|===")
    lines.append("")

    lines.append("[[ssr-acr-scoped]]")
    lines.append("==== Manager-ACR (privileged) SSR families")
    lines.append("")
    lines.append("Privileged SSRs are addressed using IDs of the form `SSR_ID = 0x n f xx` (16-bit), where `n` is the manager ACR ID.")
    lines.append("")
    lines.append("In the bring-up privilege model:")
    lines.append("")
    lines.append("* These SSRs MUST be accessible from **ACR0** and **ACR1**.")
    lines.append("* Accesses from other ACRs MAY be ignored or MAY raise an exception, depending on the platform profile.")
    lines.append("")
    lines.append("Use `HL.SSRGET/HL.SSRSET` when the full `SSR_ID` does not fit in 12 bits.")
    lines.append("")
    lines.append("[%header,cols=\"1,1,1,4\"]")
    lines.append("|===")
    lines.append("|SSR_ID |Name |Access |Description")
    lines.append("")
    for idx, name, access, desc in sorted(_iter_entries(mgr_entries), key=lambda r: r[0]):
        lines.append(f"|`0xnf{idx & 0xFF:02X}` |`{name}` |{access} |{desc}")
    lines.append("|===")
    lines.append("")

    lines.append("[[ssr-ebarg]]")
    lines.append("==== EBARG group (v0.3)")
    lines.append("")
    lines.append(str(ebarg.get("description") or "EBARG trap-save group.").strip())
    lines.append("")
    lines.append("[%header,cols=\"1,1,1,4\"]")
    lines.append("|===")
    lines.append("|SSR_ID |Name |Access |Description")
    lines.append("")
    for idx, name, access, desc in sorted(_iter_entries(ebarg_entries), key=lambda r: r[0]):
        lines.append(f"|`0xnf{idx & 0xFF:02X}` |`{name}` |{access} |{desc}")
    lines.append("|===")
    lines.append("")

    lines.append("[[ssr-debug]]")
    lines.append("==== Debug SSRs (v0.3 bring-up)")
    lines.append("")
    lines.append(str(dbg.get("description") or "Debug configuration SSRs.").strip())
    lines.append("")
    lines.append("[%header,cols=\"1,1,1,4\"]")
    lines.append("|===")
    lines.append("|SSR_ID |Name |Access |Description")
    lines.append("")
    for idx, name, access, desc in sorted(_iter_entries(dbg_entries), key=lambda r: r[0]):
        lines.append(f"|`0xnf{idx & 0xFF:02X}` |`{name}` |{access} |{desc}")
    lines.append("|===")
    lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def gen_trapno_encoding(spec_path: str, sysregs: Dict[str, Any]) -> str:
    enc = sysregs.get("trapno_encoding", {}) or {}
    fields = list(enc.get("fields", []) or [])
    trapnums = list(enc.get("bringup_trapnums", []) or [])

    lines: List[str] = []
    lines.append(_adoc_header(spec_path).rstrip("\n"))
    lines.append("[[trapno-encoding]]")
    lines.append("==== TRAPNO encoding (v0.3 bring-up)")
    lines.append("")
    for n in enc.get("notes", []) or []:
        lines.append(f"* {str(n).strip()}")
    lines.append("")
    lines.append("Bit layout:")
    lines.append("")
    lines.append("[source]")
    lines.append("----")
    lines.append("63        62        47        24        5      0")
    lines.append("+----------+----------+----------------+--------+")
    lines.append("|    E     |   ARGV   |     CAUSE      | TRAPNUM|")
    lines.append("+----------+----------+----------------+--------+")
    lines.append("----")
    lines.append("")

    if fields:
        lines.append("[%header,cols=\"1,1,3\"]")
        lines.append("|===")
        lines.append("|Field |Bits |Description")
        lines.append("")
        for f in fields:
            name = str(f.get("name") or "").strip()
            msb = _as_int(f.get("msb"))
            lsb = _as_int(f.get("lsb"))
            desc = str(f.get("description") or "").strip()
            bits = f"[{msb}]" if msb == lsb else f"[{msb}:{lsb}]"
            lines.append(f"|`{name}` |`{bits}` |{desc}")
        lines.append("|===")
        lines.append("")

    if trapnums:
        lines.append("Bring-up trap classes:")
        lines.append("")
        lines.append("[%header,cols=\"1,1,1,4\"]")
        lines.append("|===")
        lines.append("|TRAPNUM |Dec |Name |Description")
        lines.append("")
        for t in sorted(trapnums, key=lambda x: _as_int(x.get("trapnum", 0))):
            tn = _as_int(t.get("trapnum", 0))
            nm = str(t.get("name") or "").strip() or "-"
            desc = str(t.get("description") or "").strip()
            lines.append(f"|`0b{tn:06b}` |{tn} |`{nm}` |{desc}")
        lines.append("|===")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


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
        default=os.path.join("docs", "architecture", "isa-manual", "src", "generated"),
        help="Output directory for AsciiDoc fragments",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="Verify that generated files in --out-dir are up-to-date",
    )
    args = ap.parse_args()

    default_spec = os.path.join("spec", "isa", "spec", "current", "linxisa-v0.3.json")
    spec_path = args.spec or default_spec
    spec = _read_json(spec_path)
    sysregs = (((spec.get("state") or {}).get("system_registers")) or {})
    if not isinstance(sysregs, dict) or not sysregs:
        raise SystemExit("error: spec missing state.system_registers (expected v0.3 spec)")

    ssr_adoc = gen_system_registers_ssr(spec_path, sysregs)
    trap_adoc = gen_trapno_encoding(spec_path, sysregs)

    out_ssr = os.path.join(args.out_dir, "system_registers_ssr.adoc")
    out_trap = os.path.join(args.out_dir, "trapno_encoding.adoc")

    try:
        _write_if_different(out_ssr, ssr_adoc, check=args.check)
        _write_if_different(out_trap, trap_adoc, check=args.check)
    except FileNotFoundError as e:
        print(f"MISSING {e.filename}")
        return 1
    except ValueError as e:
        print(str(e))
        return 1

    if args.check:
        print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
