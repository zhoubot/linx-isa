#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


TSVC_DIR = Path(__file__).resolve().parent
WORKLOADS_DIR = TSVC_DIR.parent
REPO_ROOT = WORKLOADS_DIR.parent
GENERATED_DIR = WORKLOADS_DIR / "generated"

ANALYZE_SCRIPT = TSVC_DIR / "analyze_tsvc_vectorization.py"
COMPAT_INCLUDE = TSVC_DIR / "include"
FREESTANDING_INCLUDE = REPO_ROOT / "avs" / "runtime" / "freestanding" / "include"
FREESTANDING_SRC = REPO_ROOT / "avs" / "runtime" / "freestanding" / "src"
STARTUP_SRC = WORKLOADS_DIR / "common" / "startup.c"
COMPAT_RUNTIME_SRC = TSVC_DIR / "runtime" / "linx_compat.c"
PINNED_TSVC_SRC = TSVC_DIR / "upstream" / "TSVC_2" / "src"
FALLBACK_TSVC_SRC = WORKLOADS_DIR / "third_party" / "TSVC_2" / "src"

_RE_TSVC_ROW = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s+(\S+)\s+(\S+)\s*$")
_VECTOR_MODES = ("off", "mseq", "mpar", "auto")


@dataclass(frozen=True)
class ModeArtifacts:
    mode: str
    elf: Path
    objdump: Path
    qemu_stdout: Path | None
    qemu_stderr: Path | None
    observed_kernels: int | None
    remarks_jsonl: Path | None
    coverage_md: Path
    coverage_json: Path
    remarks_summary_json: Path
    gap_plan_json: Path
    vectorized_kernels: int
    total_kernels: int


def _run(cmd: list[str], *, cwd: Path | None = None, verbose: bool = False, **kwargs) -> subprocess.CompletedProcess[bytes]:
    if verbose:
        print("+", " ".join(shlex.quote(c) for c in cmd), file=sys.stderr)
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


def _resolve_tsvc_src(user_path: str | None) -> Path:
    if user_path:
        src = Path(os.path.expanduser(user_path)).resolve()
        if not src.exists():
            raise SystemExit(f"error: TSVC source path not found: {src}")
        return src
    if PINNED_TSVC_SRC.exists():
        return PINNED_TSVC_SRC
    if FALLBACK_TSVC_SRC.exists():
        return FALLBACK_TSVC_SRC
    raise SystemExit(
        "error: TSVC sources not found.\n"
        f"hint: run {TSVC_DIR / 'fetch_tsvc.sh'}"
    )


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

    # Runtime guard list for known auto-vectorization miscompile/hang kernels.
    # Keep these scalar until pass coverage closes the corresponding gaps.
    runtime_guard_kernels = (
        "s124",
        "s161",
        "s253",
        "s271",
        "s272",
        "s273",
        "s274",
        "s278",
        "s2711",
        "s2712",
        "s443",
        "s4115",
        "vif",
        "s318",
    )
    for kernel_name in runtime_guard_kernels:
        pattern = (
            rf"(?m)^real_t\s+{re.escape(kernel_name)}\s*\(\s*struct\s+args_t\s*\*\s*func_args\s*\)"
        )
        repl = (
            f"__attribute__((optnone,noinline)) real_t {kernel_name}(struct args_t * func_args)"
        )
        tsvc_text = re.sub(pattern, repl, tsvc_text, count=1)

    if "<stdint.h>" not in tsvc_text:
        tsvc_text = tsvc_text.replace(
            "#include <sys/time.h>\n",
            "#include <sys/time.h>\n#include <stdint.h>\n",
        )

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
        "    printf(\"%llu\\t0x%08x\\n\", (unsigned long long)taken_us, bits.u);\n"
        "}\n"
    )
    tsvc_text, n = time_func_pat.subn(lambda _m: time_func_repl, tsvc_text)
    if n != 1:
        raise SystemExit(f"error: expected to patch exactly 1 time_function, got {n}")

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


def _mode_compile_flags(mode: str, remarks_jsonl: Path | None) -> list[str]:
    flags: list[str] = ["-fno-vectorize", "-fno-slp-vectorize"]
    autovec_mode = _mode_to_autovec(mode)
    if autovec_mode is None:
        flags.extend(["-mllvm", "-linx-simt-autovec=0"])
    else:
        flags.extend(
            [
                "-mllvm",
                "-linx-simt-autovec=1",
                "-mllvm",
                f"-linx-simt-autovec-mode={autovec_mode}",
            ]
        )
        if remarks_jsonl is not None:
            flags.extend(["-mllvm", f"-linx-simt-autovec-remarks={remarks_jsonl}"])
    return flags


