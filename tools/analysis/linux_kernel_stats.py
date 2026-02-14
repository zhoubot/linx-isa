#!/usr/bin/env python3

from __future__ import annotations

import argparse
import gzip
import json
import os
import shlex
import signal
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
WORKLOADS_DIR = REPO_ROOT / "workloads"
GENERATED_DIR = WORKLOADS_DIR / "generated"


def _run(cmd: list[str], *, cwd: Path | None = None, verbose: bool = False, **kwargs) -> subprocess.CompletedProcess[bytes]:
    if verbose:
        print("+", " ".join(shlex.quote(c) for c in cmd), file=sys.stderr)
    return subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=False, **kwargs)


def _check_exe(p: Path, what: str) -> None:
    if not p.exists():
        raise SystemExit(f"error: {what} not found: {p}")
    if not os.access(p, os.X_OK):
        raise SystemExit(f"error: {what} not executable: {p}")


def _default_clang() -> Path | None:
    env = os.environ.get("CLANG")
    if env:
        return Path(os.path.expanduser(env))
    cand = Path.home() / "llvm-project" / "build-linxisa-clang" / "bin" / "clang"
    return cand if cand.exists() else None


def _default_llvm_objdump() -> Path | None:
    env = os.environ.get("OBJDUMP")
    if env:
        return Path(os.path.expanduser(env))
    clang = _default_clang()
    if clang:
        cand = clang.parent / "llvm-objdump"
        if cand.exists():
            return cand
    # Fallback: common location next to clang build.
    cand = Path.home() / "llvm-project" / "build-linxisa-clang" / "bin" / "llvm-objdump"
    return cand if cand.exists() else None


def _default_qemu() -> Path | None:
    env = os.environ.get("QEMU")
    if env:
        return Path(os.path.expanduser(env))
    cand = Path.home() / "qemu" / "build" / "qemu-system-linx64"
    if cand.exists():
        return cand
    cand_tci = Path.home() / "qemu" / "build-tci" / "qemu-system-linx64"
    return cand_tci if cand_tci.exists() else None


def _default_plugin() -> Path:
    return GENERATED_DIR / "plugins" / "liblinx_insn_hist.so"


def _ensure_plugin(plugin: Path, *, verbose: bool) -> Path:
    if plugin.exists():
        return plugin
    # Build into workloads/generated/plugins via repo script.
    cmd = ["bash", str(REPO_ROOT / "tools" / "qemu_plugins" / "build_linx_insn_hist.sh")]
    p = _run(cmd, cwd=REPO_ROOT, verbose=verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stderr)
        raise SystemExit("error: failed to build QEMU insn-hist plugin")
    if not plugin.exists():
        raise SystemExit(f"error: plugin build succeeded but output missing: {plugin}")
    return plugin


def _stream_objdump_to_file(
    *,
    llvm_objdump: Path,
    vmlinux: Path,
    triple: str,
    out_path: Path,
    compress: str,
    verbose: bool,
) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    cmd = [str(llvm_objdump), "-d", f"--triple={triple}", str(vmlinux)]
    if verbose:
        print("+", " ".join(shlex.quote(c) for c in cmd), file=sys.stderr)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert proc.stdout is not None
    assert proc.stderr is not None

    if compress == "gzip":
        with gzip.open(out_path, "wb") as f:
            while True:
                chunk = proc.stdout.read(1 << 20)
                if not chunk:
                    break
                f.write(chunk)
    else:
        with out_path.open("wb") as f:
            while True:
                chunk = proc.stdout.read(1 << 20)
                if not chunk:
                    break
                f.write(chunk)

    stderr = proc.stderr.read()
    rc = proc.wait()
    if rc != 0:
        sys.stderr.buffer.write(stderr)
        raise SystemExit(f"error: llvm-objdump failed (exit={rc})")


def _load_dyn_hist(path: Path) -> tuple[int | None, dict[str, int] | None]:
    if not path.exists():
        return None, None
    try:
        data = json.loads(path.read_text(encoding="utf-8", errors="replace"))
        total = data.get("total_insns", None)
        all_map = data.get("all", None)
        if not isinstance(total, int) or not isinstance(all_map, dict):
            return None, None
        out: dict[str, int] = {}
        for k, v in all_map.items():
            if isinstance(k, str) and isinstance(v, int):
                out[k] = v
        return total, out
    except Exception:
        return None, None


