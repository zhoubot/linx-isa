#!/usr/bin/env python3
"""
Normalize raw v0.3 example asm syntax into canonical Linx v0.3 bring-up style.

Canonicalization policy for this tool:
  - `l.*`/`L.*` mnemonics -> `v.*`
  - `L.BSTOP` -> `C.BSTOP`
  - `BSTART.PAR` -> typed `BSTART.{TMA,CUBE,VPAR}` by operand heuristic

The goal is deterministic normalization for reconciliation; this script does not
guarantee that emitted text is directly assemblable.
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


_RE_BSTART_PAR = re.compile(r"\bBSTART\.PAR\b")
_RE_MNEM = re.compile(r"\b([Ll]\.[A-Za-z0-9_.]+|L\.BSTOP)\b")
_RE_COMMENT = re.compile(r"(;|#|//).*$")


_CUBE_OPS = {
    "MAMULB",
    "MAMULBAC",
    "MAMULB.ACC",
    "MAMULBMX",
    "MAMULBMXAC",
    "MAMULBMX.ACC",
    "ACCCVT",
    "TCVT",
}
_TMA_OPS = {"TLOAD", "TSTORE", "TPREFETCH"}
_VPAR_OPS = {"VCALL", "VCALLI"}


@dataclass
class Change:
    line: int
    kind: str
    original: str
    normalized: str
    reason: str


def _split_code_comment(line: str) -> Tuple[str, str]:
    m = _RE_COMMENT.search(line)
    if not m:
        return line, ""
    return line[: m.start()], line[m.start() :]


def _guess_bstart_kind(code: str) -> Tuple[str, str]:
    """
    Infer target typed BSTART kind from the first operand after BSTART.PAR.
    """
    tail = _RE_BSTART_PAR.split(code, maxsplit=1)[1].strip()
    if not tail:
        return "BSTART.VPAR", "fallback(no-operand)"
    first = tail.split()[0].rstrip(",")
    key = first.upper()
    if key in _CUBE_OPS:
        return "BSTART.CUBE", f"operand({first})"
    if key in _TMA_OPS:
        return "BSTART.TMA", f"operand({first})"
    if key in _VPAR_OPS:
        return "BSTART.VPAR", f"operand({first})"
    # Numeric or unknown opcodes are normalized to VPAR in staged v0.3.
    return "BSTART.VPAR", f"fallback({first})"


def _normalize_line(line: str, line_no: int) -> Tuple[str, List[Change]]:
    code, comment = _split_code_comment(line.rstrip("\n"))
    changes: List[Change] = []

    if "BSTART.PAR" in code:
        repl, why = _guess_bstart_kind(code)
        new_code = _RE_BSTART_PAR.sub(repl, code)
        if new_code != code:
            changes.append(
                Change(
                    line=line_no,
                    kind="bstart",
                    original=code.strip(),
                    normalized=new_code.strip(),
                    reason=f"BSTART.PAR->typed({why})",
                )
            )
            code = new_code

    def _mnem_repl(m: re.Match[str]) -> str:
        token = m.group(1)
        if token.upper() == "L.BSTOP":
            out = "C.BSTOP"
            changes.append(
                Change(
                    line=line_no,
                    kind="mnemonic",
                    original=token,
                    normalized=out,
                    reason="legacy L.BSTOP normalized to compressed C.BSTOP",
                )
            )
            return out

        # l.xxx / L.xxx -> v.xxx
        body = token.split(".", maxsplit=1)[1]
        out = f"v.{body.lower()}"
        if token != out:
            changes.append(
                Change(
                    line=line_no,
                    kind="mnemonic",
                    original=token,
                    normalized=out,
                    reason="vector mnemonic family normalized to canonical v.*",
                )
            )
        return out

    code = _RE_MNEM.sub(_mnem_repl, code)
    return code + comment, changes


def normalize_text(text: str) -> Tuple[str, List[Change]]:
    out_lines: List[str] = []
    all_changes: List[Change] = []
    for i, line in enumerate(text.splitlines(), start=1):
        norm, changes = _normalize_line(line, i)
        out_lines.append(norm)
        all_changes.extend(changes)
    return "\n".join(out_lines) + "\n", all_changes


def main() -> int:
    ap = argparse.ArgumentParser(description="Normalize raw Linx v0.3 example asm.")
    ap.add_argument("--in", dest="in_path", required=True, help="Input raw asm path")
    ap.add_argument("--out", dest="out_path", required=True, help="Output normalized asm path")
    ap.add_argument(
        "--report",
        default="",
        help="Optional JSON report path containing normalization edits",
    )
    args = ap.parse_args()

    in_path = Path(args.in_path).expanduser().resolve()
    out_path = Path(args.out_path).expanduser().resolve()
    report_path = Path(args.report).expanduser().resolve() if args.report else None

    text = in_path.read_text(encoding="utf-8", errors="replace")
    normalized, changes = normalize_text(text)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(normalized, encoding="utf-8")

    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        payload: Dict[str, object] = {
            "input": str(in_path),
            "output": str(out_path),
            "change_count": len(changes),
            "changes": [c.__dict__ for c in changes],
        }
        report_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"normalized: {in_path} -> {out_path} ({len(changes)} edits)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

