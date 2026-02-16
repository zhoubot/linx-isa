#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]


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


def _default_qemu() -> Path:
    cands = [
        REPO_ROOT / "emulator" / "qemu" / "build" / "qemu-system-linx64",
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


def _run(cmd: list[str], *, verbose: bool, **kwargs) -> subprocess.CompletedProcess[bytes]:
    if verbose:
        print("+", " ".join(shlex.quote(c) for c in cmd), file=sys.stderr)
    return subprocess.run(cmd, check=False, **kwargs)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Validate Linx call/ret contract traps (negative) and no-fault paths (positive) in QEMU."
    )
    parser.add_argument("--clang", default=str(_default_clang()))
    parser.add_argument("--lld", default=str(_default_lld()))
    parser.add_argument("--qemu", default=str(_default_qemu()))
    parser.add_argument("--target", default="linx64-linx-none-elf")
    parser.add_argument("--timeout", type=float, default=3.0)
    parser.add_argument(
        "--positive-timeout",
        type=float,
        default=0.5,
        help="Timeout used for positive (no-fault) cases.",
    )
    parser.add_argument(
        "--out-dir",
        default=str(SCRIPT_DIR / "out" / "callret-contract"),
    )
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args(argv)

    clang = Path(os.path.expanduser(args.clang)).resolve()
    lld = Path(os.path.expanduser(args.lld)).absolute()
    qemu = Path(os.path.expanduser(args.qemu)).resolve()
    out_dir = Path(os.path.expanduser(args.out_dir)).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    _check_exe(clang, "clang")
    _check_exe(lld, "ld.lld")
    _check_exe(qemu, "qemu-system-linx64")

    src = SCRIPT_DIR / "tests" / "15_callret_contract_negative.S"
    if not src.exists():
        raise SystemExit(f"error: missing source: {src}")

    cases = [
        ("call_bad_target", 1, "bad_target"),
        ("setret_invalid_sequence", 2, "cause=8"),
        ("ret_missing_setctgt", 3, "cause=9"),
        ("ind_missing_setctgt", 4, "cause=9"),
        ("icall_missing_setctgt", 5, "cause=9"),
        ("ret_to_bad_target", 6, "bad_target"),
        ("ret_setctgt_bad_target", 7, "bad_target"),
        ("ind_setctgt_bad_target", 8, "bad_target"),
        ("icall_setctgt_bad_target", 9, "bad_target"),
        ("duplicate_setret", 10, "cause=8"),
        ("setret_noncall_sequence", 11, "cause=8"),
        ("call_missing_setret", 12, "cause=7"),
        ("call_delayed_setret", 13, "cause=8"),
        ("icall_delayed_setret", 14, "cause=8"),
        ("icall_missing_setret", 15, "cause=7"),
        ("hl_call_missing_setret", 16, "cause=7"),
        ("hl_call_delayed_setret", 17, "cause=8"),
        ("valid_call_header", 18, "no_fault"),
        ("valid_icall_header", 19, "no_fault"),
        ("valid_ret_setctgt", 20, "no_fault"),
        ("valid_ind_setctgt", 21, "no_fault"),
        ("valid_hl_call_header", 22, "no_fault"),
        ("hl_setret_invalid_sequence", 23, "cause=8"),
        ("valid_hl_setret_header", 24, "no_fault"),
        ("hl_call_delayed_hl_setret", 25, "cause=8"),
        ("valid_hl_icall_setret_header", 26, "no_fault"),
    ]

    failures: list[str] = []
    for name, case_id, expected in cases:
        case_dir = out_dir / name
        case_dir.mkdir(parents=True, exist_ok=True)
        obj = case_dir / f"{name}.o"
        kern = case_dir / f"{name}.kernel.o"
        compile_log = case_dir / "compile.log"
        qemu_log = case_dir / "qemu.log"

        compile_cmds = [
            [
                str(clang),
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
                f"-DCASE={case_id}",
                "-c",
                str(src),
                "-o",
                str(obj),
            ],
            [str(lld), "-r", "-o", str(kern), str(obj)],
        ]

        with compile_log.open("w", encoding="utf-8") as fp:
            rc = 0
            for cmd in compile_cmds:
                fp.write("+ " + shlex.join(cmd) + "\n")
                proc = _run(cmd, verbose=args.verbose, stdout=fp, stderr=subprocess.STDOUT)
                rc = proc.returncode
                if rc != 0:
                    break
        if rc != 0:
            failures.append(f"{name}: compile failed ({compile_log})")
            continue

        qemu_cmd = [
            str(qemu),
            "-machine",
            "virt",
            "-kernel",
            str(kern),
            "-nographic",
            "-monitor",
            "none",
            "-no-reboot",
            "-d",
            "guest_errors",
        ]
        timed_out = False
        qemu_rc = 0
        case_timeout = args.positive_timeout if expected == "no_fault" else args.timeout
        try:
            proc = _run(
                qemu_cmd,
                verbose=args.verbose,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=case_timeout,
            )
            qemu_rc = proc.returncode
            text = proc.stdout.decode("utf-8", errors="replace")
        except subprocess.TimeoutExpired as exc:
            timed_out = True
            data = exc.output if isinstance(exc.output, (bytes, bytearray)) else b""
            text = data.decode("utf-8", errors="replace")

        qemu_log.write_text(text, encoding="utf-8")

        if expected == "bad_target":
            ok = ("invalid branch target" in text) or ("branch target violation" in text)
            if not ok:
                failures.append(
                    f"{name}: expected bad-branch-target evidence not observed ({qemu_log})"
                )
            continue

        if expected == "no_fault":
            bad_markers = (
                "block-format fault",
                "invalid branch target",
                "branch target violation",
                "[linx trap]",
                "Kernel panic",
            )
            if any(m in text for m in bad_markers):
                failures.append(f"{name}: unexpected trap/fault observed ({qemu_log})")
                continue
            if (not timed_out) and qemu_rc != 0:
                failures.append(f"{name}: qemu exited with rc={qemu_rc} ({qemu_log})")
            continue

        if "block-format fault" not in text or expected not in text:
            failures.append(f"{name}: expected block fault {expected} not observed ({qemu_log})")

    if failures:
        for item in failures:
            print("FAIL:", item, file=sys.stderr)
        return 1

    print(f"PASS: call/ret contract traps validated in {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
