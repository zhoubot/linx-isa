#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]


SUITES: dict[str, dict[str, str]] = {
    "arithmetic": {"src": "tests/01_arithmetic.c", "macro": "LINX_TEST_ENABLE_ARITHMETIC"},
    "bitwise": {"src": "tests/02_bitwise.c", "macro": "LINX_TEST_ENABLE_BITWISE"},
    "loadstore": {"src": "tests/03_loadstore.c", "macro": "LINX_TEST_ENABLE_LOADSTORE"},
    "branch": {"src": "tests/04_branch.c", "macro": "LINX_TEST_ENABLE_BRANCH"},
    "move": {"src": "tests/05_move.c", "macro": "LINX_TEST_ENABLE_MOVE"},
    "float": {"src": "tests/06_floating_point.c", "macro": "LINX_TEST_ENABLE_FLOAT"},
    "atomic": {"src": "tests/07_atomic.c", "macro": "LINX_TEST_ENABLE_ATOMIC"},
    "jumptable": {"src": "tests/08_jumptable.c", "macro": "LINX_TEST_ENABLE_JUMPTABLE"},
    "varargs": {"src": "tests/09_varargs.c", "macro": "LINX_TEST_ENABLE_VARARGS"},
    "tile": {"src": "tests/10_tile_matmul.cpp", "macro": "LINX_TEST_ENABLE_TILE"},
    "pto_parity": {"src": "tests/16_pto_kernel_parity.cpp", "macro": "LINX_TEST_ENABLE_PTO_PARITY"},
    "system": {"src": "tests/11_system.c", "macro": "LINX_TEST_ENABLE_SYSTEM"},
    "v03_vector": {"src": "tests/12_v03_vector_tile.c", "macro": "LINX_TEST_ENABLE_V03_VECTOR"},
    "v03_vector_ops": {
        "src": "tests/13_v03_vector_ops_matrix.c",
        "macro": "LINX_TEST_ENABLE_V03_VECTOR_OPS",
    },
    "callret": {"src": "tests/14_callret.c", "macro": "LINX_TEST_ENABLE_CALLRET"},
}

COMPILE_ONLY_SUITE_SOURCE_OVERRIDE: dict[str, str] = {
    # Runtime tile stress currently relies on backend paths that are still
    # unstable for compile-only regression gating; keep a dedicated compile
    # smoke that validates PTO kernel integration.
    "tile": "tests/10_tile_compile_smoke.cpp",
}

EXTRA_SOURCES_BY_SUITE: dict[str, list[str]] = {
    "tile": [
        "workloads/pto_kernels/tload_store.cpp",
        "workloads/pto_kernels/mamulb.cpp",
        "workloads/pto_kernels/tmatmul_acc.cpp",
        "workloads/pto_kernels/gemm.cpp",
        "workloads/pto_kernels/flash_attention.cpp",
        "workloads/pto_kernels/flash_attention_masked.cpp",
    ],
    "callret": [
        "avs/qemu/tests/14_callret_templates.S",
    ],
    "pto_parity": [
        "workloads/pto_kernels/tload_store.cpp",
        "workloads/pto_kernels/mamulb.cpp",
        "workloads/pto_kernels/tmatmul_acc.cpp",
        "workloads/pto_kernels/gemm.cpp",
        "workloads/pto_kernels/gemm_basic.cpp",
        "workloads/pto_kernels/gemm_demo.cpp",
        "workloads/pto_kernels/gemm_performance.cpp",
        "workloads/pto_kernels/add_custom.cpp",
        "workloads/pto_kernels/flash_attention.cpp",
        "workloads/pto_kernels/flash_attention_demo.cpp",
        "workloads/pto_kernels/flash_attention_masked.cpp",
        "workloads/pto_kernels/fa_performance.cpp",
        "workloads/pto_kernels/mla_attention_demo.cpp",
    ],
}

EXPERIMENTAL_SUITES: set[str] = {
    # Requires tile builtin-enabled clang and PTO bridge headers.
    "tile",
    "pto_parity",
}

CORE_SUITES: list[str] = [
    "arithmetic",
]


def _parse_test_id(text: str) -> int:
    try:
        value = int(text, 0)
    except ValueError as e:
        raise SystemExit(f"error: invalid --require-test-id value '{text}': {e}")
    if value < 0 or value > 0xFFFFFFFF:
        raise SystemExit(f"error: --require-test-id out of range (must fit uint32): {text}")
    return value


def _path_or_none(p: str | None) -> Path | None:
    if not p:
        return None
    return Path(os.path.expanduser(p))


