#!/usr/bin/env python3

from __future__ import annotations

import argparse
import gzip
import json
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Iterator, List, Optional, Tuple


_RE_LINE = re.compile(
    r"^\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]{2}(?:\s+[0-9a-fA-F]{2})*)\s+(.*)$"
)
_RE_DEST = re.compile(r"->\s*([A-Za-z][A-Za-z0-9_.]*(?:#[0-9]+)?)")
_RE_TOKEN = re.compile(r"[A-Za-z][A-Za-z0-9_.]*(?:#[0-9]+)?")


def _open_text(path: Path) -> Iterable[str]:
    if str(path).endswith(".gz"):
        return gzip.open(path, "rt", encoding="utf-8", errors="replace")
    return path.open("rt", encoding="utf-8", errors="replace")


def _load_gpr_names(spec_path: Path | None) -> set[str]:
    if spec_path is None or not spec_path.exists():
        return set()
    try:
        raw = json.loads(spec_path.read_text(encoding="utf-8", errors="replace"))
        reg5 = (raw.get("registers") or {}).get("reg5") or {}
        out: set[str] = set()
        for e in reg5.get("entries") or []:
            asm = str(e.get("asm", "")).strip()
            if asm:
                out.add(asm.lower())
            name = str(e.get("name", "")).strip()
            if name:
                out.add(name.lower())
            for a in e.get("aliases") or []:
                a = str(a).strip()
                if a:
                    out.add(a.lower())
        return out
    except Exception:
        return set()


def _canonical_mnemonic(mnemonic: str) -> str:
    s = mnemonic.strip()
    if not s:
        return ""
    return s.replace(" ", ".")


def _extract_src_tokens(operands: str) -> Iterator[str]:
    operands = _RE_DEST.sub("", operands)
    for tok in _RE_TOKEN.findall(operands):
        yield tok


def _is_pseudo_reg(tok: str) -> bool:
    # `t#1`, `u#2`, etc.
    return bool(re.fullmatch(r"[a-z]{1,6}#[0-9]+", tok.lower()))


@dataclass(frozen=True)
class Insn:
    mnem: str
    enc_bits: int
    src_gprs: Tuple[str, ...]
    dst_gprs: Tuple[str, ...]


def _parse_line_to_insn(line: str, *, gpr_names: set[str]) -> Optional[Insn]:
    m = _RE_LINE.match(line)
    if not m:
        return None
    bytes_text = m.group(2)
    insn_text = m.group(3).strip()
    byte_tokens = bytes_text.split()
    if not byte_tokens:
        return None
    enc_bits = len(byte_tokens) * 8

    parts = insn_text.split()
    if not parts:
        return None
    mnem = _canonical_mnemonic(parts[0])
    operands = insn_text[len(parts[0]) :].strip()

    dst: List[str] = []
    for d in _RE_DEST.findall(operands):
        dd = d.strip().lower()
        if dd in gpr_names or _is_pseudo_reg(dd):
            dst.append(dd)

    src: List[str] = []
    for tok in _extract_src_tokens(operands):
        tt = tok.strip().lower()
        if tt in gpr_names or _is_pseudo_reg(tt):
            src.append(tt)

    return Insn(mnem=mnem, enc_bits=enc_bits, src_gprs=tuple(src), dst_gprs=tuple(dst))


def _fmt_pct(n: int, d: int) -> str:
    if d <= 0:
        return "0.00"
    return f"{(100.0 * n / d):.2f}"


def _mnem_segments(mnemonic: str) -> List[str]:
    # Mnemonics are normalized by `_canonical_mnemonic()` where spaces become '.'.
    return [s for s in mnemonic.strip().upper().split(".") if s]


def _is_block_start_mnem(mnemonic: str) -> bool:
    segs = _mnem_segments(mnemonic)
    return "BSTART" in segs


