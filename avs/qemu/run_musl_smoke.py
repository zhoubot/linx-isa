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
_EXTRA_SUMMARY_PATH: Path | None = None

SAMPLES: dict[str, dict[str, str]] = {
    "malloc_printf": {
        "src": "linux_musl_malloc_printf.c",
        "start": "MUSL_SMOKE_START",
        "pass": "MUSL_SMOKE_PASS",
    },
    "callret": {
        "src": "linux_musl_callret_matrix.c",
        "start": "MUSL_CALLRET_START",
        "pass": "MUSL_CALLRET_PASS",
    },
    "cpp17_smoke": {
        "src": "linux_musl_cpp17_smoke.cpp",
        "start": "MUSL_CPP17_START",
        "pass": "MUSL_CPP17_PASS",
    },
}


def _default_clang() -> Path:
    cands = [
        Path("/Users/zhoubot/llvm-project/build-linxisa-clang/bin/clang"),
        REPO_ROOT / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "clang",
    ]
    for p in cands:
        if p.exists():
            return p
    return cands[0]


def _default_lld() -> Path:
    cands = [
        Path("/Users/zhoubot/llvm-project/build-linxisa-clang/bin/ld.lld"),
        REPO_ROOT / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "ld.lld",
    ]
    for p in cands:
        if p.exists():
            return p
    return cands[0]


def _default_clangxx(clang: Path) -> Path:
    cands = [
        Path(str(clang).replace("/clang", "/clang++")),
        Path("/Users/zhoubot/llvm-project/build-linxisa-clang/bin/clang++"),
        REPO_ROOT / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "clang++",
    ]
    for p in cands:
        if p.exists():
            return p
    return cands[0]