def _default_clang() -> Path | None:
    env = os.environ.get("CLANG")
    if env:
        return Path(os.path.expanduser(env))
    cands = [
        REPO_ROOT / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "clang",
        Path.home() / "llvm-project" / "build-linxisa-clang" / "bin" / "clang",
    ]
    for cand in cands:
        if cand.exists():
            return cand
    return None


def _default_clangxx(clang: Path | None) -> Path | None:
    env = os.environ.get("CLANGXX")
    if env:
        return Path(os.path.expanduser(env))
    if clang:
        cand = clang.parent / "clang++"
        if cand.exists():
            return cand
    return None


def _default_lld(clang: Path | None) -> Path | None:
    env = os.environ.get("LLD")
    if env:
        return Path(os.path.expanduser(env))
    if clang:
        cand = clang.parent / "ld.lld"
        if cand.exists():
            return cand
    return None


def _default_qemu() -> Path | None:
    env = os.environ.get("QEMU")
    if env:
        return Path(os.path.expanduser(env))
    cand_local = REPO_ROOT / "emulator" / "qemu" / "build" / "qemu-system-linx64"
    if cand_local.exists():
        return cand_local
    cand_tci = Path.home() / "qemu" / "build-tci" / "qemu-system-linx64"
    if cand_tci.exists():
        return cand_tci
    cand = Path.home() / "qemu" / "build" / "qemu-system-linx64"
    return cand if cand.exists() else None


def _check_exe(p: Path, what: str) -> None:
    if not p.exists():
        raise SystemExit(f"error: {what} not found: {p}")
    if not os.access(p, os.X_OK):
        raise SystemExit(f"error: {what} not executable: {p}")


def _run(cmd: list[str], *, verbose: bool, **kwargs) -> subprocess.CompletedProcess[bytes]:
    if verbose:
        print("+", " ".join(cmd), file=sys.stderr)
    return subprocess.run(cmd, check=False, **kwargs)


def _tail(data: bytes, max_bytes: int = 4000) -> bytes:
    if len(data) <= max_bytes:
        return data
    return data[-max_bytes:]