def _compile_c(
    *,
    clang: Path,
    target: str,
    src: Path,
    out_obj: Path,
    include_dirs: list[Path],
    extra_cflags: list[str],
    verbose: bool,
) -> None:
    out_obj.parent.mkdir(parents=True, exist_ok=True)
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
        *extra_cflags,
        "-c",
        str(src),
        "-o",
        str(out_obj),
    ]
    p = _run(cmd, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stdout or b"")
        sys.stderr.buffer.write(p.stderr or b"")
        raise SystemExit(f"error: compile failed: {src}")


def _build_runtime_objects(
    *,
    clang: Path,
    target: str,
    out_dir: Path,
    verbose: bool,
) -> list[Path]:
    include_dirs = [COMPAT_INCLUDE, FREESTANDING_INCLUDE, TSVC_DIR]
    rt_dir = out_dir / "_runtime"
    rt_dir.mkdir(parents=True, exist_ok=True)

    objs: list[Path] = []
    runtime_sources = [
        (STARTUP_SRC, "startup.o", []),
        (FREESTANDING_SRC / "syscall.c", "syscall.o", []),
        (FREESTANDING_SRC / "stdio" / "stdio.c", "stdio.o", []),
        (FREESTANDING_SRC / "stdlib" / "stdlib.c", "stdlib.o", []),
        (FREESTANDING_SRC / "string" / "mem.c", "mem.o", []),
        (FREESTANDING_SRC / "string" / "str.c", "str.o", []),
        (FREESTANDING_SRC / "math" / "math.c", "math.o", []),
        (FREESTANDING_SRC / "softfp" / "softfp.c", "softfp.o", ["-O0"]),
        (FREESTANDING_SRC / "atomic" / "atomic_builtins.c", "atomic_builtins.o", []),
        (COMPAT_RUNTIME_SRC, "linx_compat.o", []),
    ]
    for src, obj_name, extra in runtime_sources:
        if not src.exists():
            raise SystemExit(f"error: missing runtime source: {src}")
        obj = rt_dir / obj_name
        _compile_c(
            clang=clang,
            target=target,
            src=src,
            out_obj=obj,
            include_dirs=include_dirs,
            extra_cflags=extra,
            verbose=verbose,
        )
        objs.append(obj)
    return objs


def _link_elf(
    *,
    lld: Path,
    out_elf: Path,
    objs: list[Path],
    verbose: bool,
) -> None:
    out_elf.parent.mkdir(parents=True, exist_ok=True)
    cmd = [str(lld), "--entry=_start", "-o", str(out_elf), *[str(o) for o in objs]]
    p = _run(cmd, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stdout or b"")
        sys.stderr.buffer.write(p.stderr or b"")
        raise SystemExit(f"error: link failed: {out_elf}")


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