def _is_block_end_mnem(mnemonic: str) -> bool:
    # Conservative: treat explicit stop/commit-like markers as block terminators.
    segs = _mnem_segments(mnemonic)
    return ("BSTACK" in segs) or ("BSTOP" in segs)


def _top_table(counter: Counter[str], *, total: int, top: int) -> str:
    lines = ["| Item | Count | % |", "|---|---:|---:|"]
    for k, v in counter.most_common(top):
        lines.append(f"| `{k}` | {v} | {_fmt_pct(v, total)} |")
    return "\n".join(lines)


class SpaceSaving:
    """
    Space-Saving heavy hitters (Metwally et al.).

    Keeps up to `k` items with approximate counts:
    - If item exists: increment count.
    - Else if room: insert with count=1,error=0.
    - Else: replace current minimum-count entry with new item
      with count=min+1,error=min.

    Implementation uses a lazy heap for O(log k) updates.
    """

    def __init__(self, k: int):
        if k <= 0:
            raise ValueError("k must be > 0")
        self.k = int(k)
        self.table: Dict[Tuple[str, ...], Tuple[int, int]] = {}  # key -> (count, error)
        self.heap: List[Tuple[int, int, Tuple[str, ...]]] = []  # (count, seq, key)
        self._seq = 0

    def _heap_push(self, count: int, key: Tuple[str, ...]) -> None:
        self._seq += 1
        self.heap.append((count, self._seq, key))
        # Manual heappush to avoid importing heapq in hot path? It's fine to import heapq.

    def add(self, key: Tuple[str, ...]) -> None:
        import heapq

        if key in self.table:
            count, err = self.table[key]
            count += 1
            self.table[key] = (count, err)
            heapq.heappush(self.heap, (count, self._seq, key))
            self._seq += 1
            return

        if len(self.table) < self.k:
            self.table[key] = (1, 0)
            heapq.heappush(self.heap, (1, self._seq, key))
            self._seq += 1
            return

        # Evict current min.
        while True:
            if not self.heap:
                # Should never happen, but recover by clearing.
                self.table.clear()
                self.heap.clear()
                self.table[key] = (1, 0)
                heapq.heappush(self.heap, (1, self._seq, key))
                self._seq += 1
                return
            min_count, _, victim = heapq.heappop(self.heap)
            cur = self.table.get(victim)
            if cur is None:
                continue
            cur_count, _cur_err = cur
            if cur_count != min_count:
                continue
            # Valid min.
            del self.table[victim]
            self.table[key] = (min_count + 1, min_count)
            heapq.heappush(self.heap, (min_count + 1, self._seq, key))
            self._seq += 1
            return

    def items(self) -> List[Tuple[Tuple[str, ...], int, int]]:
        out: List[Tuple[Tuple[str, ...], int, int]] = []
        for k, (c, e) in self.table.items():
            out.append((k, c, e))
        out.sort(key=lambda t: (-t[1], t[0]))
        return out