def _suite_selection(args: argparse.Namespace) -> list[str]:
    if args.all:
        return [s for s in SUITES.keys() if s not in EXPERIMENTAL_SUITES]

    if args.suite:
        invalid_suites = [s for s in args.suite if s not in SUITES]
        if invalid_suites:
            raise SystemExit(f"error: unsupported --suite {invalid_suites}; use --list-suites")
        return list(dict.fromkeys(args.suite))

    if args.filter:
        try:
            rx = re.compile(args.filter)
        except re.error as e:
            raise SystemExit(f"error: invalid --filter regex: {e}")
        matched: list[str] = []
        for name, meta in SUITES.items():
            if rx.search(name) or rx.search(meta["src"]):
                matched.append(name)
        if not matched:
            raise SystemExit(f"error: --filter matched no suites: {args.filter}")
        return matched

    return CORE_SUITES.copy()


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Compile and run LinxISA QEMU unit tests.")
    parser.add_argument("--clang", default=None, help="Path to clang (env: CLANG)")
    parser.add_argument("--clangxx", default=None, help="Path to clang++ (env: CLANGXX)")
    parser.add_argument("--lld", default=None, help="Path to ld.lld (env: LLD)")
    parser.add_argument("--qemu", default=None, help="Path to qemu-system-linx64 (env: QEMU)")
    parser.add_argument(
        "--target",
        default="linx64-linx-none-elf",
        help="Target triple (default: linx64-linx-none-elf)",
    )
    parser.add_argument("--out-dir", default=str(SCRIPT_DIR / "out"), help="Output directory")
    parser.add_argument("--timeout", type=float, default=5.0, help="QEMU timeout in seconds")
    parser.add_argument("--compile-only", action="store_true", help="Only compile/link; do not run QEMU")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--list-suites", action="store_true", help="List available suites and exit")
    parser.add_argument("--all", action="store_true", help="Enable all suites (including float/atomic)")
    parser.add_argument(
        "--all-suites",
        action="store_true",
        help="Enable all suites, including experimental ones (e.g. tile)",
    )
    parser.add_argument("--suite", action="append", help="Enable only this suite (repeatable)")
    parser.add_argument("--filter", help="Regex to select suites by name or filename")
    parser.add_argument("--qemu-arg", action="append", default=[], help="Extra QEMU arg (repeatable)")
    parser.add_argument(
        "--require-test-id",
        action="append",
        default=[],
        help="Require UART evidence of test_start for this uint32 test id (hex/dec)",
    )
    args = parser.parse_args(argv)

    if args.list_suites:
        for name, meta in SUITES.items():
            suffix = " (experimental)" if name in EXPERIMENTAL_SUITES else ""
            print(f"{name:10} {meta['src']}{suffix}")
        return 0

    clang = _path_or_none(args.clang) or _default_clang()
    if not clang:
        raise SystemExit("error: clang not found; set --clang or CLANG")
    clangxx = _path_or_none(args.clangxx) or _default_clangxx(clang)
    lld = _path_or_none(args.lld) or _default_lld(clang)
    if not lld:
        raise SystemExit("error: ld.lld not found; set --lld or LLD")
    qemu = _path_or_none(args.qemu) or _default_qemu()
    if not qemu and not args.compile_only:
        raise SystemExit("error: qemu-system-linx64 not found; set --qemu or QEMU")

    _check_exe(clang, "clang")
    if clangxx:
        _check_exe(clangxx, "clang++")
    _check_exe(lld, "ld.lld")
    if qemu:
        _check_exe(qemu, "qemu-system-linx64")

    selected = _suite_selection(args)
    if args.all_suites:
        selected = list(SUITES.keys())
    required_test_ids = [_parse_test_id(t) for t in args.require_test_id]

    out_dir = Path(os.path.expanduser(args.out_dir))
    obj_dir = out_dir / "obj"
    out_dir.mkdir(parents=True, exist_ok=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    include_dir = SCRIPT_DIR / "lib"
    libc_include_dir = REPO_ROOT / "avs" / "runtime" / "freestanding" / "include"
    pto_bridge_include_dir: Path | None = None
    bridge_env = os.environ.get("PTO_BRIDGE_INCLUDE")
    if bridge_env:
        bridge_candidate = Path(os.path.expanduser(bridge_env))
        if not bridge_candidate.exists():
            raise SystemExit(
                f"error: PTO_BRIDGE_INCLUDE does not exist: {bridge_candidate}"
            )
        pto_bridge_include_dir = bridge_candidate
    else:
        for bridge_candidate in (
            REPO_ROOT / "lib" / "pto" / "include",
            REPO_ROOT / "tools" / "pto" / "include",
        ):
            if bridge_candidate.exists():
                pto_bridge_include_dir = bridge_candidate
                break
    if any(s in selected for s in ("tile", "pto_parity")) and pto_bridge_include_dir is None:
        raise SystemExit(
            "error: tile suite requires PTO headers; looked for "
            f"{REPO_ROOT / 'tools' / 'pto' / 'include'} and "
            f"{REPO_ROOT / 'lib' / 'pto' / 'include'}"
        )
    pto_include_dir: Path | None = None
    env = os.environ.get("PTO_ISA_INCLUDE")
    if env:
        candidate = Path(os.path.expanduser(env))
        if not candidate.exists():
            raise SystemExit(f"error: PTO_ISA_INCLUDE does not exist: {candidate}")
        pto_include_dir = candidate
    sources: list[Path] = [SCRIPT_DIR / "tests" / "main.c"]
    for suite in selected:
        rel = SUITES[suite]["src"]
        if args.compile_only:
            rel = COMPILE_ONLY_SUITE_SOURCE_OVERRIDE.get(suite, rel)
        sources.append(SCRIPT_DIR / rel)
    for suite in selected:
        for rel in EXTRA_SOURCES_BY_SUITE.get(suite, []):
            sources.append(REPO_ROOT / rel)
    softfp_suites = {"float", "v03_vector", "v03_vector_ops", "tile", "pto_parity"}
    if any(s in softfp_suites for s in selected):
        sources.append(REPO_ROOT / "avs" / "runtime" / "freestanding" / "src" / "softfp" / "softfp.c")

    suite_macros: list[str] = []
    for name, meta in SUITES.items():
        suite_macros.append(f"-D{meta['macro']}={'1' if name in selected else '0'}")
    emit_test_logs = args.verbose or bool(required_test_ids)

    common_cflags = [
        "-target",
        args.target,
        "-O2",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-stack-protector",
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
        "-fno-exceptions",
        "-fno-jump-tables",
        "-nostdlib",
        f"-I{include_dir}",
        f"-I{libc_include_dir}",
        *suite_macros,
        f"-DLINX_TEST_QUIET={'0' if emit_test_logs else '1'}",
    ]
    if any(s in selected for s in ("tile", "pto_parity")):
        # Keep tile-suite bring-up deterministic: SIMT autovec currently
        # triggers a mid-end crash on migrated PTO kernels under strict v0.3.
        common_cflags += ["-mllvm", "-linx-simt-autovec=false"]
    if any(s in selected for s in ("tile", "pto_parity")):
        # Runtime policy: migrated PTO kernels run in smoke profile under QEMU.
        # Full-profile coverage remains in compile/objdump gates.
        common_cflags += ["-DPTO_QEMU_SMOKE=1"]
    if pto_bridge_include_dir is not None:
        common_cflags.append(f"-I{pto_bridge_include_dir}")
    if pto_include_dir:
        common_cflags.append(f"-I{pto_include_dir}")

    objects: list[Path] = []
    for src in sources:
        obj = obj_dir / (src.stem + ".o")
        cflags = list(common_cflags)
        tool = clang
        if src.suffix in {".cc", ".cpp", ".cxx"}:
            if not clangxx:
                raise SystemExit("error: tile suite requires clang++; set --clangxx or CLANGXX")
            tool = clangxx
            cflags.append("-std=c++17")
        # Keep selected sources unoptimized during bring-up when backend passes
        # are still unstable under aggressive optimization.
        if src.name in {"softfp.c"}:
            cflags = [("-O0" if f == "-O2" else f) for f in cflags]
        # Jump table/indirect branch coverage requires allowing jump tables.
        if src.name == "08_jumptable.c":
            cflags = [f for f in cflags if f != "-fno-jump-tables"]
        cmd = [str(tool), *cflags, "-c", str(src), "-o", str(obj)]
        p = _run(cmd, verbose=args.verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if p.returncode != 0:
            sys.stderr.buffer.write(p.stderr)
            raise SystemExit(f"error: compile failed: {src}")
        objects.append(obj)

    out_obj = out_dir / "linx-qemu-tests.o"
    cmd = [str(lld), "-r", "-o", str(out_obj), *[str(o) for o in objects]]
    p = _run(cmd, verbose=args.verbose, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        sys.stderr.buffer.write(p.stderr)
        raise SystemExit("error: link (ld.lld -r) failed")

    print(f"ok: built {out_obj}")
    if args.compile_only:
        return 0

    assert qemu is not None
    qemu_cmd = [
        str(qemu),
        "-machine",
        "virt",
        "-kernel",
        str(out_obj),
        "-nographic",
        "-monitor",
        "none",
        *args.qemu_arg,
    ]

    try:
        p = _run(
            qemu_cmd,
            verbose=args.verbose,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=args.timeout,
        )
    except subprocess.TimeoutExpired as e:
        stdout = e.stdout or b""
        stderr = e.stderr or b""
        sys.stderr.write(f"error: QEMU timeout after {args.timeout:.1f}s\n")
        if stdout:
            sys.stderr.write("---- guest stdout (tail) ----\n")
            sys.stderr.buffer.write(_tail(stdout))
            sys.stderr.write("\n")
        if stderr:
            sys.stderr.write("---- qemu stderr (tail) ----\n")
            sys.stderr.buffer.write(_tail(stderr))
            sys.stderr.write("\n")
        return 124

    if args.verbose or p.returncode != 0:
        if p.stdout:
            sys.stdout.buffer.write(p.stdout)
        if p.stderr:
            sys.stderr.buffer.write(p.stderr)

    if p.returncode == 0:
        if b"REGRESSION PASSED" not in p.stdout:
            sys.stderr.write("warning: exit=0 but did not see 'REGRESSION PASSED' in UART output\n")
            return 2
        if required_test_ids:
            missing: list[int] = []
            for test_id in required_test_ids:
                marker = f"Test 0x{test_id:08X}:".encode()
                if marker not in p.stdout:
                    missing.append(test_id)
            if missing:
                sys.stderr.write(
                    "error: missing required test id marker(s) in UART output: "
                    + ", ".join(f"0x{tid:08X}" for tid in missing)
                    + "\n"
                )
                if not args.verbose and p.stdout:
                    sys.stderr.write("---- guest stdout (tail) ----\n")
                    sys.stderr.buffer.write(_tail(p.stdout))
                    sys.stderr.write("\n")
                return 3
        print("PASS")
        return 0

    sys.stderr.write(f"FAIL (qemu exit={p.returncode})\n")
    if not args.verbose:
        if p.stdout:
            sys.stderr.write("---- guest stdout (tail) ----\n")
            sys.stderr.buffer.write(_tail(p.stdout))
            sys.stderr.write("\n")
        if p.stderr:
            sys.stderr.write("---- qemu stderr (tail) ----\n")
            sys.stderr.buffer.write(_tail(p.stderr))
            sys.stderr.write("\n")
    return p.returncode if p.returncode != 0 else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
