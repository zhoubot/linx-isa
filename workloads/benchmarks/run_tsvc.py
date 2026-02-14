#!/usr/bin/env python3

from __future__ import annotations

import argparse
import collections
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


BENCH_DIR = Path(__file__).resolve().parent
WORKLOADS_DIR = BENCH_DIR.parent
REPO_ROOT = WORKLOADS_DIR.parent

LIBC_DIR = REPO_ROOT / "toolchain" / "libc"
LIBC_INCLUDE = LIBC_DIR / "include"
LIBC_SRC = LIBC_DIR / "src"
COMPAT_DIR = BENCH_DIR / "common" / "compat"
TSVC_SRC_DIR = BENCH_DIR / "third_party" / "TSVC_2" / "src"

ANALYZE_SCRIPT = BENCH_DIR / "analyze_tsvc_vectorization.py"
COMPARE_SCRIPT = BENCH_DIR / "compare_tsvc_modes.py"

_RE_OBJDUMP_INSN = re.compile(
    r"^\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]{2}(?:\s+[0-9a-fA-F]{2})*)\s+(.*)$"
)
_RE_TSVC_ROW = re.compile(
    r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s+(\S+)\s+(\S+)\s*$"
)

_VECTOR_MODES = ("off", "mseq", "mpar", "auto")

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


@dataclass(frozen=True)
class TsvcRunResult:
    mode: str
    elf: Path
    objdump: Path
    stdout_log: Path
    stderr_log: Path
    static_insn_count: int
    expected_kernels: int
    observed_kernels: int
    checksum_by_kernel: dict[str, str]
    remarks_json_raw: Path | None
    remarks_json: Path | None
    coverage_md: Path | None
    coverage_json: Path | None
    coverage_vectorized: int | None
    gap_plan_json: Path | None


def _run(
    cmd: list[str],
    *,
    cwd: Path | None = None,
    verbose: bool = False,
    **kwargs,
) -> subprocess.CompletedProcess[bytes]:
    if verbose:
        print("+", " ".join(cmd), file=sys.stderr)
    return subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=False, **kwargs)


def _check_exe(path: Path, what: str) -> None:
    if not path.exists():
        raise SystemExit(f"error: {what} not found: {path}")
    if not os.access(path, os.X_OK):
        raise SystemExit(f"error: {what} not executable: {path}")


def _default_clang() -> Path | None:
    env = os.environ.get("CLANG")
    if env:
        return Path(os.path.expanduser(env))
    cand = Path.home() / "llvm-project" / "build-linxisa-clang" / "bin" / "clang"
    return cand if cand.exists() else None


def _default_lld(clang: Path | None) -> Path | None:
    env = os.environ.get("LLD")
    if env:
        return Path(os.path.expanduser(env))
    if clang:
        cand = clang.parent / "ld.lld"
        if cand.exists():
            return cand
    return None


def _default_llvm_tool(clang: Path, tool: str) -> Path | None:
    cand = clang.parent / tool
    return cand if cand.exists() else None


def _default_qemu() -> Path | None:
    env = os.environ.get("QEMU")
    if env:
        return Path(os.path.expanduser(env))
    cand = Path.home() / "qemu" / "build" / "qemu-system-linx64"
    cand_tci = Path.home() / "qemu" / "build-tci" / "qemu-system-linx64"
    if cand.exists():
        return cand
    if cand_tci.exists():
        return cand_tci
    return None


