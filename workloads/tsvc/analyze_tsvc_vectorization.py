#!/usr/bin/env python3

from __future__ import annotations

import argparse
import collections
import json
import re
import sys
from pathlib import Path


_RE_FUNC = re.compile(r"^\s*([0-9a-fA-F]+)\s+<([^>]+)>:\s*$")
_RE_BSTART_VEC = re.compile(r"(?i)\bbstart\.(?:mseq|mpar)\b")
_RE_VEC_INSN = re.compile(r"(?i)\bv\.[a-z0-9_]+")
_RE_BTEXT_TARGET = re.compile(r"(?i)\bb\.text\s+([A-Za-z0-9_.$]+)")

_GAP_BUCKET_ORDER = (
    "loop_removed_before_pass",
    "unsupported_value_expression",
    "non_affine_address",
    "inner_control_flow",
    "reductions_live_out",
    "no_store_loops",
    "other",
)

_GAP_BUCKET_ACTIONS: dict[str, str] = {
    "loop_removed_before_pass": "adjust_pass_pipeline_or_loop_preservation",
    "unsupported_value_expression": "extend_emit_value_semantics",
    "non_affine_address": "extend_address_lowering_or_fallback",
    "inner_control_flow": "if_convert_or_predicate_lowering",
    "reductions_live_out": "add_reduction_and_liveout_lowering",
    "no_store_loops": "support_reduction_only_vector_loops",
    "other": "manual_triage",
}


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


def _parse_remarks_jsonl(path: Path | None) -> list[dict[str, object]]:
    if path is None or not path.exists():
        return []
    rows: list[dict[str, object]] = []
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        if not line:
            continue
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(payload, dict):
            rows.append(payload)
    return rows


