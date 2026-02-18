#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


# Parse only top-level symbol labels; keep local ".L..." labels inside the function body.
FUNC_LABEL_RE = re.compile(r"^([A-Za-z_][\w$]*):\s*(?:#.*)?$")


def parse_functions(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    cur_name: str | None = None
    buf: list[str] = []
    for ln in text.splitlines():
        m = FUNC_LABEL_RE.match(ln)
        if m:
            if cur_name is not None:
                out[cur_name] = "\n".join(buf)
            cur_name = m.group(1)
            buf = [ln]
            continue
        if cur_name is None:
            continue
        buf.append(ln)
        if re.match(rf"^\s*\.size\s+{re.escape(cur_name)},", ln):
            out[cur_name] = "\n".join(buf)
            cur_name = None
            buf = []
    if cur_name is not None and buf:
        out[cur_name] = "\n".join(buf)
    return out


def require(cond: bool, msg: str) -> None:
    if not cond:
        raise AssertionError(msg)


def check_non_tail(label: str, asm: str) -> None:
    require("FENTRY" in asm, f"{label}: missing FENTRY")
    require("FRET.STK" in asm, f"{label}: missing FRET.STK")
    require("FEXIT" not in asm, f"{label}: unexpected FEXIT in non-musttail test")


def check_tail_musttail(label: str, asm: str) -> None:
    funcs = parse_functions(asm)
    for fn in ("callret_tail_direct", "callret_tail_indirect"):
        require(fn in funcs, f"{label}: missing function body for {fn}")
        require("FENTRY" in funcs[fn], f"{label}:{fn}: missing FENTRY")

    direct_body = funcs["callret_tail_direct"]
    direct_is_tail_transfer = (
        "FEXIT" in direct_body
        and "FRET.STK" not in direct_body
        and re.search(r"\b(?:C\.)?BSTART\s+DIRECT,\s*tail_target\b", direct_body) is not None
    )
    direct_is_legacy_tail = (
        re.search(r"\b(?:C\.)?BSTART(?:\.STD)?\s+CALL,\s*tail_target\b", direct_body) is not None
        and "FRET.STK" in direct_body
    )
    require(
        direct_is_tail_transfer or direct_is_legacy_tail,
        f"{label}:callret_tail_direct: missing accepted musttail lowering pattern",
    )

    indirect_body = funcs["callret_tail_indirect"]
    indirect_is_tail_transfer = (
        "FEXIT" in indirect_body
        and "FRET.STK" not in indirect_body
        and re.search(r"\b(?:C\.)?BSTART(?:\.STD)?\s+IND\b", indirect_body) is not None
        and re.search(r"\bc\.setc\.tgt\b", indirect_body, re.IGNORECASE) is not None
    )
    indirect_is_legacy_tail = (
        re.search(r"\b(?:C\.)?BSTART(?:\.STD)?\s+ICALL\b", indirect_body) is not None
        and re.search(r"\bc\.setret\b", indirect_body, re.IGNORECASE) is not None
        and re.search(r"\bc\.setc\.tgt\b", indirect_body, re.IGNORECASE) is not None
        and "FRET.STK" in indirect_body
    )
    require(
        indirect_is_tail_transfer or indirect_is_legacy_tail,
        f"{label}:callret_tail_indirect: missing accepted musttail lowering pattern",
    )


def main() -> int:
    ap = argparse.ArgumentParser(description="Check Linx call/ret template lowering in generated asm.")
    ap.add_argument("--asm", required=True, type=Path)
    ap.add_argument("--label", required=True)
    args = ap.parse_args()

    asm_text = args.asm.read_text(encoding="utf-8", errors="replace")
    label = args.label

    try:
        if re.match(r"^(33|34|35|36|38)_callret_", label):
            check_non_tail(label, asm_text)
        elif re.match(r"^37_callret_", label):
            check_tail_musttail(label, asm_text)
        else:
            raise AssertionError(f"unsupported label for template check: {label}")
    except AssertionError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"ok: {label} call/ret template check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