def _cc(
    *,
    clang: Path,
    target: str,
    src: Path,
    obj: Path,
    include_dirs: list[Path],
    cflags_extra: list[str],
    verbose: bool,
) -> None:
    cmd = [
        str(clang),
        "-target",
        target,
        "-O2",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-stack-protector",
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
        "-fno-exceptions",
        "-fno-jump-tables",
        "-nostdlib",
        "-std=gnu11",
        *[f"-I{p}" for p in include_dirs],
        f"-I{LIBC_INCLUDE}",
        *cflags_extra,
        "-c",
        str(src),
        "-o",
        str(obj),
    ]
    p = _run(cmd, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        stderr_text = (p.stderr or b"").decode("utf-8", errors="replace")
        sys.stderr.buffer.write(p.stderr or b"")
        if "-linx-simt-autovec" in stderr_text and "Unknown command line argument" in stderr_text:
            raise SystemExit(
                "error: clang binary does not include Linx SIMT autovec options.\n"
                "hint: rebuild /Users/zhoubot/llvm-project/build-linxisa-clang after syncing LLVM changes,\n"
                "      or run TSVC with --vector-mode off."
            )
        raise SystemExit(f"error: compile failed: {src}")


def _build_runtime_objects(
    *,
    clang: Path,
    target: str,
    out_dir: Path,
    verbose: bool,
) -> list[Path]:
    rt_dir = out_dir / "_runtime"
    rt_dir.mkdir(parents=True, exist_ok=True)

    include_dirs = [BENCH_DIR, COMPAT_DIR]
    cflags_extra: list[str] = []

    def compile_runtime(src: Path, obj_name: str) -> Path:
        obj = rt_dir / obj_name
        _cc(
            clang=clang,
            target=target,
            src=src,
            obj=obj,
            include_dirs=include_dirs,
            cflags_extra=cflags_extra,
            verbose=verbose,
        )
        return obj

    objs: list[Path] = []
    objs.append(compile_runtime(BENCH_DIR / "common" / "startup.c", "startup.o"))
    objs.append(compile_runtime(LIBC_SRC / "syscall.c", "syscall.o"))
    objs.append(compile_runtime(LIBC_SRC / "stdio" / "stdio.c", "stdio.o"))
    objs.append(compile_runtime(LIBC_SRC / "stdlib" / "stdlib.c", "stdlib.o"))
    objs.append(compile_runtime(LIBC_SRC / "string" / "mem.c", "mem.o"))
    objs.append(compile_runtime(LIBC_SRC / "string" / "str.c", "str.o"))
    objs.append(compile_runtime(LIBC_SRC / "math" / "math.c", "math.o"))
    objs.append(compile_runtime(COMPAT_DIR / "linx_compat.c", "linx_compat.o"))
    return objs


def _rewrite_macro(text: str, macro: str, value: int) -> str:
    pattern = rf"^\s*#define\s+{re.escape(macro)}\s+\d+\s*$"
    repl = f"#define {macro} {value}"
    out, n = re.subn(pattern, repl, text, flags=re.MULTILINE)
    if n == 0:
        raise SystemExit(f"error: failed to patch {macro} in TSVC common.h")
    return out


def _extract_kernel_names(tsvc_text: str) -> list[str]:
    names = re.findall(r"time_function\(&([A-Za-z_][A-Za-z0-9_]*)\s*,", tsvc_text)
    if not names:
        raise SystemExit("error: failed to extract TSVC kernel list")
    seen: set[str] = set()
    ordered: list[str] = []
    for name in names:
        if name in seen:
            continue
        seen.add(name)
        ordered.append(name)
    return ordered


def _stage_tsvc_sources(
    *,
    src_dir: Path,
    stage_dir: Path,
    iterations: int,
    len_1d: int,
    len_2d: int,
    kernel_regex: str | None,
) -> list[str]:
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    shutil.copytree(src_dir, stage_dir)

    common_h = stage_dir / "common.h"
    common_text = common_h.read_text(encoding="utf-8")
    common_text = _rewrite_macro(common_text, "iterations", iterations)
    common_text = _rewrite_macro(common_text, "LEN_1D", len_1d)
    common_text = _rewrite_macro(common_text, "LEN_2D", len_2d)
    common_h.write_text(common_text, encoding="utf-8")

    tsvc_c = stage_dir / "tsvc.c"
    tsvc_text = tsvc_c.read_text(encoding="utf-8")

    # Temporary bring-up workaround:
    # Keep `s318` out of aggressive optimization to avoid a known backend
    # cross-BB shifted-operand fold issue that can introduce an undef stride
    # register in this kernel. The full LLVM-side fix lives in
    # `LinxISABlockify.cpp`; this staging patch keeps TSVC correctness stable
    # until all toolchain binaries are rebuilt with that fix.
    tsvc_text = re.sub(
        r"(?m)^real_t\s+s318\s*\(\s*struct\s+args_t\s*\*\s*func_args\s*\)",
        "__attribute__((optnone,noinline)) real_t s318(struct args_t * func_args)",
        tsvc_text,
        count=1,
    )

    kernels = _extract_kernel_names(tsvc_text)

    keep: set[str] | None = None
    if kernel_regex:
        try:
            pattern = re.compile(kernel_regex)
        except re.error as e:
            raise SystemExit(f"error: invalid --kernel-regex: {e}") from e
        keep = {k for k in kernels if pattern.search(k)}
        if not keep:
            raise SystemExit(f"error: --kernel-regex matched 0 kernels: {kernel_regex}")

    if "<stdint.h>" not in tsvc_text:
        tsvc_text = tsvc_text.replace(
            "#include <sys/time.h>\n",
            "#include <sys/time.h>\n#include <stdint.h>\n",
        )

    # TSVC prints float timing/checksum via %f, but our minimal libc does not
    # support float formatting. Patch to a stable integer+hex format so we can
    # compare across modes.
    time_func_pat = re.compile(
        r"void\s+time_function\s*\(\s*test_function_t\s+vector_func\s*,\s*void\s*\*\s*arg_info\s*\)\s*\{.*?\n\}\n",
        flags=re.DOTALL,
    )
    time_func_repl = (
        "void time_function(test_function_t vector_func, void * arg_info)\n"
        "{\n"
        "    struct args_t func_args = {.arg_info=arg_info};\n"
        "\n"
        "    real_t result = vector_func(&func_args);\n"
        "\n"
        "    uint64_t t1_us = (uint64_t)func_args.t1.tv_sec * 1000000ull + (uint64_t)func_args.t1.tv_usec;\n"
        "    uint64_t t2_us = (uint64_t)func_args.t2.tv_sec * 1000000ull + (uint64_t)func_args.t2.tv_usec;\n"
        "    uint64_t taken_us = t2_us - t1_us;\n"
        "\n"
        "    union { real_t f; uint32_t u; } bits;\n"
        "    bits.f = result;\n"
        "\n"
        "    printf(\"%llu\\\\t0x%08x\\\\n\", (unsigned long long)taken_us, bits.u);\n"
        "}\n"
    )
    tsvc_text, n = time_func_pat.subn(time_func_repl, tsvc_text)
    if n != 1:
        raise SystemExit(f"error: expected to patch exactly 1 time_function, got {n}")

    if keep is not None:
        new_lines: list[str] = []
        for line in tsvc_text.splitlines():
            m = re.match(r"^\s*time_function\(&([A-Za-z_][A-Za-z0-9_]*)\s*,", line)
            if m and m.group(1) not in keep:
                new_lines.append(f"    // skipped by --kernel-regex: {line.strip()}")
            else:
                new_lines.append(line)
        tsvc_text = "\n".join(new_lines) + "\n"

        kernels = [k for k in kernels if k in keep]

    tsvc_c.write_text(tsvc_text, encoding="utf-8")
    return kernels


def _parse_objdump_count(objdump_text: str) -> int:
    count = 0
    for line in objdump_text.splitlines():
        if _RE_OBJDUMP_INSN.match(line):
            count += 1
    return count


def _mode_to_autovec(mode: str) -> str | None:
    if mode == "off":
        return None
    if mode == "mseq":
        return "mseq"
    if mode == "mpar":
        return "mpar-safe"
    if mode == "auto":
        return "auto"
    raise SystemExit(f"error: unsupported vector mode: {mode}")


def _tsvc_cflags(mode: str, remarks_json: Path | None) -> list[str]:
    flags: list[str] = []
    autovec_mode = _mode_to_autovec(mode)
    if autovec_mode is None:
        flags.extend(
            [
                "-fno-vectorize",
                "-fno-slp-vectorize",
                # Keep Linx SIMT autovec disabled for the scalar baseline.
                "-mllvm",
                "-linx-simt-autovec=0",
            ]
        )
    else:
        flags.extend(
            [
                # Keep generic LLVM vectorizers disabled for TSVC bring-up so
                # coverage/correctness are attributed to Linx SIMT autovec.
                "-fno-vectorize",
                "-fno-slp-vectorize",
                "-mllvm",
                "-linx-simt-autovec=1",
                "-mllvm",
                f"-linx-simt-autovec-mode={autovec_mode}",
            ]
        )
        if remarks_json is not None:
            flags.extend(
                [
                    "-mllvm",
                    f"-linx-simt-autovec-remarks={remarks_json}",
                ]
            )
    return flags


def _parse_kernel_checksums(stdout_text: str, expected_kernels: list[str]) -> dict[str, str]:
    expected = set(expected_kernels)
    checksums: dict[str, str] = {}
    for line in stdout_text.splitlines():
        m = _RE_TSVC_ROW.match(line)
        if not m:
            continue
        kernel, _timing, checksum = m.group(1), m.group(2), m.group(3)
        if kernel in expected and kernel not in checksums:
            checksums[kernel] = checksum
    return checksums


def _map_reason_to_gap_bucket(reason: str) -> str:
    text = reason.strip()
    if not text:
        return "other"
    if text in {"no_loop_candidate", "no_loop_found", "loop_removed_before_pass"}:
        return "loop_removed_before_pass"
    if text in {"no_const_tripcount", "no_tripcount_expr", "tripcount_expand_failed"}:
        return "loop_removed_before_pass"
    if text.startswith("unsupported_value_expr:"):
        return "unsupported_value_expression"
    if "non_affine" in text:
        return "non_affine_address"
    if text in {
        "inner_control_flow",
        "complex_control_flow",
        "not_innermost_loop",
        "not_loop_simplify",
        "preheader_not_simple_branch",
        "exit_has_phi",
    }:
        return "inner_control_flow"
    if text == "value_live_out":
        return "reductions_live_out"
    if text == "no_store_in_loop":
        return "no_store_loops"
    return "other"


def _synthesize_remarks_summary(
    *,
    mode: str,
    expected_kernels: list[str],
    remarks_json_raw: Path | None,
    out_json: Path,
) -> None:
    rows: list[dict[str, object]] = []
    if remarks_json_raw and remarks_json_raw.exists():
        for raw in remarks_json_raw.read_text(encoding="utf-8", errors="replace").splitlines():
            line = raw.strip()
            if not line:
                continue
            try:
                payload = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(payload, dict):
                rows.append(payload)

    by_function: dict[str, list[dict[str, object]]] = collections.defaultdict(list)
    for row in rows:
        fn = str(row.get("function", "")).strip()
        if fn:
            by_function[fn].append(row)

    kernel_rows: list[dict[str, object]] = []
    for kernel in expected_kernels:
        fn_candidates = (kernel, f"_{kernel}")
        linked_rows: list[dict[str, object]] = []
        for fn in fn_candidates:
            linked_rows.extend(by_function.get(fn, []))

        lowered_rows = [r for r in linked_rows if str(r.get("status", "")) == "lowered"]
        reject_rows = [r for r in linked_rows if str(r.get("status", "")) == "reject"]

        status = "reject"
        reason = "loop_removed_before_pass"
        selected_mode = "mseq"
        configured_mode = mode

        if lowered_rows:
            status = "lowered"
            chosen = lowered_rows[0]
            reason = str(chosen.get("reason", "lowered_vblock"))
            selected_mode = str(chosen.get("selected_mode", "mseq"))
            configured_mode = str(chosen.get("configured_mode", mode))
        elif reject_rows:
            reason_counts: collections.Counter[str] = collections.Counter(
                str(r.get("reason", "")) for r in reject_rows
            )
            reason = reason_counts.most_common(1)[0][0] if reason_counts else "reject_unknown"
            chosen = reject_rows[0]
            selected_mode = str(chosen.get("selected_mode", "mseq"))
            configured_mode = str(chosen.get("configured_mode", mode))

        bucket = "lowered" if status == "lowered" else _map_reason_to_gap_bucket(reason)
        kernel_rows.append(
            {
                "kernel": kernel,
                "function_candidates": list(fn_candidates),
                "status": status,
                "reason": reason,
                "bucket": bucket,
                "configured_mode": configured_mode,
                "selected_mode": selected_mode,
                "loop_rows_total": len(linked_rows),
                "lowered_loops": len(lowered_rows),
                "reject_loops": len(reject_rows),
            }
        )

    lowered_count = sum(1 for row in kernel_rows if row["status"] == "lowered")
    payload = {
        "mode": mode,
        "total_kernels": len(expected_kernels),
        "lowered_kernels": lowered_count,
        "rejected_kernels": len(kernel_rows) - lowered_count,
        "rows": kernel_rows,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _write_gap_plan_json(*, remarks_summary_json: Path, coverage_json: Path, out_json: Path) -> None:
    coverage = json.loads(coverage_json.read_text(encoding="utf-8"))
    remarks = json.loads(remarks_summary_json.read_text(encoding="utf-8"))
    rows = list(remarks.get("rows", []))

    non_vectorized = set(str(k) for k in coverage.get("non_vectorized_kernels", []))
    missing = set(str(k) for k in coverage.get("missing_functions", []))
    target = non_vectorized | missing

    bucket_kernels: dict[str, list[str]] = {bucket: [] for bucket in _GAP_BUCKET_ORDER}
    per_kernel: list[dict[str, object]] = []
    row_by_kernel = {str(row.get("kernel", "")): row for row in rows}

    for kernel in sorted(target):
        row = row_by_kernel.get(kernel)
        if row is None:
            bucket = "loop_removed_before_pass"
            reason = "loop_removed_before_pass"
            selected_mode = "mseq"
            configured_mode = str(remarks.get("mode", "auto"))
            loop_rows_total = 0
        else:
            bucket = str(row.get("bucket", "other"))
            reason = str(row.get("reason", "reject_unknown"))
            selected_mode = str(row.get("selected_mode", "mseq"))
            configured_mode = str(row.get("configured_mode", remarks.get("mode", "auto")))
            loop_rows_total = int(row.get("loop_rows_total", 0))
            if bucket == "lowered":
                bucket = "other"

        if bucket not in bucket_kernels:
            bucket = "other"
        bucket_kernels[bucket].append(kernel)
        per_kernel.append(
            {
                "kernel": kernel,
                "bucket": bucket,
                "reason": reason,
                "configured_mode": configured_mode,
                "selected_mode": selected_mode,
                "loop_rows_total": loop_rows_total,
                "next_action": _GAP_BUCKET_ACTIONS.get(bucket, "manual_triage"),
            }
        )

    payload = {
        "mode": str(remarks.get("mode", "auto")),
        "total_kernels": int(coverage.get("total", 0)),
        "vectorized_kernels": int(coverage.get("vectorized", 0)),
        "non_vectorized_kernels": int(coverage.get("non_vectorized", 0)),
        "missing_functions": sorted(missing),
        "bucket_counts": {bucket: len(bucket_kernels[bucket]) for bucket in _GAP_BUCKET_ORDER},
        "buckets": {bucket: bucket_kernels[bucket] for bucket in _GAP_BUCKET_ORDER},
        "kernel_plan": per_kernel,
    }
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _run_tsvc_mode(
    *,
    clang: Path,
    lld: Path,
    llvm_objdump: Path,
    qemu: Path,
    target: str,
    stage_dir: Path,
    expected_kernels: list[str],
    out_dir: Path,
    artifacts_dir: Path,
    runtime_objs: list[Path],
    mode: str,
    timeout_s: float,
    verbose: bool,
    remarks_json_raw: Path | None,
) -> TsvcRunResult:
    obj_dir = out_dir / "tsvc" / mode / "obj"
    obj_dir.mkdir(parents=True, exist_ok=True)

    include_dirs = [stage_dir, BENCH_DIR, COMPAT_DIR]
    tsvc_flags = _tsvc_cflags(mode, remarks_json_raw)
    # Keep the TSVC runtime/support sources scalar. In practice these files
    # contain heavy init/control-flow code that is not the target of TSVC loop
    # benchmarking and may trigger unrelated target bring-up bugs.
    runtime_flags = _tsvc_cflags("off", None)

    objects: list[Path] = []
    for src in (stage_dir / "tsvc.c", stage_dir / "common.c", stage_dir / "dummy.c"):
        obj = obj_dir / f"{src.stem}.o"
        cflags = tsvc_flags if src.name == "tsvc.c" else runtime_flags
        _cc(
            clang=clang,
            target=target,
            src=src,
            obj=obj,
            include_dirs=include_dirs,
            cflags_extra=cflags,
            verbose=verbose,
        )
        objects.append(obj)

    artifacts_dir.mkdir(parents=True, exist_ok=True)
    elf_dir = artifacts_dir / "elf" / "tsvc"
    objdump_dir = artifacts_dir / "objdump" / "tsvc"
    qemu_dir = artifacts_dir / "qemu" / "tsvc"
    reports_dir = artifacts_dir / "reports" / "tsvc"
    elf_dir.mkdir(parents=True, exist_ok=True)
    objdump_dir.mkdir(parents=True, exist_ok=True)
    qemu_dir.mkdir(parents=True, exist_ok=True)
    reports_dir.mkdir(parents=True, exist_ok=True)

    elf = elf_dir / f"tsvc.{mode}.elf"
    link_cmd = [
        str(lld),
        "--entry=_start",
        "-o",
        str(elf),
        *[str(o) for o in runtime_objs],
        *[str(o) for o in objects],
    ]
    p = _run(link_cmd, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stderr)
        raise SystemExit(f"error: TSVC link failed ({mode})")

    objdump_path = objdump_dir / f"tsvc.{mode}.objdump.txt"
    p = _run(
        [str(llvm_objdump), "-d", f"--triple={target}", str(elf)],
        verbose=verbose,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stderr)
        raise SystemExit(f"error: llvm-objdump failed for TSVC ({mode})")
    objdump_path.write_bytes(p.stdout or b"")

    stdout_log = qemu_dir / f"tsvc.{mode}.stdout.txt"
    stderr_log = qemu_dir / f"tsvc.{mode}.stderr.txt"
    qemu_cmd = [
        str(qemu),
        "-machine",
        "virt",
        "-kernel",
        str(elf),
        "-nographic",
        "-monitor",
        "none",
    ]
    try:
        p = _run(
            qemu_cmd,
            verbose=verbose,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout_s,
        )
    except subprocess.TimeoutExpired as e:
        stdout_log.write_bytes(e.stdout or b"")
        stderr_log.write_bytes(e.stderr or b"")
        raise SystemExit(f"error: QEMU timeout after {timeout_s:.1f}s (TSVC {mode})")

    stdout_log.write_bytes(p.stdout or b"")
    stderr_log.write_bytes(p.stderr or b"")
    if p.returncode != 0:
        raise SystemExit(
            f"error: TSVC failed on QEMU (mode={mode}, exit={p.returncode})\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )

    out_text = (p.stdout or b"").decode("utf-8", errors="replace")
    if "Loop" not in out_text or "Checksum" not in out_text:
        raise SystemExit(
            f"error: TSVC output missing header (mode={mode})\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )

    checksum_by_kernel = _parse_kernel_checksums(out_text, expected_kernels)
    missing = [k for k in expected_kernels if k not in checksum_by_kernel]
    if missing:
        preview = ", ".join(missing[:8])
        raise SystemExit(
            f"error: TSVC did not execute all kernels in mode={mode} ({len(missing)} missing)\n"
            f"  missing sample: {preview}\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )

    objdump_text = objdump_path.read_text(encoding="utf-8", errors="replace")
    static_count = _parse_objdump_count(objdump_text)

    return TsvcRunResult(
        mode=mode,
        elf=elf,
        objdump=objdump_path,
        stdout_log=stdout_log,
        stderr_log=stderr_log,
        static_insn_count=static_count,
        expected_kernels=len(expected_kernels),
        observed_kernels=len(checksum_by_kernel),
        checksum_by_kernel=checksum_by_kernel,
        remarks_json_raw=remarks_json_raw,
        remarks_json=None,
        coverage_md=None,
        coverage_json=None,
        coverage_vectorized=None,
        gap_plan_json=None,
    )


def _run_coverage_analyzer(
    *,
    python: str,
    mode_result: TsvcRunResult,
    kernel_list_path: Path,
    kernel_out_dir: Path,
    coverage_md: Path,
    coverage_json: Path,
    coverage_fail_under: int | None,
    verbose: bool,
) -> int:
    cmd = [
        python,
        str(ANALYZE_SCRIPT),
        "--objdump",
        str(mode_result.objdump),
        "--kernel-list",
        str(kernel_list_path),
        "--kernel-out-dir",
        str(kernel_out_dir),
        "--report",
        str(coverage_md),
        "--json-out",
        str(coverage_json),
        "--mode",
        mode_result.mode,
    ]
    if coverage_fail_under is not None:
        cmd += ["--fail-under", str(coverage_fail_under)]
    p = _run(cmd, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stdout or b"")
        sys.stderr.buffer.write(p.stderr or b"")
        raise SystemExit(f"error: TSVC vectorization coverage analysis failed ({mode_result.mode})")
    payload = json.loads(coverage_json.read_text(encoding="utf-8"))
    vectorized = int(payload.get("vectorized", 0))
    return vectorized


def _write_mode_report(
    *,
    report_path: Path,
    result: TsvcRunResult,
    iterations: int,
    len_1d: int,
    len_2d: int,
) -> None:
    lines = [
        "# TSVC on LinxISA QEMU",
        "",
        f"- Mode: `{result.mode}`",
        f"- Profile: `iterations={iterations}`, `LEN_1D={len_1d}`, `LEN_2D={len_2d}`",
        f"- Kernels executed: `{result.observed_kernels}/{result.expected_kernels}`",
        f"- Static instruction count: `{result.static_insn_count}`",
    ]
    if result.coverage_vectorized is not None:
        lines.append(
            f"- Vectorized kernels (disassembly metric): `{result.coverage_vectorized}/{result.expected_kernels}`"
        )
    lines.extend(
        [
            "",
            "## Artifacts",
            f"- ELF: `{result.elf}`",
            f"- Objdump: `{result.objdump}`",
            f"- QEMU stdout: `{result.stdout_log}`",
            f"- QEMU stderr: `{result.stderr_log}`",
        ]
    )
    if result.remarks_json:
        lines.append(f"- Autovec remarks (per-kernel): `{result.remarks_json}`")
    if result.remarks_json_raw:
        lines.append(f"- Autovec raw remarks (JSONL): `{result.remarks_json_raw}`")
    if result.coverage_md:
        lines.append(f"- Coverage report: `{result.coverage_md}`")
    if result.coverage_json:
        lines.append(f"- Coverage JSON: `{result.coverage_json}`")
    if result.gap_plan_json:
        lines.append(f"- Gap plan JSON: `{result.gap_plan_json}`")
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _copy_if_exists(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(src, dst)


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Build and run TSVC on LinxISA QEMU.")
    ap.add_argument("--clang", default=None, help="Path to clang (env: CLANG)")
    ap.add_argument("--lld", default=None, help="Path to ld.lld (env: LLD)")
    ap.add_argument("--qemu", default=None, help="Path to qemu-system-linx64 (env: QEMU)")
    ap.add_argument("--target", default="linx64-linx-none-elf", help="Target triple")
    ap.add_argument("--iterations", type=int, default=32)
    ap.add_argument("--len-1d", type=int, default=320)
    ap.add_argument("--len-2d", type=int, default=16)
    ap.add_argument(
        "--vector-mode",
        choices=[*_VECTOR_MODES, "all"],
        default="auto",
        help="TSVC run mode (`all` runs off+mseq+mpar+auto).",
    )
    ap.add_argument("--timeout", type=float, default=180.0, help="QEMU timeout (seconds)")
    ap.add_argument("--coverage-fail-under", type=int, default=151)
    ap.add_argument(
        "--kernel-regex",
        default=None,
        help="Only run kernels whose names match this regex (Python `re`).",
    )
    ap.add_argument(
        "--no-coverage-gate",
        action="store_true",
        help="Do not fail when vectorized-kernel coverage is below --coverage-fail-under.",
    )
    ap.add_argument("--verbose", "-v", action="store_true")
    args = ap.parse_args(argv)

    if args.len_1d % 40 != 0:
        raise SystemExit("error: --len-1d must be a multiple of 40")
    if args.iterations <= 0 or args.len_1d <= 0 or args.len_2d <= 0:
        raise SystemExit("error: iterations/len values must be > 0")

    if not TSVC_SRC_DIR.exists():
        raise SystemExit(
            f"error: TSVC sources not found at {TSVC_SRC_DIR}\n"
            "hint: run workloads/benchmarks/fetch_third_party.sh"
        )
    if not ANALYZE_SCRIPT.exists():
        raise SystemExit(f"error: missing analyzer: {ANALYZE_SCRIPT}")
    if not COMPARE_SCRIPT.exists():
        raise SystemExit(f"error: missing compare script: {COMPARE_SCRIPT}")

    clang = Path(os.path.expanduser(args.clang)) if args.clang else (_default_clang() or None)
    if not clang:
        raise SystemExit("error: clang not found; set --clang or CLANG")
    lld = Path(os.path.expanduser(args.lld)) if args.lld else (_default_lld(clang) or None)
    if not lld:
        raise SystemExit("error: ld.lld not found; set --lld or LLD")
    qemu = Path(os.path.expanduser(args.qemu)) if args.qemu else (_default_qemu() or None)
    if not qemu:
        raise SystemExit("error: qemu-system-linx64 not found; set --qemu or QEMU")
    llvm_objdump = _default_llvm_tool(clang, "llvm-objdump")
    if not llvm_objdump:
        raise SystemExit("error: llvm-objdump not found next to clang")

    _check_exe(clang, "clang")
    _check_exe(lld, "ld.lld")
    _check_exe(qemu, "qemu-system-linx64")
    _check_exe(llvm_objdump, "llvm-objdump")

    generated_dir = WORKLOADS_DIR / "generated"
    out_dir = generated_dir / "build"
    out_dir.mkdir(parents=True, exist_ok=True)

    stage_dir = out_dir / "tsvc_stage"
    expected_kernels = _stage_tsvc_sources(
        src_dir=TSVC_SRC_DIR,
        stage_dir=stage_dir,
        iterations=args.iterations,
        len_1d=args.len_1d,
        len_2d=args.len_2d,
        kernel_regex=args.kernel_regex,
    )

    reports_dir = generated_dir / "reports" / "tsvc"
    reports_dir.mkdir(parents=True, exist_ok=True)
    kernel_list_path = reports_dir / "kernel_list.txt"
    kernel_list_path.write_text("\n".join(expected_kernels) + "\n", encoding="utf-8")

    runtime_objs = _build_runtime_objects(
        clang=clang,
        target=args.target,
        out_dir=out_dir,
        verbose=args.verbose,
    )

    if args.vector_mode == "all":
        modes = list(_VECTOR_MODES)
    else:
        modes = [args.vector_mode]

    results: dict[str, TsvcRunResult] = {}
    python = sys.executable or "python3"
    for mode in modes:
        remarks_json_raw = None
        if mode != "off":
            remarks_json_raw = reports_dir / f"vectorization_remarks_raw.{mode}.jsonl"
            if remarks_json_raw.exists():
                remarks_json_raw.unlink()
        result = _run_tsvc_mode(
            clang=clang,
            lld=lld,
            llvm_objdump=llvm_objdump,
            qemu=qemu,
            target=args.target,
            stage_dir=stage_dir,
            expected_kernels=expected_kernels,
            out_dir=out_dir,
            artifacts_dir=generated_dir,
            runtime_objs=runtime_objs,
            mode=mode,
            timeout_s=args.timeout,
            verbose=args.verbose,
            remarks_json_raw=remarks_json_raw,
        )

        coverage_md = reports_dir / f"vectorization_coverage.{mode}.md"
        coverage_json = reports_dir / f"vectorization_coverage.{mode}.json"
        kernel_out_dir = generated_dir / "objdump" / "tsvc" / "kernels" / mode
        fail_under = None
        if mode != "off" and not args.no_coverage_gate:
            fail_under = args.coverage_fail_under
        vectorized = _run_coverage_analyzer(
            python=python,
            mode_result=result,
            kernel_list_path=kernel_list_path,
            kernel_out_dir=kernel_out_dir,
            coverage_md=coverage_md,
            coverage_json=coverage_json,
            coverage_fail_under=fail_under,
            verbose=args.verbose,
        )
        remarks_json = None
        gap_plan_json = None
        if mode != "off":
            remarks_json = reports_dir / f"vectorization_remarks.{mode}.json"
            _synthesize_remarks_summary(
                mode=mode,
                expected_kernels=expected_kernels,
                remarks_json_raw=result.remarks_json_raw,
                out_json=remarks_json,
            )
            gap_plan_json = reports_dir / f"vectorization_gap_plan.{mode}.json"
            _write_gap_plan_json(
                remarks_summary_json=remarks_json,
                coverage_json=coverage_json,
                out_json=gap_plan_json,
            )
        result = TsvcRunResult(
            mode=result.mode,
            elf=result.elf,
            objdump=result.objdump,
            stdout_log=result.stdout_log,
            stderr_log=result.stderr_log,
            static_insn_count=result.static_insn_count,
            expected_kernels=result.expected_kernels,
            observed_kernels=result.observed_kernels,
            checksum_by_kernel=result.checksum_by_kernel,
            remarks_json_raw=result.remarks_json_raw,
            remarks_json=remarks_json,
            coverage_md=coverage_md,
            coverage_json=coverage_json,
            coverage_vectorized=vectorized,
            gap_plan_json=gap_plan_json,
        )
        mode_report = reports_dir / f"tsvc.{mode}.report.md"
        _write_mode_report(
            report_path=mode_report,
            result=result,
            iterations=args.iterations,
            len_1d=args.len_1d,
            len_2d=args.len_2d,
        )
        results[mode] = result

    compare_report = reports_dir / "tsvc_mode_compare.md"
    if "off" in results and len(results) > 1:
        cmd = [
            python,
            str(COMPARE_SCRIPT),
            "--kernel-list",
            str(kernel_list_path),
            "--baseline",
            f"off={results['off'].stdout_log}",
            "--report",
            str(compare_report),
        ]
        for mode in modes:
            if mode == "off":
                continue
            cmd += ["--candidate", f"{mode}={results[mode].stdout_log}"]
        p = _run(cmd, verbose=args.verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if p.returncode != 0:
            sys.stderr.buffer.write(p.stdout or b"")
            sys.stderr.buffer.write(p.stderr or b"")
            raise SystemExit("error: TSVC mode comparison failed")

    selected_mode = "auto" if "auto" in results else modes[-1]
    selected = results[selected_mode]
    if selected.coverage_md:
        _copy_if_exists(selected.coverage_md, reports_dir / "vectorization_coverage.md")
    if selected.coverage_json:
        _copy_if_exists(selected.coverage_json, reports_dir / "vectorization_coverage.json")
    if selected.remarks_json:
        _copy_if_exists(selected.remarks_json, reports_dir / "vectorization_remarks.json")
    if selected.remarks_json_raw:
        _copy_if_exists(selected.remarks_json_raw, reports_dir / "vectorization_remarks_raw.jsonl")
    if selected.gap_plan_json:
        _copy_if_exists(selected.gap_plan_json, reports_dir / "vectorization_gap_plan.json")

    if (generated_dir / "objdump" / "tsvc" / "kernels" / selected_mode).is_dir():
        dst_kernels = generated_dir / "objdump" / "tsvc" / "kernels"
        src_kernels = dst_kernels / selected_mode
        for src in src_kernels.glob("*.objdump.txt"):
            _copy_if_exists(src, dst_kernels / src.name)

    summary_path = generated_dir / "tsvc_report.md"
    lines = [
        "# TSVC on LinxISA QEMU",
        "",
        f"- Profile: `iterations={args.iterations}`, `LEN_1D={args.len_1d}`, `LEN_2D={args.len_2d}`",
        f"- Modes run: `{', '.join(modes)}`",
        f"- Coverage gate: `>= {args.coverage_fail_under}` kernels",
        "",
        "## Mode results",
    ]
    for mode in modes:
        r = results[mode]
        cov = "n/a" if r.coverage_vectorized is None else str(r.coverage_vectorized)
        lines.append(
            f"- `{mode}`: kernels `{r.observed_kernels}/{r.expected_kernels}`, "
            f"vectorized `{cov}/{r.expected_kernels}`, "
            f"objdump `{r.objdump}`"
        )
    lines.extend(
        [
            "",
            "## Reports",
            f"- Coverage (selected mode): `{reports_dir / 'vectorization_coverage.md'}`",
            f"- Coverage JSON (selected mode): `{reports_dir / 'vectorization_coverage.json'}`",
            f"- Remarks JSON (selected mode): `{reports_dir / 'vectorization_remarks.json'}`",
            f"- Remarks raw JSONL (selected mode): `{reports_dir / 'vectorization_remarks_raw.jsonl'}`",
            f"- Gap plan JSON (selected mode): `{reports_dir / 'vectorization_gap_plan.json'}`",
        ]
    )
    if compare_report.exists():
        lines.append(f"- Mode comparison: `{compare_report}`")
    summary_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"ok: wrote {summary_path}")
    for mode in modes:
        r = results[mode]
        print(
            f"ok: mode={mode} kernels={r.observed_kernels}/{r.expected_kernels} "
            f"vectorized={r.coverage_vectorized}/{r.expected_kernels}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
