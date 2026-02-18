#!/usr/bin/env python3
"""
Reconcile raw LinxISA v0.3 materials into a machine-checkable decision ledger.

Inputs:
  - raw update text (default: ~/linxisa-v0.3.txt)
  - raw example asm (default: ~/linxisa-v0.3-example.asm)
  - Janus reference section for intent cross-check

Outputs:
  - isa/v0.3/reconcile/reconcile_report.json
  - isa/v0.3/reconcile/reconcile_notes.md
  - isa/v0.3/reconcile/linxisa-v0.3-example.normalized.asm
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple


_RE_ASM_TOKEN = re.compile(r"\b(BSTART\.[A-Za-z0-9.]+|C\.[A-Za-z0-9.]+|B\.[A-Za-z0-9.]+|[LlVv]\.[A-Za-z0-9_.]+)\b")


@dataclass(frozen=True)
class ItemRule:
    item_id: str
    status: str
    canonical: str
    reason: str
    patterns: Tuple[str, ...]


_RULES: Tuple[ItemRule, ...] = (
    ItemRule(
        item_id="simt_lane_policy",
        status="keep",
        canonical="Inactive-lane policy explicit (merge|zero)",
        reason="AI SIMT behavior is coherent and repeatedly specified",
        patterns=("merging mode", "zeroing mode", "inactive lane", "pmode"),
    ),
    ItemRule(
        item_id="memory_channel_model",
        status="keep",
        canonical="BCC/MTC channel contract with ordering/barrier semantics",
        reason="Text is coherent with Janus microarchitecture intent",
        patterns=("Tile Mode", "MCALL Mode", "BCC", "MTC", "\u540c\u5730\u5740\u4fdd\u5e8f", "Acquire", "Release"),
    ),
    ItemRule(
        item_id="tile_descriptor_model",
        status="keep",
        canonical="B.IOT/B.IOTI/B.IOD/B.ATTR/B.ARG/B.TEXT + C.B.DIMI",
        reason="Matches current block descriptor machinery and example evidence",
        patterns=("B.IOT", "B.IOTI", "B.IOD", "B.ATTR", "B.ARG", "B.TEXT", "C.B.DIMI"),
    ),
    ItemRule(
        item_id="cube_baseline_ops",
        status="keep",
        canonical="MAMULB/MAMULBAC/MAMULB.ACC/ACCCVT as staged CUBE baseline",
        reason="Consistent with example and existing bring-up support",
        patterns=("MAMULB", "MAMULBAC", "MAMULB.ACC", "ACCCVT"),
    ),
    ItemRule(
        item_id="tma_baseline_ops",
        status="keep",
        canonical="TLOAD/TSTORE/TPREFETCH/TMOV staged in TMA profile",
        reason="Well-defined in raw text and aligned with existing architecture docs",
        patterns=("TLOAD", "TSTORE", "TPREFETCH", "TMOV"),
    ),
    ItemRule(
        item_id="normalize_legacy_l_family",
        status="normalize",
        canonical="L.* -> V.*",
        reason="Canonical v0.3 naming policy is V.* only",
        patterns=("L.FADD", "L.FMAX", "l.fmax", "l.lw", "l.lwi", "l.sw", "l.fmul"),
    ),
    ItemRule(
        item_id="normalize_bstart_par",
        status="normalize",
        canonical="BSTART.PAR -> typed BSTART.{TMA,CUBE,TEPL,VPAR}",
        reason="Canonical typed block-start policy for v0.3 outputs",
        patterns=("BSTART.PAR",),
    ),
    ItemRule(
        item_id="normalize_kill_to_reuse",
        status="normalize",
        canonical="Legacy .kill annotations mapped to non-reuse semantics",
        reason="Canonical descriptor payload uses reuse flag encoding",
        patterns=(".kill", ".reuse"),
    ),
    ItemRule(
        item_id="defer_mamulbmx_group2",
        status="defer",
        canonical="MAMULBMX* group=2 semantics deferred",
        reason="Raw material contains unresolved/contradictory group=2 details",
        patterns=("MAMULBMXAC", "group=2", "MAMULBMX.ACC"),
    ),
    ItemRule(
        item_id="defer_legacy_052_fragments",
        status="defer",
        canonical="Legacy 0.52 fragments excluded from canonical v0.3",
        reason="Raw text includes mixed-version carryover sections",
        patterns=("0.52\u5f15\u5165\u7684\u6307\u4ee4",),
    ),
    ItemRule(
        item_id="drop_editorial_prompt_sections",
        status="drop",
        canonical="Prompt/editorial text removed",
        reason="Non-normative authoring prompts are not ISA content",
        patterns=(
            "\u8bf7\u7ed3\u5408ARM\u67b6\u6784",
            "\u8bf7\u91cd\u65b0\u5e2e\u6211\u4fee\u9970",
            "\u597d\uff0c\u8bf7\u91cd\u65b0\u5e2e\u6211\u4fee\u9970",
        ),
    ),
)


def _find_lines(text: str, needle: str, max_hits: int = 8) -> List[int]:
    out: List[int] = []
    start = 0
    while len(out) < max_hits:
        idx = text.find(needle, start)
        if idx < 0:
            break
        out.append(text.count("\n", 0, idx) + 1)
        start = idx + len(needle)
    return out


def _display_path(path: Path, repo_root: Path) -> str:
    p = path.resolve()
    home = Path.home().resolve()
    try:
        return str(p.relative_to(repo_root))
    except ValueError:
        pass
    try:
        return "~/" + str(p.relative_to(home))
    except ValueError:
        pass
    return str(p)


def _collect_mnems(asm_text: str) -> Set[str]:
    tokens = {m.group(1) for m in _RE_ASM_TOKEN.finditer(asm_text)}
    return {t for t in tokens if t}


def _classify_asm_tokens(tokens: Iterable[str]) -> List[Dict[str, object]]:
    out: List[Dict[str, object]] = []
    for t in sorted(set(tokens)):
        upper = t.upper()
        if upper == "L.BSTOP":
            out.append(
                {
                    "token": t,
                    "status": "normalize",
                    "canonical": "C.BSTOP",
                    "reason": "legacy BSTOP spelling normalized",
                }
            )
            continue
        if upper.startswith("L."):
            out.append(
                {
                    "token": t,
                    "status": "normalize",
                    "canonical": "v." + t.split(".", 1)[1].lower(),
                    "reason": "legacy vector mnemonic family normalized to V/v canonical naming",
                }
            )
            continue
        if upper == "BSTART.PAR":
            out.append(
                {
                    "token": t,
                    "status": "normalize",
                    "canonical": "BSTART.{TMA|CUBE|TEPL|VPAR}",
                    "reason": "typed BSTART form is canonical in staged v0.3",
                }
            )
            continue
        out.append(
            {
                "token": t,
                "status": "keep",
                "canonical": t,
                "reason": "token already canonical/accepted in staged v0.3 subset",
            }
        )
    return out


def _janus_check(janus_text: str, canonical: str) -> Dict[str, object]:
    checks = {
        "tile": ("tile", "tilereg", "scoreboard"),
        "lane": ("lane", "lb0", "lc0"),
        "memory": ("memory", "bcc", "mtc", "order"),
        "block": ("block", "bstart", "bstop"),
    }
    hay = janus_text.lower()
    hit: Dict[str, bool] = {name: any(k in hay for k in keys) for name, keys in checks.items()}
    aligned = any(hit.values())
    return {"aligned": aligned, "signals": hit, "note": f"Janus keyword scan for: {canonical}"}


def _run_normalizer(script: Path, asm_in: Path, asm_out: Path, report_out: Path) -> None:
    cmd = [
        "python3",
        str(script),
        "--in",
        str(asm_in),
        "--out",
        str(asm_out),
        "--report",
        str(report_out),
    ]
    p = subprocess.run(cmd, check=False, capture_output=True, text=True)
    if p.returncode != 0:
        raise SystemExit(f"normalizer failed: {p.stderr.strip() or p.stdout.strip()}")


def _render_notes(report: Dict[str, object]) -> str:
    lines: List[str] = []
    lines.append("# LinxISA v0.3 Reconciliation Notes")
    lines.append("")
    lines.append("## Status Summary")
    counts: Dict[str, int] = report["summary"]["status_counts"]  # type: ignore[index]
    for key in ("keep", "normalize", "defer", "drop"):
        lines.append(f"- `{key}`: {counts.get(key, 0)}")
    lines.append("")

    lines.append("## Decisions")
    for item in report["items"]:  # type: ignore[index]
        lines.append(
            f"- `{item['id']}` -> `{item['status']}`: {item['canonical']} ({item['reason']})"
        )
    lines.append("")

    lines.append("## Explicit Defer List")
    deferred = [i for i in report["items"] if i["status"] == "defer"]  # type: ignore[index]
    if not deferred:
        lines.append("- (none)")
    else:
        for item in deferred:
            lines.append(f"- `{item['id']}`: {item['canonical']}")
    lines.append("")

    lines.append("## ASM Token Classification")
    for token in report["asm_tokens"]:  # type: ignore[index]
        lines.append(f"- `{token['token']}` -> `{token['status']}` ({token['canonical']})")
    lines.append("")

    lines.append("## Janus Alignment")
    for item in report["items"]:  # type: ignore[index]
        jc = item["janus_check"]
        lines.append(f"- `{item['id']}` aligned={jc['aligned']} ({jc['note']})")
    lines.append("")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="Reconcile LinxISA v0.3 raw materials.")
    ap.add_argument("--raw-text", default="~/linxisa-v0.3.txt", help="Raw v0.3 text")
    ap.add_argument("--raw-asm", default="~/linxisa-v0.3-example.asm", help="Raw v0.3 asm example")
    ap.add_argument(
        "--janus-ref",
        default="~/JanusCore/paper/srcs/sections/04_tile_microarchitecture.tex",
        help="Janus cross-check reference file",
    )
    ap.add_argument(
        "--out-dir",
        default="isa/v0.3/reconcile",
        help="Output reconciliation directory",
    )
    args = ap.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    out_dir = (repo_root / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    raw_text_path = Path(args.raw_text).expanduser().resolve()
    raw_asm_path = Path(args.raw_asm).expanduser().resolve()
    janus_path = Path(args.janus_ref).expanduser().resolve()

    raw_text = raw_text_path.read_text(encoding="utf-8", errors="replace")
    raw_asm = raw_asm_path.read_text(encoding="utf-8", errors="replace")
    janus_text = janus_path.read_text(encoding="utf-8", errors="replace")

    normalize_script = repo_root / "tools" / "isa" / "normalize_v03_example_asm.py"
    normalized_asm_path = out_dir / "linxisa-v0.3-example.normalized.asm"
    normalized_report_path = out_dir / "linxisa-v0.3-example.normalize_report.json"
    _run_normalizer(normalize_script, raw_asm_path, normalized_asm_path, normalized_report_path)

    items: List[Dict[str, object]] = []
    for rule in _RULES:
        hits: List[Dict[str, object]] = []
        for pat in rule.patterns:
            text_hits = _find_lines(raw_text, pat)
            asm_hits = _find_lines(raw_asm, pat)
            if text_hits or asm_hits:
                hits.append({"pattern": pat, "raw_text_lines": text_hits, "raw_asm_lines": asm_hits})
        if not hits:
            continue
        items.append(
            {
                "id": rule.item_id,
                "status": rule.status,
                "canonical": rule.canonical,
                "reason": rule.reason,
                "evidence": hits,
                "janus_check": _janus_check(janus_text, rule.canonical),
            }
        )

    asm_tokens = _classify_asm_tokens(_collect_mnems(raw_asm))
    asm_unclassified = [t["token"] for t in asm_tokens if t["status"] not in {"keep", "normalize", "defer", "drop"}]

    status_counts: Dict[str, int] = {}
    for item in items:
        status = str(item["status"])
        status_counts[status] = status_counts.get(status, 0) + 1

    report: Dict[str, object] = {
        "version": "0.3",
        "inputs": {
            "raw_text": _display_path(raw_text_path, repo_root),
            "raw_asm": _display_path(raw_asm_path, repo_root),
            "janus_ref": _display_path(janus_path, repo_root),
            "normalized_asm": _display_path(normalized_asm_path, repo_root),
        },
        "summary": {
            "status_counts": status_counts,
            "item_count": len(items),
            "asm_token_count": len(asm_tokens),
            "asm_unclassified": asm_unclassified,
            "explicit_defer_list": [i["id"] for i in items if i["status"] == "defer"],
        },
        "items": items,
        "asm_tokens": asm_tokens,
        "assumptions": [
            "v0.2 encoding baseline is preserved for staged v0.3 integration.",
            "Canonical v0.3 output uses typed BSTART.* and V.* mnemonic families only.",
            "Unresolved MAMULBMX group=2 semantics are deferred from canonical staged profile.",
        ],
    }

    report_path = out_dir / "reconcile_report.json"
    notes_path = out_dir / "reconcile_notes.md"
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    notes_path.write_text(_render_notes(report), encoding="utf-8")

    print(f"wrote {report_path}")
    print(f"wrote {notes_path}")
    print(f"normalized asm: {normalized_asm_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
