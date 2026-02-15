#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]


def _default_clang() -> Path:
    return Path("/Users/zhoubot/llvm-project/build-linxisa-clang/bin/clang")


def _default_lld() -> Path:
    return Path("/Users/zhoubot/llvm-project/build-linxisa-clang/bin/ld.lld")


def _default_qemu() -> Path:
    cands = [
        Path("/Users/zhoubot/qemu/build/qemu-system-linx64"),
        Path("/Users/zhoubot/qemu/build-tci/qemu-system-linx64"),
    ]
    for p in cands:
        if p.exists():
            return p
    return cands[0]


def _check_exe(path: Path, what: str) -> None:
    if not path.exists():
        raise SystemExit(f"error: {what} not found: {path}")
    if not os.access(path, os.X_OK):
        raise SystemExit(f"error: {what} is not executable: {path}")


def _find_kernel(linux_root: Path) -> Path:
    cands = [
        linux_root / "build-linx-fixed" / "vmlinux",
        linux_root / "build-linx" / "vmlinux",
        linux_root / "vmlinux",
    ]
    for p in cands:
        if p.exists():
            return p
    raise SystemExit(f"error: could not find kernel image under {linux_root}")


def _find_gen_init_cpio(linux_root: Path, out_dir: Path) -> Path:
    cands = [
        linux_root / "build-linx-fixed" / "usr" / "gen_init_cpio",
        linux_root / "usr" / "gen_init_cpio",
    ]
    for p in cands:
        if p.exists():
            return p

    src = linux_root / "usr" / "gen_init_cpio.c"
    if not src.exists():
        raise SystemExit(f"error: missing gen_init_cpio source: {src}")

    host_cc = Path("/usr/bin/clang")
    out_bin = out_dir / "gen_init_cpio"
    subprocess.run(
        [str(host_cc), "-O2", "-Wall", "-Wextra", "-o", str(out_bin), str(src)],
        check=True,
    )
    return out_bin


