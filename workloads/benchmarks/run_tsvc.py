#!/usr/bin/env python3

from __future__ import annotations

import argparse
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

_RE_OBJDUMP_INSN = re.compile(
    r"^\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]{2}(?:\s+[0-9a-fA-F]{2})*)\s+(.*)$"
)


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
        "-fno-vectorize",
        "-fno-slp-vectorize",
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
        sys.stderr.buffer.write(p.stderr)
        raise SystemExit(f"error: compile failed: {src}")


def _build_runtime_objects(
    *,
    clang: Path,
    target: str,
    out_dir: Path,
    vector_mode: str,
    verbose: bool,
) -> list[Path]:
    rt_dir = out_dir / "_runtime"
    rt_dir.mkdir(parents=True, exist_ok=True)

    include_dirs = [BENCH_DIR, COMPAT_DIR]
    tsvc_vmode = 0
    if vector_mode == "mpar":
        tsvc_vmode = 1
    cflags_extra = [f"-DLINX_TSVC_VECTOR_MODE={tsvc_vmode}"]

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
    objs.append(compile_runtime(COMPAT_DIR / "linx_tsvc_vec.c", "linx_tsvc_vec.o"))
    return objs


def _rewrite_macro(text: str, macro: str, value: int) -> str:
    pattern = rf"^\s*#define\s+{re.escape(macro)}\s+\d+\s*$"
    repl = f"#define {macro} {value}"
    out, n = re.subn(pattern, repl, text, flags=re.MULTILINE)
    if n == 0:
        raise SystemExit(f"error: failed to patch {macro} in TSVC common.h")
    return out