def _run_qemu(
    *,
    qemu: Path,
    elf: Path,
    stdout_log: Path,
    stderr_log: Path,
    timeout_s: float,
    verbose: bool,
) -> tuple[int, str]:
    stdout_log.parent.mkdir(parents=True, exist_ok=True)
    stderr_log.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
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
            cmd,
            verbose=verbose,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout_s,
        )
    except subprocess.TimeoutExpired as e:
        stdout_log.write_bytes(e.stdout or b"")
        stderr_log.write_bytes(e.stderr or b"")
        raise SystemExit(f"error: QEMU timeout after {timeout_s:.1f}s ({elf.name})")

    stdout_log.write_bytes(p.stdout or b"")
    stderr_log.write_bytes(p.stderr or b"")
    text = (p.stdout or b"").decode("utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit(
            f"error: QEMU failed (exit={p.returncode})\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )
    return p.returncode, text


def _run_analyzer(
    *,
    python: str,
    mode: str,
    objdump: Path,
    kernel_list: Path,
    kernel_out_dir: Path,
    report: Path,
    json_out: Path,
    remarks_jsonl: Path | None,
    remarks_summary_out: Path,
    gap_plan_out: Path,
    strict_fail_under: int | None,
    verbose: bool,
) -> int:
    cmd = [
        python,
        str(ANALYZE_SCRIPT),
        "--objdump",
        str(objdump),
        "--kernel-list",
        str(kernel_list),
        "--kernel-out-dir",
        str(kernel_out_dir),
        "--report",
        str(report),
        "--json-out",
        str(json_out),
        "--remarks-summary-out",
        str(remarks_summary_out),
        "--gap-plan-out",
        str(gap_plan_out),
        "--mode",
        mode,
    ]
    if remarks_jsonl is not None:
        cmd += ["--remarks-jsonl", str(remarks_jsonl)]
    if strict_fail_under is not None:
        cmd += ["--fail-under", str(strict_fail_under)]
    p = _run(cmd, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stdout or b"")
        sys.stderr.buffer.write(p.stderr or b"")
        raise SystemExit(f"error: TSVC strict coverage analysis failed ({mode})")
    payload = json.loads(json_out.read_text(encoding="utf-8"))
    return int(payload.get("vectorized", 0))


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Build TSVC for Linx, run on QEMU, and emit strict auto-vectorization reports.")
    ap.add_argument("--clang", default=None, help="Path to clang (env: CLANG)")
    ap.add_argument("--lld", default=None, help="Path to ld.lld")
    ap.add_argument("--llvm-objdump", default=None, help="Path to llvm-objdump")
    ap.add_argument("--qemu", default=None, help="Path to qemu-system-linx64 (env: QEMU)")
    ap.add_argument("--target", default="linx64-linx-none-elf", help="Target triple")
    ap.add_argument("--tsvc-src", default=None, help="TSVC source directory (expects common.h + tsvc.c)")
    ap.add_argument("--iterations", type=int, default=32)
    ap.add_argument("--len-1d", type=int, default=320)
    ap.add_argument("--len-2d", type=int, default=16)
    ap.add_argument("--qemu-timeout", type=float, default=240.0, help="QEMU timeout (seconds)")
    ap.add_argument("--no-run-qemu", action="store_true", help="Skip QEMU execution (compile+objdump+analysis only)")
    ap.add_argument(
        "--vector-mode",
        choices=[*_VECTOR_MODES, "all"],
        default="auto",
        help="Build mode (`all` runs off+mseq+mpar+auto).",
    )
    ap.add_argument("--kernel-regex", default=None, help="Only run kernels matching this regex.")
    ap.add_argument("--strict-fail-under", type=int, default=None, help="Fail if strict vectorized kernels are below this threshold.")
    ap.add_argument("--out-dir", default=str(GENERATED_DIR), help="Generated artifacts root")
    ap.add_argument("--verbose", "-v", action="store_true")
    args = ap.parse_args(argv)

    if args.len_1d % 40 != 0:
        raise SystemExit("error: --len-1d must be a multiple of 40")
    if args.iterations <= 0 or args.len_1d <= 0 or args.len_2d <= 0:
        raise SystemExit("error: iterations/len values must be > 0")
    if not ANALYZE_SCRIPT.exists():
        raise SystemExit(f"error: missing analyzer: {ANALYZE_SCRIPT}")
    if not FREESTANDING_INCLUDE.exists():
        raise SystemExit(f"error: missing freestanding include tree: {FREESTANDING_INCLUDE}")

    clang = Path(os.path.expanduser(args.clang)) if args.clang else (_default_clang() or None)
    if not clang:
        raise SystemExit("error: clang not found; set --clang or CLANG")
    lld = (
        Path(os.path.expanduser(args.lld))
        if args.lld
        else (_default_llvm_tool(clang, "ld.lld") or _default_llvm_tool(clang, "lld"))
    )
    llvm_objdump = (
        Path(os.path.expanduser(args.llvm_objdump))
        if args.llvm_objdump
        else (_default_llvm_tool(clang, "llvm-objdump") or None)
    )
    qemu = (
        Path(os.path.expanduser(args.qemu))
        if args.qemu
        else (_default_qemu() if not args.no_run_qemu else None)
    )

    _check_exe(clang, "clang")
    if not lld:
        raise SystemExit("error: ld.lld/lld not found; set --lld")
    _check_exe(lld, "ld.lld")
    if not llvm_objdump:
        raise SystemExit("error: llvm-objdump not found; set --llvm-objdump or use clang sibling tool")
    _check_exe(llvm_objdump, "llvm-objdump")
    if not args.no_run_qemu:
        if not qemu:
            raise SystemExit("error: qemu-system-linx64 not found; set --qemu or QEMU")
        _check_exe(qemu, "qemu-system-linx64")

    tsvc_src = _resolve_tsvc_src(args.tsvc_src)
    required = ["common.h", "tsvc.c", "common.c", "dummy.c", "array_defs.h"]
    for name in required:
        if not (tsvc_src / name).exists():
            raise SystemExit(f"error: malformed TSVC source tree, missing: {tsvc_src / name}")

    out_root = Path(args.out_dir).resolve()
    build_dir = out_root / "build" / "tsvc"
    stage_dir = build_dir / "stage"
    elf_dir = out_root / "elf" / "tsvc"
    objdump_dir = out_root / "objdump" / "tsvc"
    qemu_dir = out_root / "qemu" / "tsvc"
    reports_dir = out_root / "reports" / "tsvc"
    for d in (build_dir, elf_dir, objdump_dir, qemu_dir, reports_dir):
        d.mkdir(parents=True, exist_ok=True)

    kernels = _stage_tsvc_sources(
        src_dir=tsvc_src,
        stage_dir=stage_dir,
        iterations=args.iterations,
        len_1d=args.len_1d,
        len_2d=args.len_2d,
        kernel_regex=args.kernel_regex,
    )
    kernel_list_path = reports_dir / "kernel_list.txt"
    kernel_list_path.write_text("\n".join(kernels) + "\n", encoding="utf-8")

    runtime_objs = _build_runtime_objects(
        clang=clang,
        target=args.target,
        out_dir=build_dir,
        verbose=args.verbose,
    )

    if args.vector_mode == "all":
        modes = list(_VECTOR_MODES)
    else:
        modes = [args.vector_mode]

    python = sys.executable or "python3"
    results: dict[str, ModeArtifacts] = {}
    include_dirs = [COMPAT_INCLUDE, FREESTANDING_INCLUDE, stage_dir]
    for mode in modes:
        mode_obj_dir = build_dir / mode / "obj"
        mode_obj_dir.mkdir(parents=True, exist_ok=True)
        remarks_jsonl = None
        if mode != "off":
            remarks_jsonl = reports_dir / f"vectorization_remarks_raw.{mode}.jsonl"
            if remarks_jsonl.exists():
                remarks_jsonl.unlink()

        tsvc_obj = mode_obj_dir / "tsvc.o"
        common_obj = mode_obj_dir / "common.o"
        dummy_obj = mode_obj_dir / "dummy.o"
        _compile_c(
            clang=clang,
            target=args.target,
            src=stage_dir / "tsvc.c",
            out_obj=tsvc_obj,
            include_dirs=include_dirs,
            extra_cflags=_mode_compile_flags(mode, remarks_jsonl),
            verbose=args.verbose,
        )
        _compile_c(
            clang=clang,
            target=args.target,
            src=stage_dir / "common.c",
            out_obj=common_obj,
            include_dirs=include_dirs,
            extra_cflags=_mode_compile_flags("off", None),
            verbose=args.verbose,
        )
        _compile_c(
            clang=clang,
            target=args.target,
            src=stage_dir / "dummy.c",
            out_obj=dummy_obj,
            include_dirs=include_dirs,
            extra_cflags=_mode_compile_flags("off", None),
            verbose=args.verbose,
        )

        elf_path = elf_dir / f"tsvc.{mode}.elf"
        _link_elf(
            lld=lld,
            out_elf=elf_path,
            objs=[*runtime_objs, tsvc_obj, common_obj, dummy_obj],
            verbose=args.verbose,
        )

        objdump_path = objdump_dir / f"tsvc.{mode}.objdump.txt"
        p = _run(
            [str(llvm_objdump), "-d", f"--triple={args.target}", str(elf_path)],
            verbose=args.verbose,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if p.returncode != 0:
            sys.stderr.buffer.write(p.stdout or b"")
            sys.stderr.buffer.write(p.stderr or b"")
            raise SystemExit(f"error: llvm-objdump failed ({mode})")
        objdump_path.write_bytes(p.stdout or b"")

        qemu_stdout = None
        qemu_stderr = None
        observed_kernels = None
        if not args.no_run_qemu:
            qemu_stdout = qemu_dir / f"tsvc.{mode}.stdout.txt"
            qemu_stderr = qemu_dir / f"tsvc.{mode}.stderr.txt"
            _exit_code, out_text = _run_qemu(
                qemu=qemu,
                elf=elf_path,
                stdout_log=qemu_stdout,
                stderr_log=qemu_stderr,
                timeout_s=args.qemu_timeout,
                verbose=args.verbose,
            )
            if "Loop" not in out_text or "Checksum" not in out_text:
                raise SystemExit(
                    f"error: TSVC output missing header ({mode})\n"
                    f"  stdout: {qemu_stdout}\n"
                    f"  stderr: {qemu_stderr}"
                )
            checksum_by_kernel = _parse_kernel_checksums(out_text, kernels)
            observed_kernels = len(checksum_by_kernel)
            missing = [k for k in kernels if k not in checksum_by_kernel]
            if missing:
                preview = ", ".join(missing[:8])
                raise SystemExit(
                    f"error: TSVC missing kernels on QEMU ({mode}): {len(missing)}\n"
                    f"  missing sample: {preview}\n"
                    f"  stdout: {qemu_stdout}\n"
                    f"  stderr: {qemu_stderr}"
                )

        coverage_md = reports_dir / f"vectorization_coverage.{mode}.md"
        coverage_json = reports_dir / f"vectorization_coverage.{mode}.json"
        remarks_summary_json = reports_dir / f"vectorization_remarks.{mode}.json"
        gap_plan_json = reports_dir / f"vectorization_gap_plan.{mode}.json"
        kernel_out_dir = objdump_dir / "kernels" / mode
        vectorized = _run_analyzer(
            python=python,
            mode=mode,
            objdump=objdump_path,
            kernel_list=kernel_list_path,
            kernel_out_dir=kernel_out_dir,
            report=coverage_md,
            json_out=coverage_json,
            remarks_jsonl=remarks_jsonl,
            remarks_summary_out=remarks_summary_json,
            gap_plan_out=gap_plan_json,
            strict_fail_under=args.strict_fail_under if mode != "off" else None,
            verbose=args.verbose,
        )

        results[mode] = ModeArtifacts(
            mode=mode,
            elf=elf_path,
            objdump=objdump_path,
            qemu_stdout=qemu_stdout,
            qemu_stderr=qemu_stderr,
            observed_kernels=observed_kernels,
            remarks_jsonl=remarks_jsonl,
            coverage_md=coverage_md,
            coverage_json=coverage_json,
            remarks_summary_json=remarks_summary_json,
            gap_plan_json=gap_plan_json,
            vectorized_kernels=vectorized,
            total_kernels=len(kernels),
        )

    selected_mode = "auto" if "auto" in results else modes[-1]
    selected = results[selected_mode]

    (reports_dir / "vectorization_coverage.md").write_text(
        selected.coverage_md.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    (reports_dir / "vectorization_coverage.json").write_text(
        selected.coverage_json.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    (reports_dir / "vectorization_remarks.json").write_text(
        selected.remarks_summary_json.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    (reports_dir / "vectorization_gap_plan.json").write_text(
        selected.gap_plan_json.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    if selected.remarks_jsonl and selected.remarks_jsonl.exists():
        (reports_dir / "vectorization_remarks_raw.jsonl").write_text(
            selected.remarks_jsonl.read_text(encoding="utf-8", errors="replace"),
            encoding="utf-8",
        )

    summary = [
        "# TSVC auto-vectorization report",
        "",
        f"- Source: `{tsvc_src}`",
        f"- Profile: `iterations={args.iterations}`, `LEN_1D={args.len_1d}`, `LEN_2D={args.len_2d}`",
        f"- Modes run: `{', '.join(modes)}`",
        f"- QEMU executed: `{'no' if args.no_run_qemu else 'yes'}`",
        "",
        "## Mode artifacts",
    ]
    for mode in modes:
        item = results[mode]
        qemu_status = "skipped"
        if not args.no_run_qemu:
            qemu_status = f"{item.observed_kernels}/{item.total_kernels} kernels"
        summary.append(
            f"- `{mode}`: strict vectorized `{item.vectorized_kernels}/{item.total_kernels}`, "
            f"QEMU `{qemu_status}`, objdump `{item.objdump}`"
        )
    summary.extend(
        [
            "",
            "## Selected mode outputs",
            f"- Coverage: `{reports_dir / 'vectorization_coverage.md'}`",
            f"- Coverage JSON: `{reports_dir / 'vectorization_coverage.json'}`",
            f"- Remarks JSON: `{reports_dir / 'vectorization_remarks.json'}`",
            f"- Gap plan JSON: `{reports_dir / 'vectorization_gap_plan.json'}`",
            f"- Kernel objdumps: `{objdump_dir / 'kernels' / selected_mode}`",
        ]
    )
    if not args.no_run_qemu and selected.qemu_stdout and selected.qemu_stderr:
        summary.extend(
            [
                f"- QEMU stdout: `{selected.qemu_stdout}`",
                f"- QEMU stderr: `{selected.qemu_stderr}`",
            ]
        )
    (out_root / "tsvc_report.md").write_text("\n".join(summary) + "\n", encoding="utf-8")

    print(
        f"ok: TSVC {selected_mode} artifacts generated under {out_root}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