def _parse_mode_summary(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    if not path.exists():
        return out
    for ln in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" not in ln:
            continue
        k, v = ln.split("=", 1)
        out[k.strip()] = v.strip()
    return out


def _write_summary(path: Path, payload: dict[str, Any]) -> None:
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Run Linx musl malloc/printf runtime smoke on Linux+initramfs+QEMU.")
    parser.add_argument("--linux-root", default="/Users/zhoubot/linux")
    parser.add_argument("--musl-root", default="/Users/zhoubot/linx-isa/lib/musl")
    parser.add_argument("--clang", default=str(_default_clang()))
    parser.add_argument("--lld", default=str(_default_lld()))
    parser.add_argument("--qemu", default=str(_default_qemu()))
    parser.add_argument("--target", default="linx64-unknown-linux-musl")
    parser.add_argument("--image-base", default="0x40000000")
    parser.add_argument("--mode", choices=["phase-a", "phase-b"], default="phase-a")
    parser.add_argument("--timeout", type=int, default=90)
    parser.add_argument(
        "--out-dir",
        default="/Users/zhoubot/linx-isa/avs/qemu/out/musl-smoke",
    )
    args = parser.parse_args(argv)

    linux_root = Path(os.path.expanduser(args.linux_root)).resolve()
    musl_root = Path(os.path.expanduser(args.musl_root)).resolve()
    clang = Path(os.path.expanduser(args.clang)).resolve()
    lld = Path(os.path.expanduser(args.lld)).resolve()
    qemu = Path(os.path.expanduser(args.qemu)).resolve()
    out_dir = Path(os.path.expanduser(args.out_dir)).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    summary: dict[str, Any] = {
        "mode": args.mode,
        "target": args.target,
        "paths": {
            "linux_root": str(linux_root),
            "musl_root": str(musl_root),
            "clang": str(clang),
            "lld": str(lld),
            "qemu": str(qemu),
            "out_dir": str(out_dir),
            "image_base": args.image_base,
        },
        "stages": [],
        "result": {"ok": False, "classification": "not_run"},
    }
    summary_path = out_dir / "summary.json"

    def add_stage(name: str, status: str, detail: str, log: str | None = None) -> None:
        item: dict[str, str] = {"name": name, "status": status, "detail": detail}
        if log:
            item["log"] = log
        summary["stages"].append(item)
        _write_summary(summary_path, summary)

    _check_exe(clang, "clang")
    _check_exe(lld, "ld.lld")
    _check_exe(qemu, "qemu-system-linx64")

    build_script = musl_root / "tools" / "linx" / "build_linx64_musl.sh"
    if not build_script.exists():
        add_stage("musl-build", "fail", f"missing build script: {build_script}")
        summary["result"] = {"ok": False, "classification": "musl_build_script_missing"}
        _write_summary(summary_path, summary)
        return 2

    build_log = out_dir / "musl_build.log"
    env = os.environ.copy()
    env["MODE"] = args.mode
    env["TARGET"] = args.target
    env["LINX_ISA_ROOT"] = str(REPO_ROOT)
    env["CLANG"] = str(clang)
    llvm_bin = clang.parent
    env["LLVM_BIN"] = str(llvm_bin)
    env["AR"] = str(llvm_bin / "llvm-ar")
    env["RANLIB"] = str(llvm_bin / "llvm-ranlib")
    env["NM"] = str(llvm_bin / "llvm-nm")
    env["STRIP"] = str(llvm_bin / "llvm-strip")
    env["READELF"] = str(llvm_bin / "llvm-readelf")

    with build_log.open("w", encoding="utf-8") as fp:
        fp.write("+ " + shlex.join([str(build_script)]) + "\n")
        rc = subprocess.run(
            [str(build_script)],
            cwd=str(musl_root),
            env=env,
            stdout=fp,
            stderr=subprocess.STDOUT,
            check=False,
        ).returncode

    mode_summary = _parse_mode_summary(REPO_ROOT / "out" / "libc" / "musl" / "logs" / f"{args.mode}-summary.txt")
    if rc != 0:
        classification = "musl_build_failure"
        if mode_summary.get("m1") == "fail":
            classification = "musl_configure_failure"
        elif mode_summary.get("m2") == "fail":
            classification = "musl_static_lib_failure"
        add_stage("musl-build", "fail", f"musl build failed (mode={args.mode})", str(build_log))
        summary["result"] = {"ok": False, "classification": classification}
        _write_summary(summary_path, summary)
        return 2
    add_stage("musl-build", "pass", f"m1={mode_summary.get('m1', 'unknown')} m2={mode_summary.get('m2', 'unknown')} m3={mode_summary.get('m3', 'unknown')}", str(build_log))

    sysroot = REPO_ROOT / "out" / "libc" / "musl" / "install" / args.mode
    sample_src = SCRIPT_DIR / "tests" / "linux_musl_malloc_printf.c"
    sample_bin = out_dir / "musl_smoke"
    runtime_obj_dir = out_dir / "runtime-obj"
    runtime_obj_dir.mkdir(parents=True, exist_ok=True)
    compile_log = out_dir / "compile.log"

    sample_obj = runtime_obj_dir / "linux_musl_malloc_printf.o"
    softfp_src = REPO_ROOT / "avs" / "runtime" / "freestanding" / "src" / "softfp" / "softfp.c"
    softfp_obj = runtime_obj_dir / "softfp.o"
    atomic_src = REPO_ROOT / "avs" / "runtime" / "freestanding" / "src" / "atomic" / "atomic_builtins.c"
    atomic_obj = runtime_obj_dir / "atomic_builtins.o"
    runtime_inc = REPO_ROOT / "avs" / "runtime" / "freestanding" / "include"

    compile_sample_cmd = [
        str(clang),
        "-target",
        args.target,
        "--sysroot",
        str(sysroot),
        "-c",
        str(sample_src),
        "-o",
        str(sample_obj),
    ]
    compile_softfp_cmd = [
        str(clang),
        "-target",
        args.target,
        "--sysroot",
        str(sysroot),
        "-ffreestanding",
        "-fno-builtin",
        "-O2",
        "-I",
        str(runtime_inc),
        "-c",
        str(softfp_src),
        "-o",
        str(softfp_obj),
    ]
    compile_atomic_cmd = [
        str(clang),
        "-target",
        args.target,
        "--sysroot",
        str(sysroot),
        "-ffreestanding",
        "-fno-builtin",
        "-O2",
        "-I",
        str(runtime_inc),
        "-c",
        str(atomic_src),
        "-o",
        str(atomic_obj),
    ]
    link_cmd = [
        str(clang),
        "-target",
        args.target,
        "--sysroot",
        str(sysroot),
        "-static",
        "-fuse-ld=lld",
        "-nostdlib",
        str(sysroot / "lib" / "crt1.o"),
        str(sysroot / "lib" / "crti.o"),
        str(sample_obj),
        str(softfp_obj),
        str(atomic_obj),
        str(sysroot / "lib" / "libc.a"),
        str(sysroot / "lib" / "crtn.o"),
        f"-Wl,--image-base={args.image_base}",
        "-o",
        str(sample_bin),
    ]
    env_compile = os.environ.copy()
    env_compile["PATH"] = f"{lld.parent}:{env_compile.get('PATH', '')}"
    with compile_log.open("w", encoding="utf-8") as fp:
        rc = 0
        for cmd in [compile_sample_cmd, compile_softfp_cmd, compile_atomic_cmd, link_cmd]:
            fp.write("+ " + shlex.join(cmd) + "\n")
            rc = subprocess.run(
                cmd,
                env=env_compile,
                stdout=fp,
                stderr=subprocess.STDOUT,
                check=False,
            ).returncode
            if rc != 0:
                break
    if rc != 0:
        add_stage("sample-compile", "fail", "failed to compile/link musl smoke sample", str(compile_log))
        summary["result"] = {"ok": False, "classification": "sample_compile_failure"}
        _write_summary(summary_path, summary)
        return 2
    add_stage("sample-compile", "pass", f"built {sample_bin}", str(compile_log))

    kernel = _find_kernel(linux_root)
    gen_init_cpio = _find_gen_init_cpio(linux_root, out_dir)
    initramfs_list = out_dir / "initramfs.list"
    initramfs = out_dir / "initramfs.cpio"
    initramfs_log = out_dir / "initramfs.log"

    initramfs_list.write_text(
        "\n".join(
            [
                "dir /dev 0755 0 0",
                "nod /dev/console 0600 0 0 c 5 1",
                "nod /dev/null 0666 0 0 c 1 3",
                "nod /dev/ttyS0 0600 0 0 c 4 64",
                "dir /proc 0755 0 0",
                "dir /sys 0755 0 0",
                "dir /run 0755 0 0",
                "dir /tmp 1777 0 0",
                f"file /init {sample_bin} 0755 0 0",
                "",
            ]
        ),
        encoding="utf-8",
    )

    cmd_gen = [str(gen_init_cpio), "-o", str(initramfs), str(initramfs_list)]
    with initramfs_log.open("w", encoding="utf-8") as fp:
        fp.write("+ " + shlex.join(cmd_gen) + "\n")
        rc = subprocess.run(cmd_gen, stdout=fp, stderr=subprocess.STDOUT, check=False).returncode
    if rc != 0:
        add_stage("initramfs", "fail", "failed to create initramfs", str(initramfs_log))
        summary["result"] = {"ok": False, "classification": "initramfs_generation_failure"}
        _write_summary(summary_path, summary)
        return 2
    add_stage("initramfs", "pass", f"built {initramfs}", str(initramfs_log))

    qemu_log = out_dir / "qemu.log"
    qemu_cmd = [
        str(qemu),
        "-machine",
        "virt",
        "-nographic",
        "-monitor",
        "none",
        "-kernel",
        str(kernel),
        "-initrd",
        str(initramfs),
        "-append",
        "lpj=1000000 loglevel=1 console=ttyS0",
    ]

    text = ""
    qemu_rc = 124
    timed_out = False
    try:
        proc = subprocess.run(
            qemu_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=args.timeout,
            check=False,
        )
        qemu_rc = proc.returncode
        text = proc.stdout.decode("utf-8", errors="replace")
    except subprocess.TimeoutExpired as exc:
        timed_out = True
        data = exc.output if isinstance(exc.output, (bytes, bytearray)) else b""
        text = data.decode("utf-8", errors="replace")

    qemu_log.write_text(text, encoding="utf-8")

    start_seen = "MUSL_SMOKE_START" in text
    pass_seen = "MUSL_SMOKE_PASS" in text
    if timed_out:
        add_stage("qemu-runtime", "fail", f"timeout after {args.timeout}s", str(qemu_log))
        summary["result"] = {"ok": False, "classification": "runtime_timeout"}
        _write_summary(summary_path, summary)
        return 2
    if not start_seen or not pass_seen:
        classification = "runtime_missing_marker"
        if not start_seen:
            classification = "runtime_syscall_failure"
        add_stage(
            "qemu-runtime",
            "fail",
            f"missing markers: start={start_seen} pass={pass_seen}, qemu_rc={qemu_rc}",
            str(qemu_log),
        )
        summary["result"] = {"ok": False, "classification": classification}
        _write_summary(summary_path, summary)
        return 2

    add_stage("qemu-runtime", "pass", f"markers observed; qemu_rc={qemu_rc}", str(qemu_log))
    summary["result"] = {"ok": True, "classification": "runtime_pass"}
    _write_summary(summary_path, summary)
    print(f"ok: musl smoke passed ({summary_path})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