def _classify_mnemonic(mnemonic: str) -> str:
    m = mnemonic.strip().lower()
    if m.startswith("c."):
        m = m[2:]
    if m.startswith("hl."):
        m = m[3:]
    if m.startswith(("v", "vec", "simd")):
        return "vector"
    if m.startswith(("f",)):
        return "floating-point"
    if (
        "bstart" in m
        or m.startswith(("br", "j", "call", "ret", "fret", "fentry", "setret"))
        or "branch" in m
    ):
        return "control-flow"
    if m.startswith(("ld", "st", "lw", "sw", "lb", "sb", "lh", "sh", "lwu", "sdi", "ldi")):
        return "memory"
    if m.startswith(("and", "or", "xor", "not", "sll", "srl", "sra", "rol", "ror")):
        return "bitwise-shift"
    if m.startswith(("setc", "cmp")):
        return "compare-condition"
    if m.startswith(("add", "sub", "mul", "div", "rem", "mov", "neg", "sext")):
        return "integer-alu"
    if m.startswith(("csr", "sys", "ecall", "ebreak", "fence", "mret", "sret", "wfi")):
        return "system"
    return "other"


def _build_type_hist(mnemonic_hist: dict[str, int]) -> dict[str, int]:
    out: Counter[str] = Counter()
    for mnemonic, count in mnemonic_hist.items():
        out[_classify_mnemonic(mnemonic)] += count
    return dict(sorted(out.items(), key=lambda kv: (-kv[1], kv[0])))


def _format_top_table(m: dict[str, int], *, total: int, top_n: int = 50) -> str:
    c = Counter(m)
    items = c.most_common(top_n)
    lines = ["| Mnemonic | Count | % |", "|---|---:|---:|"]
    for k, v in items:
        pct = (100.0 * v / total) if total else 0.0
        lines.append(f"| `{k}` | {v} | {pct:5.2f} |")
    return "\n".join(lines)


def _format_type_table(m: dict[str, int], *, total: int) -> str:
    items = sorted(m.items(), key=lambda kv: (-kv[1], kv[0]))
    lines = ["| Type | Count | % |", "|---|---:|---:|"]
    for k, v in items:
        pct = (100.0 * v / total) if total else 0.0
        lines.append(f"| `{k}` | {v} | {pct:5.2f} |")
    return "\n".join(lines)


def _extract_linux_version_from_log(text: str) -> str | None:
    for line in text.splitlines():
        if "Linux version " in line:
            # Keep the full line for traceability.
            return line.strip()
    return None