def _stage_tsvc_sources(
    *,
    src_dir: Path,
    stage_dir: Path,
    iterations: int,
    len_1d: int,
    len_2d: int,
    vector_mode: str,
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

    marker = (
        "\n#ifndef LINX_TSVC_VBLOCK_ENABLED\n"
        "#define LINX_TSVC_VBLOCK_ENABLED 1\n"
        "#endif\n"
        "#if LINX_TSVC_VBLOCK_ENABLED\n"
        "extern void linx_tsvc_vec_smoke(void);\n"
        "#define LINX_TSVC_LAUNCH() linx_tsvc_vec_smoke()\n"
        "#else\n"
        "#define LINX_TSVC_LAUNCH() ((void)0)\n"
        "#endif\n"
    )

    anchor = '#include "array_defs.h"\n'
    if marker not in tsvc_text:
        if anchor not in tsvc_text:
            raise SystemExit("error: failed to locate include anchor in tsvc.c")
        tsvc_text = tsvc_text.replace(anchor, anchor + marker, 1)

    if "LINX_TSVC_LAUNCH();" not in tsvc_text:
        tsvc_text, n = re.subn(
            r"^(\s*)((?:double|float|real_t)\s+result\s*=\s*vector_func\(&func_args\);)\s*$",
            r"\1LINX_TSVC_LAUNCH();\n\1\2",
            tsvc_text,
            count=1,
            flags=re.MULTILINE,
        )
        if n == 0:
            raise SystemExit("error: failed to locate time_function call site in tsvc.c")
    tsvc_c.write_text(tsvc_text, encoding="utf-8")

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

    if vector_mode == "off":
        return ordered
    return ordered


def _parse_objdump_count(objdump_text: str) -> int:
    count = 0
    for line in objdump_text.splitlines():
        if _RE_OBJDUMP_INSN.match(line):
            count += 1
    return count


@dataclass(frozen=True)
class TsvcRunResult:
    elf: Path
    objdump: Path
    stdout_log: Path
    stderr_log: Path
    static_insn_count: int
    expected_kernels: int
    observed_kernels: int
    vector_marker_count: int
    vector_insn_count: int


def _run_tsvc(
    *,
    clang: Path,
    lld: Path,
    llvm_objdump: Path,
    qemu: Path,
    target: str,
    out_dir: Path,
    artifacts_dir: Path,
    runtime_objs: list[Path],
    iterations: int,
    len_1d: int,
    len_2d: int,
    vector_mode: str,
    timeout_s: float,
    verbose: bool,
) -> TsvcRunResult:
    stage_dir = out_dir / "tsvc_stage"
    expected_kernels = _stage_tsvc_sources(
        src_dir=TSVC_SRC_DIR,
        stage_dir=stage_dir,
        iterations=iterations,
        len_1d=len_1d,
        len_2d=len_2d,
        vector_mode=vector_mode,
    )

    obj_dir = out_dir / "tsvc" / "obj"
    obj_dir.mkdir(parents=True, exist_ok=True)

    include_dirs = [stage_dir, BENCH_DIR, COMPAT_DIR]
    cflags = [
        f"-DLINX_TSVC_VBLOCK_ENABLED={0 if vector_mode == 'off' else 1}",
    ]

    objects: list[Path] = []
    for src in (stage_dir / "tsvc.c", stage_dir / "common.c", stage_dir / "dummy.c"):
        obj = obj_dir / f"{src.stem}.o"
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
    elf_dir = artifacts_dir / "elf"
    objdump_dir = artifacts_dir / "objdump"
    qemu_dir = artifacts_dir / "qemu"
    elf_dir.mkdir(parents=True, exist_ok=True)
    objdump_dir.mkdir(parents=True, exist_ok=True)
    qemu_dir.mkdir(parents=True, exist_ok=True)

    elf = elf_dir / "tsvc.elf"
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
        raise SystemExit("error: TSVC link failed")

    objdump_path = objdump_dir / "tsvc.objdump.txt"
    p = _run(
        [str(llvm_objdump), "-d", f"--triple={target}", str(elf)],
        verbose=verbose,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stderr)
        raise SystemExit("error: llvm-objdump failed for TSVC")
    objdump_path.write_bytes(p.stdout or b"")

    stdout_log = qemu_dir / "tsvc.stdout.txt"
    stderr_log = qemu_dir / "tsvc.stderr.txt"
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
        raise SystemExit(f"error: QEMU timeout after {timeout_s:.1f}s (TSVC)")

    stdout_log.write_bytes(p.stdout or b"")
    stderr_log.write_bytes(p.stderr or b"")
    if p.returncode != 0:
        raise SystemExit(
            f"error: TSVC failed on QEMU (exit={p.returncode})\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )

    out_text = (p.stdout or b"").decode("utf-8", errors="replace")
    if "Loop" not in out_text or "Checksum" not in out_text:
        raise SystemExit(
            "error: TSVC output missing header\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )

    observed = 0
    missing: list[str] = []
    for kernel in expected_kernels:
        if re.search(rf"(?m)(^|\n)\s*{re.escape(kernel)}\t", out_text):
            observed += 1
        else:
            missing.append(kernel)
    if missing:
        preview = ", ".join(missing[:8])
        raise SystemExit(
            f"error: TSVC did not execute all kernels ({len(missing)} missing)\n"
            f"  missing sample: {preview}\n"
            f"  stdout: {stdout_log}\n"
            f"  stderr: {stderr_log}"
        )

    objdump_text = objdump_path.read_text(encoding="utf-8", errors="replace")
    static_count = _parse_objdump_count(objdump_text)
    vector_marker_count = len(re.findall(r"(?i)\bbstart\.(?:mseq|mpar)\b", objdump_text))
    vector_insn_count = len(re.findall(r"(?i)\bv\.[a-z0-9]", objdump_text))
    if vector_mode != "off" and (vector_marker_count == 0 or vector_insn_count == 0):
        raise SystemExit(
            "error: TSVC build does not contain Linx vector block markers/instructions\n"
            f"  objdump: {objdump_path}"
        )

    return TsvcRunResult(
        elf=elf,
        objdump=objdump_path,
        stdout_log=stdout_log,
        stderr_log=stderr_log,
        static_insn_count=static_count,
        expected_kernels=len(expected_kernels),
        observed_kernels=observed,
        vector_marker_count=vector_marker_count,
        vector_insn_count=vector_insn_count,
    )


def _write_report(
    *,
    report_path: Path,
    result: TsvcRunResult,
    iterations: int,
    len_1d: int,
    len_2d: int,
    vector_mode: str,
) -> None:
    lines = [
        "# TSVC on LinxISA QEMU",
        "",
        f"- Profile: `iterations={iterations}`, `LEN_1D={len_1d}`, `LEN_2D={len_2d}`",
        f"- Vector mode: `{vector_mode}`",
        f"- Kernels executed: `{result.observed_kernels}/{result.expected_kernels}`",
        f"- Vector block markers in objdump: `{result.vector_marker_count}`",
        f"- Vector instructions in objdump: `{result.vector_insn_count}`",
        f"- Static instruction count: `{result.static_insn_count}`",
        "",
        "## Artifacts",
        f"- ELF: `{result.elf}`",
        f"- Objdump: `{result.objdump}`",
        f"- QEMU stdout: `{result.stdout_log}`",
        f"- QEMU stderr: `{result.stderr_log}`",
    ]
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


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
        choices=["off", "mseq", "mpar"],
        default="mseq",
    )
    ap.add_argument("--timeout", type=float, default=180.0, help="QEMU timeout (seconds)")
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

    runtime_objs = _build_runtime_objects(
        clang=clang,
        target=args.target,
        out_dir=out_dir,
        vector_mode=args.vector_mode,
        verbose=args.verbose,
    )
    result = _run_tsvc(
        clang=clang,
        lld=lld,
        llvm_objdump=llvm_objdump,
        qemu=qemu,
        target=args.target,
        out_dir=out_dir,
        artifacts_dir=generated_dir,
        runtime_objs=runtime_objs,
        iterations=args.iterations,
        len_1d=args.len_1d,
        len_2d=args.len_2d,
        vector_mode=args.vector_mode,
        timeout_s=args.timeout,
        verbose=args.verbose,
    )
    report_path = generated_dir / "tsvc_report.md"
    _write_report(
        report_path=report_path,
        result=result,
        iterations=args.iterations,
        len_1d=args.len_1d,
        len_2d=args.len_2d,
        vector_mode=args.vector_mode,
    )
    print(f"ok: wrote {report_path}")
    print(f"ok: kernels executed {result.observed_kernels}/{result.expected_kernels}")
    print(f"ok: vector markers {result.vector_marker_count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
