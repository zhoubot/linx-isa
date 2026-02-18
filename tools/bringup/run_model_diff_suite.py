#!/usr/bin/env python3
"""
Run deterministic QEMU-vs-model differential suite for Linx bring-up.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

RELEASE_STRICT_REQUIRED_CATEGORIES = {
    "scalar_basic",
    "vector_lane_control",
    "tile_descriptor_legality",
    "tile_control_flow",
    "privileged_exception_edge",
}


def _load_yaml_or_json(path: Path) -> dict[str, Any]:
    text = path.read_text(encoding="utf-8")
    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        try:
            import yaml  # type: ignore
        except Exception as exc:
            if path.name == "linx_model_diff_suite.yaml":
                return _parse_suite_yaml_without_pyyaml(text, path)
            raise SystemExit(
                f"error: failed to parse {path} as JSON and PyYAML unavailable: {exc}"
            ) from exc
        data = yaml.safe_load(text)
    if not isinstance(data, dict):
        raise SystemExit(f"error: {path} must decode to an object")
    return data


def _parse_scalar(value: str) -> Any:
    v = value.strip()
    if not v:
        return ""
    if v.startswith('"') and v.endswith('"') and len(v) >= 2:
        return v[1:-1]
    if v.startswith("'") and v.endswith("'") and len(v) >= 2:
        return v[1:-1]
    if v.startswith("[") and v.endswith("]"):
        body = v[1:-1].strip()
        if not body:
            return []
        out: list[Any] = []
        for item in body.split(","):
            tok = item.strip().strip('"').strip("'")
            if re.fullmatch(r"-?\d+", tok):
                out.append(int(tok))
            elif tok.lower() in {"true", "false"}:
                out.append(tok.lower() == "true")
            elif tok:
                out.append(tok)
        return out
    if re.fullmatch(r"-?\d+", v):
        return int(v)
    if v.lower() in {"true", "false"}:
        return v.lower() == "true"
    return v


def _parse_suite_yaml_without_pyyaml(text: str, path: Path) -> dict[str, Any]:
    cases: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None
    in_cases = False
    for raw in text.splitlines():
        line = raw.rstrip()
        if not in_cases:
            if re.match(r"^\s*cases:\s*$", line):
                in_cases = True
            continue

        m_case = re.match(r"^\s*-\s*id:\s*([A-Za-z0-9._:-]+)\s*$", line)
        if m_case:
            if current is not None:
                cases.append(current)
            current = {"id": m_case.group(1)}
            continue

        if current is None:
            continue

        m_kv = re.match(r"^\s{4}([A-Za-z0-9_]+):\s*(.+?)\s*$", line)
        if not m_kv:
            continue
        key = m_kv.group(1)
        value = _parse_scalar(m_kv.group(2))
        current[key] = value

    if current is not None:
        cases.append(current)
    if not cases:
        raise SystemExit(f"error: failed to parse cases from {path} without PyYAML")
    return {"cases": cases}


def _find_exe(cands: list[Path], env_name: str) -> Path:
    env = os.environ.get(env_name)
    if env:
        p = Path(env)
        if p.exists() and os.access(p, os.X_OK):
            return p
        raise SystemExit(f"error: {env_name}={env} is not executable")
    for cand in cands:
        if cand.exists() and os.access(cand, os.X_OK):
            return cand
    raise SystemExit(f"error: no executable found for {env_name}; tried: {', '.join(str(c) for c in cands)}")


def _run(
    cmd: list[str],
    *,
    env: dict[str, str] | None = None,
    cwd: Path | None = None,
    timeout_sec: float | None = None,
) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            cmd,
            env=env,
            cwd=str(cwd) if cwd else None,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
            timeout=timeout_sec,
        )
    except subprocess.TimeoutExpired as exc:
        out = ""
        if isinstance(exc.stdout, str):
            out += exc.stdout
        if isinstance(exc.stderr, str):
            out += exc.stderr
        out += f"\nerror: command timed out after {timeout_sec}s\n"
        return subprocess.CompletedProcess(cmd, 124, out)


def _safe_name(value: str) -> str:
    return "".join(ch if ch.isalnum() or ch in {"-", "_"} else "_" for ch in value)


def _required_for_profile(case: dict[str, Any], profile: str) -> bool:
    required_profiles = case.get("required_in_profile")
    if isinstance(required_profiles, list):
        return profile in {str(item).strip() for item in required_profiles if str(item).strip()}
    return bool(case.get("required", True))


def _source_kind(path: Path, explicit_kind: str) -> str:
    kind = explicit_kind.strip().lower()
    if kind:
        return kind
    suf = path.suffix.lower()
    if suf in {".s", ".S", ".asm"}:
        return "asm"
    if suf in {".ll", ".bc"}:
        return "ir"
    return "asm"


def _compile_source(
    *,
    source_kind: str,
    src_path: Path,
    obj: Path,
    llvm_mc: Path,
    llc: Path,
    timeout_sec: float,
) -> subprocess.CompletedProcess[str]:
    if source_kind == "asm":
        return _run(
            [str(llvm_mc), "-triple=linx64", "-filetype=obj", str(src_path), "-o", str(obj)],
            timeout_sec=timeout_sec,
        )
    if source_kind == "ir":
        cmd = [
            str(llc),
            "-mtriple=linx64",
            "-filetype=obj",
            str(src_path),
            "-o",
            str(obj),
        ]
        return _run(cmd, timeout_sec=timeout_sec)
    raise SystemExit(
        f"error: unsupported source_kind {source_kind!r} for case source {src_path} (expected asm|ir)"
    )


def _trace_has_any_block_kind(trace_path: Path, required_kinds: set[str]) -> bool:
    for raw in trace_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        if not line:
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        if not isinstance(row, dict):
            continue
        kind = str(row.get("block_kind", "")).strip().lower()
        if kind in required_kinds:
            return True
    return False


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Run Linx model diff suite")
    ap.add_argument("--root", default=".", help="linx-isa repo root")
    ap.add_argument("--suite", default="avs/model/linx_model_diff_suite.yaml")
    ap.add_argument("--profile", default="release-strict", choices=["dev", "release-strict"])
    ap.add_argument("--trace-schema-version", default="1.0")
    ap.add_argument("--compile-timeout", type=float, default=60.0)
    ap.add_argument("--qemu-timeout", type=float, default=20.0)
    ap.add_argument("--model-timeout", type=float, default=60.0)
    ap.add_argument("--diff-timeout", type=float, default=20.0)
    ap.add_argument(
        "--require-category",
        action="append",
        default=[],
        help="Require at least one passing required case in this category (repeatable)",
    )
    ap.add_argument("--workdir", default="", help="Optional persistent output directory")
    ap.add_argument("--report-out", default="", help="Optional summary JSON output path")
    args = ap.parse_args(argv)

    root = Path(args.root).resolve()
    suite_path = (root / args.suite).resolve()
    suite = _load_yaml_or_json(suite_path)
    cases = suite.get("cases")
    if not isinstance(cases, list) or not cases:
        raise SystemExit(f"error: {suite_path} missing non-empty list field 'cases'")

    llvm_mc = _find_exe(
        [
            root / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "llvm-mc",
            Path.home() / "llvm-project" / "build-linxisa-clang" / "bin" / "llvm-mc",
        ],
        "LLVM_MC",
    )
    llc = _find_exe(
        [
            root / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "llc",
            Path.home() / "llvm-project" / "build-linxisa-clang" / "bin" / "llc",
        ],
        "LLC",
    )
    qemu_bin = _find_exe(
        [
            root / "emulator" / "qemu" / "build" / "qemu-system-linx64",
            Path.home() / "qemu" / "build" / "qemu-system-linx64",
        ],
        "QEMU_BIN",
    )
    pyc_compile = _find_exe(
        [
            root / "tools" / "pyCircuit" / "build" / "bin" / "pyc-compile",
            root / "tools" / "pyCircuit" / "build-top" / "bin" / "pyc-compile",
            root / "tools" / "pyCircuit" / "pyc" / "mlir" / "build" / "bin" / "pyc-compile",
            Path.home() / "pyCircuit" / "build" / "bin" / "pyc-compile",
            Path.home() / "pyCircuit" / "build-top" / "bin" / "pyc-compile",
            Path.home() / "pyCircuit" / "pyc" / "mlir" / "build" / "bin" / "pyc-compile",
        ],
        "PYC_COMPILE",
    )

    pyc_runner = root / "tools" / "pyCircuit" / "tools" / "run_linx_cpu_pyc_cpp.sh"
    if not (pyc_runner.exists() and os.access(pyc_runner, os.X_OK)):
        raise SystemExit(f"error: missing executable pyCircuit runner: {pyc_runner}")
    diff_tool = root / "tools" / "pyCircuit" / "tools" / "linx_trace_diff.py"
    schema_tool = root / "tools" / "bringup" / "validate_trace_schema.py"
    if not diff_tool.exists():
        raise SystemExit(f"error: missing diff tool: {diff_tool}")
    if not schema_tool.exists():
        raise SystemExit(f"error: missing trace schema validator: {schema_tool}")

    if args.workdir:
        base_work = Path(args.workdir).resolve()
        base_work.mkdir(parents=True, exist_ok=True)
        cleanup_work = False
    elif args.report_out:
        report_path = Path(args.report_out).resolve()
        base_work = report_path.parent / "model_diff_work"
        base_work.mkdir(parents=True, exist_ok=True)
        cleanup_work = False
    else:
        base_work = Path(tempfile.mkdtemp(prefix="linx-model-diff."))
        cleanup_work = True

    summary: dict[str, Any] = {
        "suite": str(suite_path),
        "profile": args.profile,
        "trace_schema_version": args.trace_schema_version,
        "cases": [],
    }
    overall_fail = False
    required_categories = {str(item).strip() for item in args.require_category if str(item).strip()}
    if not required_categories and args.profile == "release-strict":
        required_categories = set(RELEASE_STRICT_REQUIRED_CATEGORIES)
    category_required_seen: set[str] = set()
    category_required_passed: set[str] = set()

    try:
        for idx, case in enumerate(cases):
            if not isinstance(case, dict):
                raise SystemExit(f"error: suite case[{idx}] must be an object")
            cid = str(case.get("id", "")).strip()
            if not cid:
                raise SystemExit(f"error: suite case[{idx}] missing non-empty id")
            src = case.get("source")
            if not isinstance(src, str) or not src.strip():
                raise SystemExit(f"error: case {cid} missing source path")
            src_path = (root / src).resolve() if not Path(src).is_absolute() else Path(src).resolve()
            if not src_path.exists():
                raise SystemExit(f"error: case {cid} source missing: {src_path}")
            seed = str(case.get("seed", "0"))
            required = _required_for_profile(case, args.profile)
            category = str(case.get("category", "uncategorized")).strip() or "uncategorized"
            if required and category in required_categories:
                category_required_seen.add(category)
            source_kind = _source_kind(src_path, str(case.get("source_kind", "")))
            ignore = case.get("ignore_fields", ["cycle"])
            if not isinstance(ignore, list):
                ignore = ["cycle"]
            ignore_fields = [str(item).strip() for item in ignore if str(item).strip()]
            required_kinds_raw = case.get("require_block_kind_any_of", [])
            required_kinds = {
                str(item).strip().lower()
                for item in required_kinds_raw
                if str(item).strip()
            }

            case_dir = base_work / f"{idx:02d}_{_safe_name(cid)}_seed{_safe_name(seed)}"
            case_dir.mkdir(parents=True, exist_ok=True)
            obj = case_dir / "test.o"
            qemu_trace = case_dir / "qemu.jsonl"
            pyc_trace = case_dir / "pyc.jsonl"
            log = case_dir / "run.log"

            with log.open("w", encoding="utf-8") as lf:
                def _log(msg: str) -> None:
                    lf.write(msg.rstrip() + "\n")
                    lf.flush()
                    print(msg)

                _log(
                    f"[case {cid}] profile={args.profile} required={required} "
                    f"category={category} source_kind={source_kind} seed={seed} source={src_path}"
                )
                r = _compile_source(
                    source_kind=source_kind,
                    src_path=src_path,
                    obj=obj,
                    llvm_mc=llvm_mc,
                    llc=llc,
                    timeout_sec=float(case.get("compile_timeout", args.compile_timeout)),
                )
                _log(r.stdout)
                if r.returncode != 0:
                    _log(f"[case {cid}] FAIL: compile rc={r.returncode}")
                    result = {
                        "id": cid,
                        "category": category,
                        "source_kind": source_kind,
                        "required": required,
                        "status": "fail",
                        "stage": "compile",
                        "seed": seed,
                        "log": str(log),
                    }
                    summary["cases"].append(result)
                    if required:
                        overall_fail = True
                    continue

                env_qemu = dict(os.environ)
                env_qemu["LINX_COMMIT_TRACE"] = str(qemu_trace)
                r = _run(
                    [str(qemu_bin), "-nographic", "-monitor", "none", "-machine", "virt", "-kernel", str(obj)],
                    env=env_qemu,
                    timeout_sec=float(case.get("qemu_timeout", args.qemu_timeout)),
                )
                _log(r.stdout)
                if r.returncode != 0:
                    _log(f"[case {cid}] FAIL: qemu rc={r.returncode}")
                    result = {
                        "id": cid,
                        "category": category,
                        "source_kind": source_kind,
                        "required": required,
                        "status": "fail",
                        "stage": "qemu",
                        "seed": seed,
                        "log": str(log),
                    }
                    summary["cases"].append(result)
                    if required:
                        overall_fail = True
                    continue

                if required_kinds and not _trace_has_any_block_kind(qemu_trace, required_kinds):
                    _log(
                        f"[case {cid}] FAIL: qemu trace missing required block kinds "
                        f"{sorted(required_kinds)}"
                    )
                    result = {
                        "id": cid,
                        "category": category,
                        "source_kind": source_kind,
                        "required": required,
                        "status": "fail",
                        "stage": "shape_block_kind",
                        "seed": seed,
                        "log": str(log),
                    }
                    summary["cases"].append(result)
                    if required:
                        overall_fail = True
                    continue

                env_pyc = dict(os.environ)
                env_pyc["PYC_KONATA"] = "0"
                env_pyc["PYC_EXPECT_EXIT"] = "0"
                boot_pc = str(case.get("boot_pc", "")).strip()
                if boot_pc:
                    env_pyc["PYC_BOOT_PC"] = boot_pc
                env_pyc["PYC_COMMIT_TRACE"] = str(pyc_trace)
                env_pyc["LINX_DIFF_FIXTURE_ID"] = cid
                env_pyc["LINX_DIFF_SEED"] = seed
                env_pyc["PYC_COMPILE"] = str(pyc_compile)
                r = _run(
                    [str(pyc_runner), "--elf", str(obj)],
                    env=env_pyc,
                    cwd=root / "tools" / "pyCircuit",
                    timeout_sec=float(case.get("model_timeout", args.model_timeout)),
                )
                _log(r.stdout)
                if r.returncode != 0:
                    _log(f"[case {cid}] FAIL: pyc runner rc={r.returncode}")
                    result = {
                        "id": cid,
                        "category": category,
                        "source_kind": source_kind,
                        "required": required,
                        "status": "fail",
                        "stage": "model",
                        "seed": seed,
                        "log": str(log),
                    }
                    summary["cases"].append(result)
                    if required:
                        overall_fail = True
                    continue

                model_traces: list[tuple[str, Path]] = [("model", pyc_trace)]
                extra_model_traces = case.get("extra_model_traces", [])
                if isinstance(extra_model_traces, list):
                    for item in extra_model_traces:
                        if not isinstance(item, dict):
                            continue
                        name = str(item.get("name", "")).strip()
                        rel_path = str(item.get("path", "")).strip()
                        if not name or not rel_path:
                            continue
                        p = Path(rel_path)
                        trace_path = p if p.is_absolute() else (case_dir / rel_path).resolve()
                        model_traces.append((name, trace_path))

                for trace_name, trace_path in [("qemu", qemu_trace), *model_traces]:
                    r = _run(
                        [
                            sys.executable,
                            str(schema_tool),
                            "--trace",
                            str(trace_path),
                            "--expected-version",
                            args.trace_schema_version,
                            "--assume-trace-version",
                            str(case.get("trace_version", "1.0")),
                            "--check-ordering",
                        ],
                        timeout_sec=float(case.get("schema_timeout", args.diff_timeout)),
                    )
                    _log(r.stdout)
                    if r.returncode != 0:
                        _log(f"[case {cid}] FAIL: {trace_name} trace schema rc={r.returncode}")
                        result = {
                            "id": cid,
                            "category": category,
                            "source_kind": source_kind,
                            "required": required,
                            "status": "fail",
                            "stage": f"{trace_name}_trace_schema",
                            "seed": seed,
                            "log": str(log),
                        }
                        summary["cases"].append(result)
                        if required:
                            overall_fail = True
                        break
                else:
                    skip_trace_diff = bool(case.get("skip_trace_diff", False))
                    if skip_trace_diff:
                        _log(f"[case {cid}] SKIP: trace diff disabled by case policy (schema/shape-only)")
                        result = {
                            "id": cid,
                            "category": category,
                            "source_kind": source_kind,
                            "required": required,
                            "status": "pass",
                            "stage": "schema_only",
                            "seed": seed,
                            "log": str(log),
                        }
                        summary["cases"].append(result)
                        if required and category in required_categories:
                            category_required_passed.add(category)
                        continue

                    diff_fail = False
                    diff_stage = "complete"
                    drop_boundary_selfloops = bool(case.get("drop_boundary_selfloops", False))
                    for trace_name, trace_path in model_traces:
                        cmd = [sys.executable, str(diff_tool), str(qemu_trace), str(trace_path)]
                        for field in ignore_fields:
                            cmd.extend(["--ignore", field])
                        if drop_boundary_selfloops:
                            cmd.append("--drop-boundary-selfloops")
                        r = _run(cmd, timeout_sec=float(case.get("diff_timeout", args.diff_timeout)))
                        _log(r.stdout)
                        if r.returncode != 0:
                            diff_fail = True
                            diff_stage = f"diff_{trace_name}"
                            _log(f"[case {cid}] FAIL: trace diff ({trace_name}) rc={r.returncode}")
                            break
                    if diff_fail:
                        result = {
                            "id": cid,
                            "category": category,
                            "source_kind": source_kind,
                            "required": required,
                            "status": "fail",
                            "stage": diff_stage,
                            "seed": seed,
                            "log": str(log),
                        }
                        summary["cases"].append(result)
                        if required:
                            overall_fail = True
                    else:
                        result = {
                            "id": cid,
                            "category": category,
                            "source_kind": source_kind,
                            "required": required,
                            "status": "pass",
                            "stage": "complete",
                            "seed": seed,
                            "log": str(log),
                        }
                        summary["cases"].append(result)
                        if required and category in required_categories:
                            category_required_passed.add(category)

        missing_required_categories = sorted(required_categories - category_required_seen)
        failing_required_categories = sorted(category_required_seen - category_required_passed)
        summary["required_categories"] = sorted(required_categories)
        summary["required_categories_seen"] = sorted(category_required_seen)
        summary["required_categories_passed"] = sorted(category_required_passed)
        summary["missing_required_categories"] = missing_required_categories
        summary["failing_required_categories"] = failing_required_categories
        if missing_required_categories or failing_required_categories:
            overall_fail = True

        summary["ok"] = not overall_fail
        summary["workdir"] = str(base_work)
        if args.report_out:
            out = Path(args.report_out).resolve()
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        else:
            print(json.dumps(summary, indent=2, sort_keys=True))
        if overall_fail:
            missing = summary.get("missing_required_categories", [])
            failing = summary.get("failing_required_categories", [])
            if missing or failing:
                print(
                    "error: model diff suite failed required category coverage "
                    f"(missing={missing}, failing={failing})",
                    file=sys.stderr,
                )
            else:
                print("error: model diff suite failed for one or more required cases", file=sys.stderr)
            return 1
        print("ok: model diff suite passed")
        return 0
    finally:
        if cleanup_work:
            shutil.rmtree(base_work, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