def _default_qemu() -> Path:
    cands = [
        Path("/Users/zhoubot/qemu/build/qemu-system-linx64"),
        Path("/Users/zhoubot/qemu/build-tci/qemu-system-linx64"),
        REPO_ROOT / "emulator" / "qemu" / "build" / "qemu-system-linx64",
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


def _find_first_file(cands: list[Path]) -> Path | None:
    for p in cands:
        if p.exists():
            return p
    return None


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
    link_mode = payload.get("link_mode")
    result = payload.get("result")
    mode_results = payload.get("mode_results")
    if isinstance(link_mode, str) and isinstance(result, dict) and isinstance(mode_results, dict):
        mode_results[link_mode] = {
            "ok": bool(result.get("ok", False)),
            "classification": str(result.get("classification", "not_run")),
        }
    text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    path.write_text(text, encoding="utf-8")
    if _EXTRA_SUMMARY_PATH and _EXTRA_SUMMARY_PATH != path:
        _EXTRA_SUMMARY_PATH.write_text(text, encoding="utf-8")


def _select_samples(raw_samples: list[str] | None) -> list[str]:
    if not raw_samples:
        return ["malloc_printf"]
    if "all" in raw_samples:
        return list(SAMPLES.keys())
    return list(dict.fromkeys(raw_samples))


def _run_split_link_modes(args: argparse.Namespace, out_dir: Path, selected_samples: list[str]) -> int:
    mode_results: dict[str, Any] = {}
    runner = Path(__file__).resolve()

    for link_mode in ("static", "shared"):
        cmd = [
            sys.executable,
            str(runner),
            "--linux-root",
            args.linux_root,
            "--musl-root",
            args.musl_root,
            "--clang",
            args.clang,
            "--clangxx",
            args.clangxx,
            "--lld",
            args.lld,
            "--qemu",
            args.qemu,
            "--target",
            args.target,
            "--image-base",
            args.image_base,
            "--mode",
            args.mode,
            "--link",
            link_mode,
            "--callret-crossstack",
            args.callret_crossstack,
            "--timeout",
            str(args.timeout),
            "--out-dir",
            str(out_dir),
        ]
        for sample in selected_samples:
            cmd.extend(["--sample", sample])

        run_log = out_dir / f"run_{link_mode}.log"
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        output = proc.stdout.decode("utf-8", errors="replace")
        run_log.write_text(output, encoding="utf-8")

        mode_summary_path = out_dir / f"summary_{link_mode}.json"
        mode_summary: dict[str, Any] = {}
        if mode_summary_path.exists():
            try:
                mode_summary = json.loads(mode_summary_path.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                mode_summary = {}
        mode_result = mode_summary.get("result", {}) if isinstance(mode_summary, dict) else {}
        mode_ok = bool(mode_result.get("ok", False)) and proc.returncode == 0
        mode_class = str(mode_result.get("classification", "runtime_not_recorded"))
        if proc.returncode != 0 and mode_class == "runtime_pass":
            mode_class = "runtime_subprocess_failure"

        mode_results[link_mode] = {
            "ok": mode_ok,
            "classification": mode_class,
            "exit_code": proc.returncode,
            "summary": str(mode_summary_path),
            "run_log": str(run_log),
            "command": shlex.join(cmd),
        }

    overall_ok = all(mode_results[m]["ok"] for m in ("static", "shared"))
    summary: dict[str, Any] = {
        "schema_version": "musl-smoke-v2",
        "mode": args.mode,
        "target": args.target,
        "samples": selected_samples,
        "paths": {
            "linux_root": str(Path(os.path.expanduser(args.linux_root)).resolve()),
            "musl_root": str(Path(os.path.expanduser(args.musl_root)).resolve()),
            "clang": str(Path(os.path.expanduser(args.clang)).resolve()),
            "clangxx": str(Path(os.path.expanduser(args.clangxx)).resolve()),
            "lld": str(Path(os.path.expanduser(args.lld)).absolute()),
            "qemu": str(Path(os.path.expanduser(args.qemu)).resolve()),
            "out_dir": str(out_dir),
            "image_base": args.image_base,
            "link": "both",
        },
        "link_modes": ["static", "shared"],
        "mode_results": mode_results,
        "result": {
            "ok": overall_ok,
            "classification": "runtime_pass" if overall_ok else "runtime_mode_failure",
        },
    }
    _write_summary(out_dir / "summary.json", summary)

    if overall_ok:
        print(f"ok: musl smoke passed ({out_dir / 'summary.json'})")
        return 0

    print(f"error: musl smoke failed ({out_dir / 'summary.json'})", file=sys.stderr)
    return 2


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Run Linx musl malloc/printf runtime smoke on Linux+initramfs+QEMU.")
    parser.add_argument("--linux-root", default="/Users/zhoubot/linux")
    parser.add_argument("--musl-root", default="/Users/zhoubot/linx-isa/lib/musl")
    parser.add_argument("--clang", default=str(_default_clang()))
    parser.add_argument("--clangxx", default="")
    parser.add_argument("--lld", default=str(_default_lld()))
    parser.add_argument("--qemu", default=str(_default_qemu()))
    parser.add_argument("--target", default="linx64-unknown-linux-musl")
    parser.add_argument("--image-base", default="0x40000000")
    parser.add_argument("--mode", choices=["phase-a", "phase-b"], default="phase-b")
    parser.add_argument("--link", choices=["static", "shared", "both"], default="both")
    parser.add_argument(
        "--callret-crossstack",
        choices=["off", "check", "strict"],
        default="strict",
        help="Cross-stack Linux call/ret audit mode for --sample callret.",
    )
    parser.add_argument(
        "--sample",
        action="append",
        choices=[*SAMPLES.keys(), "all"],
        help="Runtime sample(s) to run (default: malloc_printf). Repeatable.",
    )
    parser.add_argument("--timeout", type=int, default=90)
    parser.add_argument(
        "--append",
        default="lpj=1000000 loglevel=1 console=ttyS0 kfence.sample_interval=0",
        help="Kernel command line used for QEMU runtime boot.",
    )
    parser.add_argument(
        "--out-dir",
        default="/Users/zhoubot/linx-isa/avs/qemu/out/musl-smoke",
    )
    args = parser.parse_args(argv)

    linux_root = Path(os.path.expanduser(args.linux_root)).resolve()
    musl_root = Path(os.path.expanduser(args.musl_root)).resolve()
    clang = Path(os.path.expanduser(args.clang)).resolve()
    clangxx = (
        Path(os.path.expanduser(args.clangxx)).resolve()
        if args.clangxx
        else _default_clangxx(clang).resolve()
    )
    lld = Path(os.path.expanduser(args.lld)).absolute()
    qemu = Path(os.path.expanduser(args.qemu)).resolve()
    out_dir = Path(os.path.expanduser(args.out_dir)).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    selected_samples = _select_samples(args.sample)
    args.clang = str(clang)
    args.clangxx = str(clangxx)
    args.lld = str(lld)
    args.qemu = str(qemu)
    args.out_dir = str(out_dir)

    if args.link == "both":
        return _run_split_link_modes(args, out_dir, selected_samples)

    global _EXTRA_SUMMARY_PATH
    _EXTRA_SUMMARY_PATH = out_dir / f"summary_{args.link}.json"

    summary: dict[str, Any] = {
        "schema_version": "musl-smoke-v2",
        "mode": args.mode,
        "target": args.target,
        "paths": {
            "linux_root": str(linux_root),
            "musl_root": str(musl_root),
            "clang": str(clang),
            "clangxx": str(clangxx),
            "lld": str(lld),
            "qemu": str(qemu),
            "out_dir": str(out_dir),
            "image_base": args.image_base,
            "link": args.link,
        },
        "link_mode": args.link,
        "stages": [],
        "result": {"ok": False, "classification": "not_run"},
    }
    summary["samples"] = selected_samples
    summary_path = out_dir / "summary.json"

    def add_stage(name: str, status: str, detail: str, log: str | None = None) -> None:
        item: dict[str, str] = {"name": name, "status": status, "detail": detail}
        if log:
            item["log"] = log
        summary["stages"].append(item)
        _write_summary(summary_path, summary)

    _check_exe(clang, "clang")
    _check_exe(clangxx, "clang++")
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
    runtime_lib = REPO_ROOT / "out" / "libc" / "musl" / "runtime" / args.mode / "liblinx_builtin_rt.a"
    env_compile = os.environ.copy()
    env_compile["PATH"] = f"{lld.parent}:{env_compile.get('PATH', '')}"
    kernel = _find_kernel(linux_root)
    gen_init_cpio = _find_gen_init_cpio(linux_root, out_dir)

    if args.link == "both":
        link_modes = ["static", "shared"]
    else:
        link_modes = [args.link]
    summary["link_modes"] = link_modes
    summary["mode_results"] = {args.link: {"ok": False, "classification": "not_run"}}

    if "callret" in selected_samples and args.callret_crossstack != "off":
        audit_script = REPO_ROOT / "tools" / "ci" / "check_linx_callret_crossstack.sh"
        if not audit_script.exists():
            add_stage("callret-crossstack", "fail", f"missing audit script: {audit_script}")
            summary["result"] = {"ok": False, "classification": "callret_crossstack_script_missing"}
            _write_summary(summary_path, summary)
            return 2

        audit_log = out_dir / "callret_crossstack.log"
        audit_env = os.environ.copy()
        # Whole-vmlinux disassembly audit is expensive and can be enabled
        # explicitly when needed; keep object-level contract checks on by default.
        audit_env["LINX_AUDIT_VMLINUX"] = os.environ.get("LINX_AUDIT_VMLINUX", "0")
        if args.callret_crossstack == "strict":
            audit_env["LINX_STRICT_CALLRET_RELOCS"] = "1"

        with audit_log.open("w", encoding="utf-8") as fp:
            cmd = [str(audit_script), str(linux_root)]
            fp.write("+ " + shlex.join(cmd) + "\n")
            rc = subprocess.run(
                cmd,
                env=audit_env,
                stdout=fp,
                stderr=subprocess.STDOUT,
                check=False,
            ).returncode
        if rc != 0:
            add_stage(
                "callret-crossstack",
                "fail",
                f"linux call/ret cross-stack audit failed (mode={args.callret_crossstack})",
                str(audit_log),
            )
            summary["result"] = {
                "ok": False,
                "classification": "callret_linux_crossstack_contract_failure",
            }
            _write_summary(summary_path, summary)
            return 2
        add_stage(
            "callret-crossstack",
            "pass",
            f"linux call/ret cross-stack audit passed (mode={args.callret_crossstack})",
            str(audit_log),
        )

    if not runtime_lib.exists():
        add_stage("sample-compile", "fail", f"missing runtime builtins archive: {runtime_lib}")
        summary["result"] = {"ok": False, "classification": "runtime_builtins_missing"}
        _write_summary(summary_path, summary)
        return 2

    for sample_name in selected_samples:
        sample_meta = SAMPLES[sample_name]
        sample_src = SCRIPT_DIR / "tests" / sample_meta["src"]
        if not sample_src.exists():
            add_stage("sample-compile", "fail", f"missing sample source: {sample_src}")
            summary["result"] = {"ok": False, "classification": f"{sample_name}_source_missing"}
            _write_summary(summary_path, summary)
            return 2

        for link_mode in link_modes:
            sample_bin = out_dir / f"{sample_name}_{link_mode}"
            sample_obj = out_dir / f"{sample_src.stem}_{link_mode}.o"
            compile_log = out_dir / f"compile_{sample_name}_{link_mode}.log"
            is_cpp = sample_src.suffix in {".cc", ".cpp", ".cxx"}
            compile_tool = clangxx if is_cpp else clang

            shared_lib = sysroot / "lib" / "libc.so"
            shared_loader = sysroot / "lib" / "ld-musl-linx64.so.1"
            lib_search = [
                sysroot / "lib",
                sysroot / "usr/lib",
                REPO_ROOT / "out" / "cpp-runtime" / "musl-cxx17-noeh" / "install" / "lib",
                REPO_ROOT / "out" / "cpp-runtime" / "musl-cxx17-noeh" / "install" / "usr/lib",
            ]
            cpp_static_libs: list[str] = []
            cpp_unwindlib = "none"
            cpp_include_dir: Path | None = None
            if is_cpp:
                libcxx = _find_first_file([d / "libc++.a" for d in lib_search])
                libcxxabi = _find_first_file([d / "libc++abi.a" for d in lib_search])
                libunwind = _find_first_file([d / "libunwind.a" for d in lib_search])
                if libcxx is None or libcxxabi is None:
                    add_stage(
                        f"sample-compile[{sample_name}:{link_mode}]",
                        "fail",
                        "missing static C++ runtime archives (libc++, libc++abi)",
                    )
                    summary["result"] = {
                        "ok": False,
                        "classification": f"{sample_name}_{link_mode}_cpp_runtime_lib_missing",
                    }
                    _write_summary(summary_path, summary)
                    return 2
                cpp_static_libs = [str(libcxx), str(libcxxabi)]
                if libunwind is not None:
                    cpp_static_libs.append(str(libunwind))
                    cpp_unwindlib = "libunwind"
                cpp_include_dir = _find_first_file(
                    [
                        sysroot / "include" / "c++" / "v1",
                        sysroot / "usr" / "include" / "c++" / "v1",
                        REPO_ROOT
                        / "out"
                        / "cpp-runtime"
                        / "musl-cxx17-noeh"
                        / "install"
                        / "include"
                        / "c++"
                        / "v1",
                    ]
                )
                if cpp_include_dir is None:
                    add_stage(
                        f"sample-compile[{sample_name}:{link_mode}]",
                        "fail",
                        "missing C++ headers (c++/v1) in sysroot/runtime overlay",
                    )
                    summary["result"] = {
                        "ok": False,
                        "classification": f"{sample_name}_{link_mode}_cpp_headers_missing",
                    }
                    _write_summary(summary_path, summary)
                    return 2

            compile_sample_cmd = [
                str(compile_tool),
                "-target",
                args.target,
                "--sysroot",
                str(sysroot),
            ]
            if is_cpp:
                compile_sample_cmd += [
                    "-std=c++17",
                    "-fno-exceptions",
                    "-fno-rtti",
                    "-nostdinc++",
                    "-isystem",
                    str(cpp_include_dir),
                    "-stdlib=libc++",
                    "-rtlib=compiler-rt",
                    "-unwindlib=" + cpp_unwindlib,
                ]
            compile_sample_cmd += [
                "-c",
                str(sample_src),
                "-o",
                str(sample_obj),
            ]

            if link_mode == "static":
                if is_cpp:
                    link_cmd = [
                        str(clangxx),
                        "-target",
                        args.target,
                        "--sysroot",
                        str(sysroot),
                        "-std=c++17",
                        "-fno-exceptions",
                        "-fno-rtti",
                        "-static",
                        "-no-pie",
                        "-unwindlib=" + cpp_unwindlib,
                        "-fuse-ld=lld",
                        "-nostdlib",
                        str(sysroot / "lib" / "crt1.o"),
                        str(sysroot / "lib" / "crti.o"),
                        str(sample_obj),
                        str(runtime_lib),
                        *cpp_static_libs,
                        str(sysroot / "lib" / "libc.a"),
                        str(sysroot / "lib" / "crtn.o"),
                        f"-Wl,--image-base={args.image_base}",
                        "-o",
                        str(sample_bin),
                    ]
                else:
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
                        str(runtime_lib),
                        str(sysroot / "lib" / "libc.a"),
                        str(sysroot / "lib" / "crtn.o"),
                        f"-Wl,--image-base={args.image_base}",
                        "-o",
                        str(sample_bin),
                    ]
            else:
                if not shared_lib.exists() or not shared_loader.exists():
                    add_stage(
                        f"sample-compile[{sample_name}:{link_mode}]",
                        "fail",
                        f"missing shared runtime artifacts: {shared_lib} / {shared_loader}",
                    )
                    summary["result"] = {
                        "ok": False,
                        "classification": f"{sample_name}_{link_mode}_runtime_artifacts_missing",
                    }
                    _write_summary(summary_path, summary)
                    return 2
                if is_cpp:
                    link_cmd = [
                        str(clangxx),
                        "-target",
                        args.target,
                        "--sysroot",
                        str(sysroot),
                        "-std=c++17",
                        "-fno-exceptions",
                        "-fno-rtti",
                        "-unwindlib=" + cpp_unwindlib,
                        "-fuse-ld=lld",
                        "-nostdlib",
                        str(sysroot / "lib" / "crt1.o"),
                        str(sysroot / "lib" / "crti.o"),
                        str(sample_obj),
                        str(runtime_lib),
                        *cpp_static_libs,
                        "-L" + str(sysroot / "lib"),
                        "-L" + str(sysroot / "usr/lib"),
                        "-lc",
                        str(sysroot / "lib" / "crtn.o"),
                        "-Wl,--dynamic-linker=/lib/ld-musl-linx64.so.1",
                        f"-Wl,--image-base={args.image_base}",
                        "-o",
                        str(sample_bin),
                    ]
                else:
                    link_cmd = [
                        str(clang),
                        "-target",
                        args.target,
                        "--sysroot",
                        str(sysroot),
                        "-fuse-ld=lld",
                        "-nostdlib",
                        str(sysroot / "lib" / "crt1.o"),
                        str(sysroot / "lib" / "crti.o"),
                        str(sample_obj),
                        str(runtime_lib),
                        "-L" + str(sysroot / "lib"),
                        "-L" + str(sysroot / "usr/lib"),
                        "-lc",
                        str(sysroot / "lib" / "crtn.o"),
                        "-Wl,--dynamic-linker=/lib/ld-musl-linx64.so.1",
                        f"-Wl,--image-base={args.image_base}",
                        "-o",
                        str(sample_bin),
                    ]

            with compile_log.open("w", encoding="utf-8") as fp:
                rc = 0
                for cmd in [compile_sample_cmd, link_cmd]:
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
                add_stage(
                    f"sample-compile[{sample_name}:{link_mode}]",
                    "fail",
                    "failed to compile/link musl sample",
                    str(compile_log),
                )
                summary["result"] = {
                    "ok": False,
                    "classification": f"{sample_name}_{link_mode}_sample_compile_failure",
                }
                _write_summary(summary_path, summary)
                return 2
            add_stage(
                f"sample-compile[{sample_name}:{link_mode}]",
                "pass",
                f"built {sample_bin}",
                str(compile_log),
            )

            initramfs_list = out_dir / f"initramfs_{sample_name}_{link_mode}.list"
            initramfs = out_dir / f"initramfs_{sample_name}_{link_mode}.cpio"
            initramfs_log = out_dir / f"initramfs_{sample_name}_{link_mode}.log"

            init_lines = [
                "dir /dev 0755 0 0",
                "nod /dev/console 0600 0 0 c 5 1",
                "nod /dev/null 0666 0 0 c 1 3",
                "nod /dev/ttyS0 0600 0 0 c 4 64",
                "dir /proc 0755 0 0",
                "dir /sys 0755 0 0",
                "dir /run 0755 0 0",
                "dir /tmp 1777 0 0",
                f"file /init {sample_bin} 0755 0 0",
            ]
            if link_mode == "shared":
                loader_src = shared_loader if shared_loader.exists() else shared_lib
                init_lines += [
                    "dir /lib 0755 0 0",
                    f"file /lib/libc.so {shared_lib} 0755 0 0",
                    f"file /lib/ld-musl-linx64.so.1 {loader_src} 0755 0 0",
                ]
            init_lines.append("")
            initramfs_list.write_text("\n".join(init_lines), encoding="utf-8")

            cmd_gen = [str(gen_init_cpio), "-o", str(initramfs), str(initramfs_list)]
            with initramfs_log.open("w", encoding="utf-8") as fp:
                fp.write("+ " + shlex.join(cmd_gen) + "\n")
                rc = subprocess.run(cmd_gen, stdout=fp, stderr=subprocess.STDOUT, check=False).returncode
            if rc != 0:
                add_stage(
                    f"initramfs[{sample_name}:{link_mode}]",
                    "fail",
                    "failed to create initramfs",
                    str(initramfs_log),
                )
                summary["result"] = {
                    "ok": False,
                    "classification": f"{sample_name}_{link_mode}_initramfs_generation_failure",
                }
                _write_summary(summary_path, summary)
                return 2
            add_stage(
                f"initramfs[{sample_name}:{link_mode}]",
                "pass",
                f"built {initramfs}",
                str(initramfs_log),
            )

            qemu_log = out_dir / f"qemu_{sample_name}_{link_mode}.log"
            qemu_cmd = [
                str(qemu),
                "-machine",
                "virt",
                "-nographic",
                "-monitor",
                "none",
                "-no-reboot",
                "-kernel",
                str(kernel),
                "-initrd",
                str(initramfs),
                "-append",
                args.append,
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

            panic_seen = "Kernel panic - not syncing" in text
            trap_seen = "[linx trap]" in text.lower()
            start_seen = sample_meta["start"] in text
            pass_seen = sample_meta["pass"] in text
            if timed_out and start_seen and pass_seen:
                add_stage(
                    f"qemu-runtime[{sample_name}:{link_mode}]",
                    "pass",
                    f"markers observed before timeout; qemu_rc={qemu_rc}",
                    str(qemu_log),
                )
                continue
            if timed_out:
                if trap_seen and not pass_seen:
                    add_stage(
                        f"qemu-runtime[{sample_name}:{link_mode}]",
                        "fail",
                        f"linx trap before pass marker (timeout after {args.timeout}s)",
                        str(qemu_log),
                    )
                    summary["result"] = {
                        "ok": False,
                        "classification": f"{sample_name}_{link_mode}_runtime_block_trap",
                    }
                    _write_summary(summary_path, summary)
                    return 2
                if panic_seen and not pass_seen:
                    add_stage(
                        f"qemu-runtime[{sample_name}:{link_mode}]",
                        "fail",
                        f"kernel panic before pass marker (timeout after {args.timeout}s)",
                        str(qemu_log),
                    )
                    summary["result"] = {
                        "ok": False,
                        "classification": f"{sample_name}_{link_mode}_runtime_kernel_panic",
                    }
                    _write_summary(summary_path, summary)
                    return 2
                add_stage(
                    f"qemu-runtime[{sample_name}:{link_mode}]",
                    "fail",
                    f"timeout after {args.timeout}s",
                    str(qemu_log),
                )
                summary["result"] = {
                    "ok": False,
                    "classification": f"{sample_name}_{link_mode}_runtime_timeout",
                }
                _write_summary(summary_path, summary)
                return 2
            if panic_seen and not pass_seen:
                add_stage(
                    f"qemu-runtime[{sample_name}:{link_mode}]",
                    "fail",
                    f"kernel panic before pass marker, qemu_rc={qemu_rc}",
                    str(qemu_log),
                )
                summary["result"] = {
                    "ok": False,
                    "classification": f"{sample_name}_{link_mode}_runtime_kernel_panic",
                }
                _write_summary(summary_path, summary)
                return 2
            if trap_seen and not pass_seen:
                add_stage(
                    f"qemu-runtime[{sample_name}:{link_mode}]",
                    "fail",
                    f"linx trap before pass marker, qemu_rc={qemu_rc}",
                    str(qemu_log),
                )
                summary["result"] = {
                    "ok": False,
                    "classification": f"{sample_name}_{link_mode}_runtime_block_trap",
                }
                _write_summary(summary_path, summary)
                return 2
            if not start_seen or not pass_seen:
                classification = f"{sample_name}_{link_mode}_runtime_missing_marker"
                if not start_seen:
                    classification = f"{sample_name}_{link_mode}_runtime_syscall_failure"
                add_stage(
                    f"qemu-runtime[{sample_name}:{link_mode}]",
                    "fail",
                    f"missing markers: start={start_seen} pass={pass_seen}, qemu_rc={qemu_rc}",
                    str(qemu_log),
                )
                summary["result"] = {"ok": False, "classification": classification}
                _write_summary(summary_path, summary)
                return 2

            add_stage(
                f"qemu-runtime[{sample_name}:{link_mode}]",
                "pass",
                f"markers observed; qemu_rc={qemu_rc}",
                str(qemu_log),
            )

    summary["result"] = {"ok": True, "classification": "runtime_pass"}
    _write_summary(summary_path, summary)
    print(f"ok: musl smoke passed ({summary_path})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