def _ngram_table_heavyhitters(
    items: List[Tuple[Tuple[str, ...], int, int]],
    *,
    total: int,
    top: int,
) -> str:
    lines = ["| Pattern | Count | % |", "|---|---:|---:|"]
    for k, c, _e in items[:top]:
        pat = " ; ".join(k)
        lines.append(f"| `{pat}` | {c} | {_fmt_pct(c, total)} |")
    return "\n".join(lines)


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser(
        description="Aggregate Linx llvm-objdump outputs into opcode/length/register/pattern stats (streaming, gzip ok)."
    )
    ap.add_argument(
        "--roots",
        nargs="+",
        default=["workloads/generated/objdump"],
        help="One or more directories to search for objdump text files.",
    )
    ap.add_argument(
        "--glob",
        default="**/*.objdump.txt*",
        help="Glob pattern under each root (default: **/*.objdump.txt*).",
    )
    ap.add_argument(
        "--spec",
        default="spec/isa/spec/current/linxisa-v0.3.json",
        help="ISA spec JSON for register name extraction.",
    )
    ap.add_argument(
        "--out-md",
        default="workloads/generated/objdump_aggregate_stats.md",
        help="Output Markdown report path.",
    )
    ap.add_argument(
        "--out-json",
        default="workloads/generated/objdump_aggregate_stats.json",
        help="Output JSON stats path.",
    )
    ap.add_argument("--top", type=int, default=50, help="Top-N entries to show in tables.")
    ap.add_argument("--max-files", type=int, default=0, help="If non-zero, limit number of files processed.")
    ap.add_argument(
        "--ngram-heavyhitters-k",
        type=int,
        default=20000,
        help="Space-Saving capacity per n-gram size (default: 20000).",
    )
    args = ap.parse_args(argv)

    spec_path = Path(args.spec)
    gpr_names = _load_gpr_names(spec_path)
    if not gpr_names:
        print(
            f"warning: no GPR names loaded from spec: {spec_path} (register stats may be incomplete)",
            file=sys.stderr,
        )

    roots = [Path(r) for r in args.roots]
    files: List[Path] = []
    for r in roots:
        files.extend(sorted(r.glob(args.glob)))
    files = [p for p in files if p.is_file()]
    if not files:
        raise SystemExit(f"error: no objdump files found (roots={args.roots} glob={args.glob})")
    if args.max_files and args.max_files > 0:
        files = files[: args.max_files]

    opcode_hist: Counter[str] = Counter()
    enc_hist: Counter[int] = Counter()
    src_reg_hist: Counter[str] = Counter()
    dst_reg_hist: Counter[str] = Counter()

    hh2 = SpaceSaving(args.ngram_heavyhitters_k)
    hh3 = SpaceSaving(args.ngram_heavyhitters_k)
    hh4 = SpaceSaving(args.ngram_heavyhitters_k)
    total_ngrams_2 = 0
    total_ngrams_3 = 0
    total_ngrams_4 = 0

    block_len_hist: Counter[int] = Counter()
    total_blocks = 0
    two_insn_block_hist: Counter[Tuple[str, str]] = Counter()

    per_file: Dict[str, Dict] = {}
    total_insns = 0

    for p in files:
        file_opcode = Counter()
        file_enc = Counter()
        file_insns = 0

        prev: List[str] = []  # mnemonic stream window for n-grams (max 3 items)
        cur_block_len = 0
        cur_block_prefix: List[str] = []  # first few mnemonics in the current block
        in_block = False

        def _finish_block() -> None:
            nonlocal cur_block_len, in_block, total_blocks
            if not in_block:
                return
            if cur_block_len == 2 and len(cur_block_prefix) >= 2:
                two_insn_block_hist[(cur_block_prefix[0], cur_block_prefix[1])] += 1
            block_len_hist[cur_block_len] += 1
            total_blocks += 1
            cur_block_len = 0
            cur_block_prefix.clear()
            in_block = False

        with _open_text(p) as f:
            for line in f:
                insn = _parse_line_to_insn(line, gpr_names=gpr_names)
                if insn is None or not insn.mnem:
                    continue

                if _is_block_start_mnem(insn.mnem):
                    _finish_block()
                    prev.clear()
                    in_block = True
                    cur_block_len = 0
                    cur_block_prefix.clear()

                file_insns += 1
                total_insns += 1
                opcode_hist[insn.mnem] += 1
                enc_hist[insn.enc_bits] += 1
                file_opcode[insn.mnem] += 1
                file_enc[insn.enc_bits] += 1

                for r in insn.src_gprs:
                    src_reg_hist[r] += 1
                for r in insn.dst_gprs:
                    dst_reg_hist[r] += 1

                if in_block:
                    cur_block_len += 1
                    if len(cur_block_prefix) < 4:
                        cur_block_prefix.append(insn.mnem)

                    # Update n-gram heavy hitters within the current Linx block.
                    mnem = insn.mnem
                    if len(prev) >= 1:
                        hh2.add((prev[-1], mnem))
                        total_ngrams_2 += 1
                    if len(prev) >= 2:
                        hh3.add((prev[-2], prev[-1], mnem))
                        total_ngrams_3 += 1
                    if len(prev) >= 3:
                        hh4.add((prev[-3], prev[-2], prev[-1], mnem))
                        total_ngrams_4 += 1
                    prev.append(mnem)
                    if len(prev) > 3:
                        prev.pop(0)

                    if _is_block_end_mnem(insn.mnem):
                        _finish_block()
                        prev.clear()

        _finish_block()

        per_file[str(p)] = {
            "insns": file_insns,
            "unique_opcodes": len(file_opcode),
            "enc_bits_hist": dict(sorted(file_enc.items())),
            "top_opcodes": file_opcode.most_common(10),
        }

    len_keys = [16, 32, 48, 64]
    len_summary = {k: int(enc_hist.get(k, 0)) for k in len_keys}
    len_summary["other"] = int(sum(v for k, v in enc_hist.items() if k not in len_keys))

    total_src_regs = sum(src_reg_hist.values())
    total_dst_regs = sum(dst_reg_hist.values())

    out_json = {
        "inputs": {
            "roots": args.roots,
            "glob": args.glob,
            "spec": str(spec_path),
            "files": [str(p) for p in files],
            "ngram_heavyhitters_k": args.ngram_heavyhitters_k,
        },
        "totals": {
            "files": len(files),
            "instructions": total_insns,
            "blocks": total_blocks,
        },
        "block_len_hist": dict(sorted((int(k), int(v)) for k, v in block_len_hist.items())),
        "two_insn_blocks": {
            "total": int(sum(two_insn_block_hist.values())),
            "unique": int(len(two_insn_block_hist)),
            "items_top": [([a, b], int(c)) for ((a, b), c) in two_insn_block_hist.most_common(2000)],
        },
        "encoding_bits_hist": dict(sorted(enc_hist.items())),
        "encoding_bits_summary": {
            "16": len_summary[16],
            "32": len_summary[32],
            "48": len_summary[48],
            "64": len_summary[64],
            "other": len_summary["other"],
        },
        "opcode_hist": opcode_hist.most_common(),
        "src_reg_hist": src_reg_hist.most_common(),
        "dst_reg_hist": dst_reg_hist.most_common(),
        "ngrams": {
            "2": {
                "total": total_ngrams_2,
                "items": [(list(k), c, e) for (k, c, e) in hh2.items()],
            },
            "3": {
                "total": total_ngrams_3,
                "items": [(list(k), c, e) for (k, c, e) in hh3.items()],
            },
            "4": {
                "total": total_ngrams_4,
                "items": [(list(k), c, e) for (k, c, e) in hh4.items()],
            },
        },
        "per_file": per_file,
        "notes": {
            "register_model": "GPR names from spec reg5 + pseudo regs like t#1; dest regs inferred from '->reg' tokens.",
            "patterns": "N-grams computed within Linx BSTART-defined blocks (no cross-block patterns); heavy hitters are approximate.",
            "block_boundaries": "Start: mnemonics with segment 'BSTART' (e.g. C.BSTART, HL.BSTART.STD). End: mnemonics with segment 'BSTACK' or 'BSTOP' if present.",
        },
    }

    out_md: List[str] = []
    out_md.append("# LinxISA Objdump Aggregate Stats\n")
    out_md.append(f"- Spec: `{spec_path}`")
    out_md.append(f"- Files: `{len(files)}`")
    out_md.append(f"- Total instructions: `{total_insns}`")
    out_md.append(f"- Total blocks: `{total_blocks}`")
    out_md.append(f"- N-gram heavy hitters capacity (`K`): `{args.ngram_heavyhitters_k}`\n")

    out_md.append("## Block Length Distribution (Top 20)\n")
    out_md.append("| Block insns | Blocks | % |")
    out_md.append("|---:|---:|---:|")
    for bl, cnt in block_len_hist.most_common(20):
        out_md.append(f"| {bl} | {cnt} | {_fmt_pct(cnt, total_blocks)} |")
    out_md.append("")

    out_md.append("## Two-Instruction Blocks (Top 20)\n")
    out_md.append(f"- Total 2-insn blocks: `{sum(two_insn_block_hist.values())}`\n")
    out_md.append("| Block (m0 ; m1) | Blocks | % (of 2-insn blocks) |")
    out_md.append("|---|---:|---:|")
    total_2b = int(sum(two_insn_block_hist.values()))
    for (m0, m1), cnt in two_insn_block_hist.most_common(20):
        out_md.append(f"| `{m0} ; {m1}` | {cnt} | {_fmt_pct(cnt, total_2b)} |")
    out_md.append("")

    out_md.append("## Encoding Length Fractions\n")
    out_md.append("| Length | Count | % |")
    out_md.append("|---|---:|---:|")
    for k in (16, 32, 48, 64):
        v = len_summary[k]
        out_md.append(f"| {k}b | {v} | {_fmt_pct(v, total_insns)} |")
    v = len_summary["other"]
    out_md.append(f"| other | {v} | {_fmt_pct(v, total_insns)} |")
    out_md.append("")

    out_md.append(f"## Opcode Distribution (Top {args.top})\n")
    out_md.append(_top_table(opcode_hist, total=total_insns, top=args.top))
    out_md.append("")

    out_md.append(f"## Source Register Usage (Top {args.top})\n")
    out_md.append(f"- Total source register mentions: `{total_src_regs}`\n")
    out_md.append(_top_table(src_reg_hist, total=total_src_regs, top=args.top))
    out_md.append("")

    out_md.append(f"## Destination Register Usage (Top {args.top})\n")
    out_md.append(f"- Total destination register mentions: `{total_dst_regs}`\n")
    out_md.append(_top_table(dst_reg_hist, total=total_dst_regs, top=args.top))
    out_md.append("")

    out_md.append(f"## Common Instruction Patterns (2-grams, Top {args.top})\n")
    out_md.append(_ngram_table_heavyhitters(hh2.items(), total=total_ngrams_2, top=args.top))
    out_md.append("")

    out_md.append(f"## Common Instruction Patterns (3-grams, Top {args.top})\n")
    out_md.append(_ngram_table_heavyhitters(hh3.items(), total=total_ngrams_3, top=args.top))
    out_md.append("")

    out_md.append(f"## Common Instruction Patterns (4-grams, Top {args.top})\n")
    out_md.append(_ngram_table_heavyhitters(hh4.items(), total=total_ngrams_4, top=args.top))
    out_md.append("")

    out_md.append("## Notes\n")
    out_md.append("- Dest registers are inferred from `->reg` markers in objdump output; some instruction forms may write registers without an explicit `->` token.")
    out_md.append("- Source registers are extracted from operand tokens after removing `->dest` markers; only spec GPR names (reg5) and pseudo spellings like `t#1` are counted.")
    out_md.append("- N-gram patterns are computed within Linx blocks: the n-gram window is reset at `*.BSTART*` mnemonics and after `*.BSTACK*`/`*.BSTOP*` if present.")
    out_md.append("- N-gram patterns use Space-Saving heavy hitters (approximate) to keep memory bounded for very large disassemblies (e.g. Linux `vmlinux`).")
    out_md.append("")

    out_md_path = Path(args.out_md)
    out_md_path.parent.mkdir(parents=True, exist_ok=True)
    out_md_path.write_text("\n".join(out_md) + "\n", encoding="utf-8")

    out_json_path = Path(args.out_json)
    out_json_path.parent.mkdir(parents=True, exist_ok=True)
    out_json_path.write_text(json.dumps(out_json, indent=2) + "\n", encoding="utf-8")

    print(f"ok: wrote {out_md_path}")
    print(f"ok: wrote {out_json_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(__import__("sys").argv[1:]))
