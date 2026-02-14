#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


_RE_FUNC = re.compile(r"^\s*([0-9a-fA-F]+)\s+<([^>]+)>:\s*$")
_RE_BSTART_VEC = re.compile(r"(?i)\bbstart\.(?:mseq|mpar)\b")
_RE_VEC_INSN = re.compile(r"(?i)\bv\.[a-z0-9_]+")
_RE_BTEXT_TARGET = re.compile(r"(?i)\bb\.text\s+([A-Za-z0-9_.$]+)")


def _read_kernel_list(path: Path) -> list[str]:
    kernels: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        name = raw.strip()
        if not name or name.startswith("#"):
            continue
        kernels.append(name)
    if not kernels:
        raise SystemExit(f"error: empty kernel list: {path}")
    return kernels


def _split_functions(objdump_text: str) -> dict[str, str]:
    functions: dict[str, list[str]] = {}
    current: str | None = None
    for line in objdump_text.splitlines():
        m = _RE_FUNC.match(line)
        if m:
            name = m.group(2)
            # llvm-objdump prints basic block/local labels like:
            #   00000000 <.LBB0_1>:
            #   00000000 <.__linx_vblock_body.42>:
            #
            # Treat these as *intra-function* labels, not function boundaries,
            # so we keep the whole kernel function disassembly in one chunk.
            if not name.startswith("."):
                current = name
                functions[current] = [line]
                continue
        if current is not None:
            functions[current].append(line)
    return {name: "\n".join(lines).rstrip() + "\n" for name, lines in functions.items()}


def _lookup_function_name(functions: dict[str, str], kernel: str) -> str | None:
    if kernel in functions:
        return kernel
    prefixed = f"_{kernel}"
    if prefixed in functions:
        return prefixed
    return None


def _expand_btext_reachable(functions: dict[str, str], root: str) -> str:
    visited: set[str] = set()
    worklist: list[str] = [root]
    chunks: list[str] = []

    while worklist:
        name = worklist.pop()
        if name in visited:
            continue
        body = functions.get(name)
        if body is None:
            continue
        visited.add(name)
        chunks.append(body)
        for match in _RE_BTEXT_TARGET.finditer(body):
            target = match.group(1)
            if target not in visited and target in functions:
                worklist.append(target)

    return "\n".join(chunks).rstrip() + "\n"


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Analyze TSVC objdump vectorization coverage.")
    ap.add_argument("--objdump", required=True, help="Objdump text path")
    ap.add_argument("--kernel-list", required=True, help="Kernel list path (one per line)")
    ap.add_argument("--kernel-out-dir", required=True, help="Per-kernel objdump output directory")
    ap.add_argument("--report", required=True, help="Markdown report output path")
    ap.add_argument("--json-out", required=True, help="JSON summary output path")
    ap.add_argument("--mode", default="unknown", help="Mode label for report")
    ap.add_argument("--fail-under", type=int, default=None, help="Minimum vectorized kernels")
    args = ap.parse_args(argv)

    objdump_path = Path(args.objdump)
    kernel_list_path = Path(args.kernel_list)
    kernel_out_dir = Path(args.kernel_out_dir)
    report_path = Path(args.report)
    json_out_path = Path(args.json_out)

    if not objdump_path.exists():
        raise SystemExit(f"error: objdump not found: {objdump_path}")
    if not kernel_list_path.exists():
        raise SystemExit(f"error: kernel list not found: {kernel_list_path}")

    kernels = _read_kernel_list(kernel_list_path)
    objdump_text = objdump_path.read_text(encoding="utf-8", errors="replace")
    functions = _split_functions(objdump_text)

    kernel_out_dir.mkdir(parents=True, exist_ok=True)

    missing_functions: list[str] = []
    vectorized: list[str] = []
    non_vectorized: list[str] = []

    for kernel in kernels:
        resolved = _lookup_function_name(functions, kernel)
        if resolved is None:
            missing_functions.append(kernel)
            continue
        root_body = functions[resolved]
        body = _expand_btext_reachable(functions, resolved)
        (kernel_out_dir / f"{kernel}.objdump.txt").write_text(body, encoding="utf-8")
        has_vec_block = bool(_RE_BSTART_VEC.search(root_body))
        has_vec_insn = bool(_RE_VEC_INSN.search(body))
        if has_vec_block and has_vec_insn:
            vectorized.append(kernel)
        else:
            non_vectorized.append(kernel)

    total = len(kernels)
    vec_count = len(vectorized)
    non_vec_count = len(non_vectorized) + len(missing_functions)
    coverage = (100.0 * vec_count / total) if total else 0.0

    payload = {
        "mode": args.mode,
        "total": total,
        "vectorized": vec_count,
        "non_vectorized": non_vec_count,
        "coverage_percent": round(coverage, 2),
        "vectorized_kernels": vectorized,
        "non_vectorized_kernels": non_vectorized,
        "missing_functions": missing_functions,
        "objdump": str(objdump_path),
        "kernel_out_dir": str(kernel_out_dir),
    }
    json_out_path.parent.mkdir(parents=True, exist_ok=True)
    json_out_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# TSVC vectorization coverage",
        "",
        f"- Mode: `{args.mode}`",
        f"- Objdump: `{objdump_path}`",
        f"- Kernels total: `{total}`",
        f"- Vectorized kernels: `{vec_count}`",
        f"- Non-vectorized kernels: `{non_vec_count}`",
        f"- Coverage: `{coverage:.2f}%`",
        "",
        "## Coverage metric",
        "- A kernel is vectorized iff its function disassembly contains at least one",
        "  `BSTART.MSEQ`/`BSTART.MPAR` and at least one `v.*` instruction.",
        "- For decoupled SIMT lowering, `v.*` is matched from the function body plus",
        "  any `B.TEXT`-reachable local body labels.",
    ]
    if missing_functions:
        lines.extend(["", "## Missing kernel symbols"] + [f"- `{k}`" for k in missing_functions[:64]])
    if non_vectorized:
        lines.extend(["", "## Non-vectorized kernels"] + [f"- `{k}`" for k in non_vectorized[:128]])
    if len(non_vectorized) > 128:
        lines.append(f"- ... ({len(non_vectorized) - 128} more)")
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    if args.fail_under is not None and vec_count < args.fail_under:
        print(
            f"error: vectorized-kernel coverage gate failed ({vec_count} < {args.fail_under})",
            file=sys.stderr,
        )
        print(f"  report: {report_path}", file=sys.stderr)
        return 2

    print(
        f"ok: vectorization coverage {vec_count}/{total} ({coverage:.2f}%) -> {report_path}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