def _map_reason_to_gap_bucket(reason: str) -> str:
    text = reason.strip()
    if not text:
        return "other"
    if text in {"no_loop_candidate", "no_tripcount_expr", "tripcount_expand_failed"}:
        return "loop_removed_before_pass"
    if text.startswith("unsupported_value_expr:"):
        return "unsupported_value_expression"
    if "non_affine" in text or text in {"unsupported_store_stride"}:
        return "non_affine_address"
    if text in {
        "inner_control_flow",
        "complex_control_flow",
        "not_innermost_loop",
        "not_loop_simplify",
        "preheader_not_simple_branch",
        "unsupported_inner_backedge",
        "unsupported_branch_condition",
        "unsupported_branch_predicate",
        "unsupported_branch_fcmp_condition",
        "unsupported_terminator",
    }:
        return "inner_control_flow"
    if text in {
        "value_live_out",
        "unsupported_reduction_kind",
        "unsupported_reduction_init",
        "unsupported_reduction_value",
    }:
        return "reductions_live_out"
    if text == "no_store_in_loop":
        return "no_store_loops"
    return "other"


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Analyze TSVC auto-vectorization coverage (strict lowered-loop metric).")
    ap.add_argument("--objdump", required=True, help="Objdump text path")
    ap.add_argument("--kernel-list", required=True, help="Kernel list path")
    ap.add_argument("--remarks-jsonl", default=None, help="Raw autovec remarks JSONL path")
    ap.add_argument("--kernel-out-dir", required=True, help="Per-kernel objdump output directory")
    ap.add_argument("--report", required=True, help="Markdown report output path")
    ap.add_argument("--json-out", required=True, help="Coverage JSON output path")
    ap.add_argument("--remarks-summary-out", required=True, help="Per-kernel remarks summary JSON output path")
    ap.add_argument("--gap-plan-out", required=True, help="Gap-plan JSON output path")
    ap.add_argument("--mode", default="auto", help="Mode label")
    ap.add_argument("--fail-under", type=int, default=None, help="Minimum strict-lowered kernels")
    args = ap.parse_args(argv)

    objdump_path = Path(args.objdump)
    kernel_list_path = Path(args.kernel_list)
    remarks_jsonl_path = Path(args.remarks_jsonl) if args.remarks_jsonl else None
    kernel_out_dir = Path(args.kernel_out_dir)
    report_path = Path(args.report)
    json_out_path = Path(args.json_out)
    remarks_summary_out_path = Path(args.remarks_summary_out)
    gap_plan_out_path = Path(args.gap_plan_out)

    if not objdump_path.exists():
        raise SystemExit(f"error: objdump not found: {objdump_path}")
    if not kernel_list_path.exists():
        raise SystemExit(f"error: kernel list not found: {kernel_list_path}")

    kernels = _read_kernel_list(kernel_list_path)
    objdump_text = objdump_path.read_text(encoding="utf-8", errors="replace")
    functions = _split_functions(objdump_text)
    kernel_out_dir.mkdir(parents=True, exist_ok=True)

    asm_by_kernel: dict[str, dict[str, object]] = {}
    missing_functions: list[str] = []

    for kernel in kernels:
        resolved = _lookup_function_name(functions, kernel)
        if resolved is None:
            missing_functions.append(kernel)
            asm_by_kernel[kernel] = {
                "kernel": kernel,
                "resolved_symbol": None,
                "has_vec_block": False,
                "has_vec_insn": False,
                "has_btext": False,
            }
            continue

        root_body = functions[resolved]
        body = _expand_btext_reachable(functions, resolved)
        (kernel_out_dir / f"{kernel}.objdump.txt").write_text(body, encoding="utf-8")
        asm_by_kernel[kernel] = {
            "kernel": kernel,
            "resolved_symbol": resolved,
            "has_vec_block": bool(_RE_BSTART_VEC.search(root_body)),
            "has_vec_insn": bool(_RE_VEC_INSN.search(body)),
            "has_btext": bool(_RE_BTEXT_TARGET.search(root_body)),
        }

    rows = _parse_remarks_jsonl(remarks_jsonl_path)
    by_function: dict[str, list[dict[str, object]]] = collections.defaultdict(list)
    for row in rows:
        fn = str(row.get("function", "")).strip()
        if fn:
            by_function[fn].append(row)

    kernel_rows: list[dict[str, object]] = []
    vectorized: list[str] = []
    non_vectorized: list[str] = []

    for kernel in kernels:
        fn_candidates = (kernel, f"_{kernel}")
        linked_rows: list[dict[str, object]] = []
        for fn in fn_candidates:
            linked_rows.extend(by_function.get(fn, []))

        lowered_rows = [r for r in linked_rows if str(r.get("status", "")) == "lowered"]
        reject_rows = [r for r in linked_rows if str(r.get("status", "")) == "reject"]

        chosen = None
        status = "reject"
        reason = "no_remarks_for_kernel"
        selected_mode = "mseq"
        configured_mode = args.mode
        if lowered_rows:
            chosen = next(
                (r for r in lowered_rows if str(r.get("reason", "")).startswith("lowered_vblock")),
                lowered_rows[0],
            )
            status = "lowered"
            reason = str(chosen.get("reason", "lowered_vblock"))
            selected_mode = str(chosen.get("selected_mode", "mseq"))
            configured_mode = str(chosen.get("configured_mode", args.mode))
        elif reject_rows:
            reason_counts: collections.Counter[str] = collections.Counter(
                str(r.get("reason", "")) for r in reject_rows
            )
            reason = reason_counts.most_common(1)[0][0] if reason_counts else "reject_unknown"
            chosen = reject_rows[0]
            selected_mode = str(chosen.get("selected_mode", "mseq"))
            configured_mode = str(chosen.get("configured_mode", args.mode))

        asm = asm_by_kernel[kernel]
        has_vec_block = bool(asm["has_vec_block"])
        has_vec_insn = bool(asm["has_vec_insn"])
        has_btext = bool(asm["has_btext"])
        has_strict_lowering_reason = status == "lowered" and reason.startswith("lowered_vblock")
        strict_vectorized = has_strict_lowering_reason and has_vec_block and has_vec_insn and has_btext

        if strict_vectorized:
            vectorized.append(kernel)
        else:
            non_vectorized.append(kernel)

        bucket = "lowered" if strict_vectorized else _map_reason_to_gap_bucket(reason)
        kernel_rows.append(
            {
                "kernel": kernel,
                "function_candidates": list(fn_candidates),
                "resolved_symbol": asm["resolved_symbol"],
                "status": status,
                "reason": reason,
                "bucket": bucket,
                "configured_mode": configured_mode,
                "selected_mode": selected_mode,
                "loop_rows_total": len(linked_rows),
                "lowered_loops": len(lowered_rows),
                "reject_loops": len(reject_rows),
                "asm_has_vec_block": has_vec_block,
                "asm_has_btext": has_btext,
                "asm_has_vec_insn": has_vec_insn,
                "strict_vectorized": strict_vectorized,
            }
        )

    total = len(kernels)
    vec_count = len(vectorized)
    non_vec_count = len(non_vectorized)
    coverage = (100.0 * vec_count / total) if total else 0.0

    coverage_payload = {
        "mode": args.mode,
        "metric": "strict_lowered_loops",
        "metric_description": (
            "Vectorized iff remarks report lowered_vblock* and disassembly has "
            "BSTART.MSEQ/MPAR + B.TEXT + reachable v.* body ops."
        ),
        "total": total,
        "vectorized": vec_count,
        "non_vectorized": non_vec_count,
        "coverage_percent": round(coverage, 2),
        "vectorized_kernels": vectorized,
        "non_vectorized_kernels": non_vectorized,
        "missing_functions": missing_functions,
        "objdump": str(objdump_path),
        "remarks_jsonl": str(remarks_jsonl_path) if remarks_jsonl_path else None,
        "kernel_out_dir": str(kernel_out_dir),
    }
    json_out_path.parent.mkdir(parents=True, exist_ok=True)
    json_out_path.write_text(json.dumps(coverage_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    remarks_payload = {
        "mode": args.mode,
        "total_kernels": total,
        "strict_vectorized_kernels": vec_count,
        "rows": kernel_rows,
    }
    remarks_summary_out_path.parent.mkdir(parents=True, exist_ok=True)
    remarks_summary_out_path.write_text(json.dumps(remarks_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    bucket_kernels: dict[str, list[str]] = {bucket: [] for bucket in _GAP_BUCKET_ORDER}
    kernel_plan: list[dict[str, object]] = []
    for row in kernel_rows:
        if bool(row.get("strict_vectorized", False)):
            continue
        kernel = str(row.get("kernel", ""))
        bucket = str(row.get("bucket", "other"))
        if bucket not in bucket_kernels:
            bucket = "other"
        bucket_kernels[bucket].append(kernel)
        kernel_plan.append(
            {
                "kernel": kernel,
                "bucket": bucket,
                "reason": str(row.get("reason", "reject_unknown")),
                "configured_mode": str(row.get("configured_mode", args.mode)),
                "selected_mode": str(row.get("selected_mode", "mseq")),
                "loop_rows_total": int(row.get("loop_rows_total", 0)),
                "next_action": _GAP_BUCKET_ACTIONS.get(bucket, "manual_triage"),
            }
        )

    gap_payload = {
        "mode": args.mode,
        "total_kernels": total,
        "vectorized_kernels": vec_count,
        "non_vectorized_kernels": non_vec_count,
        "missing_functions": sorted(missing_functions),
        "bucket_counts": {bucket: len(bucket_kernels[bucket]) for bucket in _GAP_BUCKET_ORDER},
        "buckets": {bucket: bucket_kernels[bucket] for bucket in _GAP_BUCKET_ORDER},
        "kernel_plan": kernel_plan,
    }
    gap_plan_out_path.parent.mkdir(parents=True, exist_ok=True)
    gap_plan_out_path.write_text(json.dumps(gap_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# TSVC strict auto-vectorization coverage",
        "",
        f"- Mode: `{args.mode}`",
        f"- Objdump: `{objdump_path}`",
        f"- Kernels total: `{total}`",
        f"- Strict vectorized kernels: `{vec_count}`",
        f"- Strict non-vectorized kernels: `{non_vec_count}`",
        f"- Coverage: `{coverage:.2f}%`",
        "",
        "## Strict metric",
        "- Requires both remark-level lowering and decoupled body assembly evidence:",
        "  - `reason` starts with `lowered_vblock`",
        "  - root function has `BSTART.MSEQ`/`BSTART.MPAR` and `B.TEXT`",
        "  - `B.TEXT`-reachable body contains `v.*` operations",
    ]
    if missing_functions:
        lines.extend(["", "## Missing kernel symbols"] + [f"- `{k}`" for k in missing_functions[:64]])
        if len(missing_functions) > 64:
            lines.append(f"- ... ({len(missing_functions) - 64} more)")
    if non_vectorized:
        lines.extend(["", "## Non-vectorized kernels"] + [f"- `{k}`" for k in non_vectorized[:128]])
        if len(non_vectorized) > 128:
            lines.append(f"- ... ({len(non_vectorized) - 128} more)")
    lines.extend(
        [
            "",
            "## Gap buckets",
            *[
                f"- `{bucket}`: `{len(bucket_kernels[bucket])}`"
                for bucket in _GAP_BUCKET_ORDER
            ],
        ]
    )
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    if args.fail_under is not None and vec_count < args.fail_under:
        print(
            f"error: strict coverage gate failed ({vec_count} < {args.fail_under})",
            file=sys.stderr,
        )
        print(f"  report: {report_path}", file=sys.stderr)
        return 2

    print(
        f"ok: strict coverage {vec_count}/{total} ({coverage:.2f}%) -> {report_path}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