def _qemu_boot_sample(
    *,
    qemu: Path,
    vmlinux: Path,
    initrd: Path | None,
    cmdline: str,
    plugin: Path,
    out_stdout: Path,
    out_stderr: Path,
    out_hist: Path,
    timeout_s: float,
    verbose: bool,
) -> None:
    out_stdout.parent.mkdir(parents=True, exist_ok=True)
    out_stderr.parent.mkdir(parents=True, exist_ok=True)
    out_hist.parent.mkdir(parents=True, exist_ok=True)

    qemu_cmd = [
        str(qemu),
        "-machine",
        "virt",
        "-m",
        "512M",
        "-nographic",
        "-monitor",
        "none",
        "-kernel",
        str(vmlinux),
    ]
    if initrd is not None and initrd.exists():
        qemu_cmd += ["-initrd", str(initrd)]
    qemu_cmd += ["-append", cmdline]
    qemu_cmd += ["-plugin", f"{plugin},out={out_hist},top=200"]

    if verbose:
        print("+", " ".join(shlex.quote(c) for c in qemu_cmd), file=sys.stderr)

    proc = subprocess.Popen(qemu_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert proc.stdout is not None
    assert proc.stderr is not None

    try:
        stdout, stderr = proc.communicate(timeout=timeout_s)
    except subprocess.TimeoutExpired:
        # Try to shut down gracefully so the plugin can flush.
        for sig, grace in ((signal.SIGINT, 2.0), (signal.SIGTERM, 2.0)):
            try:
                proc.send_signal(sig)
            except Exception:
                pass
            try:
                stdout, stderr = proc.communicate(timeout=grace)
                break
            except subprocess.TimeoutExpired:
                continue
        else:
            try:
                proc.kill()
            except Exception:
                pass
            stdout, stderr = proc.communicate(timeout=5.0)

    out_stdout.write_bytes(stdout or b"")
    out_stderr.write_bytes(stderr or b"")

    # QEMU exit status is not used as a strict gate for "boot sample" runs.
    # Some runs are intentionally host-terminated.


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Collect static+dynamic instruction stats for Linx Linux vmlinux under ~/linux.")
    ap.add_argument("--linux-root", default=str(Path.home() / "linux"))
    ap.add_argument("--build-dir", default="build-linx-fixed")
    ap.add_argument("--vmlinux", default="vmlinux")
    ap.add_argument("--initrd", default="linx-initramfs/initramfs.cpio")
    ap.add_argument("--qemu", default=None)
    ap.add_argument("--plugin", default=str(_default_plugin()))
    ap.add_argument("--kernel-cmdline", default="lpj=1000000 loglevel=4 slab_nomerge")
    ap.add_argument("--timeout-s", type=float, default=30.0)
    ap.add_argument("--static-only", action="store_true")
    ap.add_argument("--dynamic-only", action="store_true")
    ap.add_argument("--objdump-tool", default=None)
    ap.add_argument("--triple", default="linx64-linx-none-elf")
    ap.add_argument("--compress-objdump", choices=["none", "gzip"], default="gzip")
    ap.add_argument("--verbose", "-v", action="store_true")
    args = ap.parse_args(argv)

    linux_root = Path(os.path.expanduser(args.linux_root))
    build_dir = linux_root / args.build_dir
    vmlinux = build_dir / args.vmlinux
    initrd = build_dir / args.initrd

    if not vmlinux.exists():
        raise SystemExit(f"error: vmlinux not found: {vmlinux}")

    qemu = Path(os.path.expanduser(args.qemu)) if args.qemu else (_default_qemu() or None)
    if qemu is None:
        raise SystemExit("error: qemu-system-linx64 not found; set --qemu or QEMU")
    _check_exe(qemu, "qemu-system-linx64")

    plugin = Path(os.path.expanduser(args.plugin))
    plugin = _ensure_plugin(plugin, verbose=args.verbose)

    llvm_objdump = Path(os.path.expanduser(args.objdump_tool)) if args.objdump_tool else (_default_llvm_objdump() or None)
    if llvm_objdump is None:
        raise SystemExit("error: llvm-objdump not found; set --objdump-tool or OBJDUMP or CLANG")
    _check_exe(llvm_objdump, "llvm-objdump")

    # Output layout (repo-relative, under workloads/generated).
    out_objdump_dir = GENERATED_DIR / "objdump" / "linux" / args.build_dir
    out_linux_dir = GENERATED_DIR / "linux" / args.build_dir
    out_qemu_dir = GENERATED_DIR / "qemu" / "linux" / args.build_dir
    out_objdump_dir.mkdir(parents=True, exist_ok=True)
    out_linux_dir.mkdir(parents=True, exist_ok=True)
    out_qemu_dir.mkdir(parents=True, exist_ok=True)

    objdump_out = out_objdump_dir / ("vmlinux.objdump.txt.gz" if args.compress_objdump == "gzip" else "vmlinux.objdump.txt")
    static_md = out_linux_dir / "static_stats.md"
    static_json = out_linux_dir / "static_stats.json"
    dyn_stdout = out_qemu_dir / f"boot_{int(args.timeout_s)}s.stdout.txt"
    dyn_stderr = out_qemu_dir / f"boot_{int(args.timeout_s)}s.stderr.txt"
    dyn_hist = out_qemu_dir / f"boot_{int(args.timeout_s)}s.dyn_insn_hist.json"
    dyn_md = out_linux_dir / "dynamic_stats.md"
    report_md = out_linux_dir / "kernel_report.md"

    do_static = not args.dynamic_only
    do_dynamic = not args.static_only
    if args.static_only and args.dynamic_only:
        raise SystemExit("error: --static-only and --dynamic-only are mutually exclusive")

    # 1) Static: objdump + aggregate stats.
    if do_static:
        if not objdump_out.exists():
            _stream_objdump_to_file(
                llvm_objdump=llvm_objdump,
                vmlinux=vmlinux,
                triple=args.triple,
                out_path=objdump_out,
                compress=args.compress_objdump,
                verbose=args.verbose,
            )

        p = _run(
            [
                sys.executable,
                str(REPO_ROOT / "tools" / "analysis" / "objdump_stats.py"),
                "--roots",
                str(out_objdump_dir),
                "--glob",
                "**/*.objdump.txt*",
                "--spec",
                str(REPO_ROOT / "spec" / "isa" / "spec" / "current" / "linxisa-v0.3.json"),
                "--out-md",
                str(static_md),
                "--out-json",
                str(static_json),
                "--top",
                "50",
            ],
            cwd=REPO_ROOT,
            verbose=args.verbose,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if p.returncode != 0:
            sys.stderr.buffer.write(p.stdout)
            sys.stderr.buffer.write(p.stderr)
            raise SystemExit("error: objdump_stats.py failed for vmlinux")

    # 2) Dynamic: QEMU boot sample with plugin.
    dyn_total = None
    dyn_map = None
    linux_version_line = None
    if do_dynamic:
        _qemu_boot_sample(
            qemu=qemu,
            vmlinux=vmlinux,
            initrd=initrd if initrd.exists() else None,
            cmdline=args.kernel_cmdline,
            plugin=plugin,
            out_stdout=dyn_stdout,
            out_stderr=dyn_stderr,
            out_hist=dyn_hist,
            timeout_s=args.timeout_s,
            verbose=args.verbose,
        )

        # Summarize dynamic histogram.
        dyn_total, dyn_map = _load_dyn_hist(dyn_hist)
        boot_log = (dyn_stdout.read_text(encoding="utf-8", errors="replace") + "\n" + dyn_stderr.read_text(encoding="utf-8", errors="replace"))
        linux_version_line = _extract_linux_version_from_log(boot_log)

        lines: list[str] = []
        lines.append("# Linx Linux Dynamic Instruction Stats\n")
        lines.append(f"- Build: `{build_dir}`")
        lines.append(f"- QEMU: `{qemu}`")
        lines.append(f"- vmlinux: `{vmlinux}`")
        if initrd.exists():
            lines.append(f"- initrd: `{initrd}`")
        lines.append(f"- cmdline: `{args.kernel_cmdline}`")
        lines.append(f"- timeout: `{args.timeout_s}` seconds")
        lines.append(f"- plugin: `{plugin}`")
        lines.append(f"- histogram: `{dyn_hist}`")
        lines.append(f"- logs: `{dyn_stdout}` / `{dyn_stderr}`\n")
        if linux_version_line:
            lines.append(f"- Linux version: `{linux_version_line}`\n")

        if dyn_total is None or dyn_map is None:
            lines.append("## Status\n")
            lines.append("- ERROR: dynamic histogram not found or invalid\n")
        else:
            lines.append("## Summary\n")
            lines.append(f"- Dynamic instruction count (plugin total): `{dyn_total}`\n")
            lines.append("## Dynamic Opcode Distribution (Top 50)\n")
            lines.append(_format_top_table(dyn_map, total=dyn_total, top_n=50))
            lines.append("")
            lines.append("## Dynamic Instruction Type Histogram\n")
            lines.append(_format_type_table(_build_type_hist(dyn_map), total=dyn_total))
            lines.append("")

        dyn_md.write_text("\n".join(lines) + "\n", encoding="utf-8")

    # 3) Combined report.
    report_lines: list[str] = []
    report_lines.append("# Linx Linux Kernel Instruction Report\n")
    report_lines.append(f"- Linux root: `{linux_root}`")
    report_lines.append(f"- Build dir: `{build_dir}`")
    report_lines.append(f"- vmlinux: `{vmlinux}`")
    if initrd.exists():
        report_lines.append(f"- initrd: `{initrd}`")
    report_lines.append(f"- Objdump: `{objdump_out}`")
    report_lines.append(f"- Static stats: `{static_md}` / `{static_json}`")
    report_lines.append(f"- Dynamic stats: `{dyn_md}`")
    report_lines.append(f"- Dynamic histogram: `{dyn_hist}`")
    report_lines.append(f"- QEMU logs: `{dyn_stdout}` / `{dyn_stderr}`\n")
    if linux_version_line:
        report_lines.append(f"- Linux version: `{linux_version_line}`\n")

    report_lines.append("## Static\n")
    if static_md.exists():
        report_lines.append(f"See: `{static_md}`\n")
    else:
        report_lines.append("- N/A\n")

    report_lines.append("## Dynamic\n")
    if dyn_md.exists():
        report_lines.append(f"See: `{dyn_md}`\n")
    else:
        report_lines.append("- N/A\n")

    report_md.write_text("\n".join(report_lines) + "\n", encoding="utf-8")

    print(f"ok: wrote {report_md}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
