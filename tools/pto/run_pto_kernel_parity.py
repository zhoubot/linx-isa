#!/usr/bin/env python3

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT = Path(__file__).resolve()
REPO_ROOT = SCRIPT.parents[2]
OUT_DIR = REPO_ROOT / "workloads" / "generated"

KERNEL_NAMES = [
    "tload_store",
    "mamulb",
    "tmatmul_acc",
    "gemm",
    "gemm_basic",
    "gemm_demo",
    "gemm_performance",
    "add_custom",
    "flash_attention",
    "flash_attention_demo",
    "flash_attention_masked",
    "fa_performance",
    "mla_attention_demo",
]

KERNEL_SOURCES = [
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
]

DIGEST_RE = re.compile(r"PTO_DIGEST\s+([A-Za-z0-9_]+)\s+0x([0-9A-Fa-f]+)")


def run(
    cmd: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    timeout: float | None = None,
) -> subprocess.CompletedProcess[str]:
    p = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout,
        check=False,
    )
    return p


def parse_digests(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for m in DIGEST_RE.finditer(text):
        out[m.group(1)] = "0x" + m.group(2).upper()
    return out


def host_compiler_works(compiler: str) -> bool:
    p = subprocess.run(
        [compiler, "-x", "c++", "-std=c++17", "-", "-c", "-o", os.devnull],
        input="int main(){return 0;}\n",
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return p.returncode == 0


def pick_clangxx() -> str:
    env = os.environ.get("CLANGXX")
    if env:
        if host_compiler_works(env):
            return env
        raise SystemExit(
            f"error: CLANGXX={env} cannot compile host C++ code; "
            "set CLANGXX to a host compiler (for example /usr/bin/clang++)"
        )

    candidates: list[str] = []
    clangxx = shutil.which("clang++")
    if clangxx:
        candidates.append(clangxx)
    cxx = shutil.which("c++")
    if cxx:
        candidates.append(cxx)

    local = REPO_ROOT / "compiler" / "llvm" / "build-linxisa-clang" / "bin" / "clang++"
    if local.exists():
        candidates.append(str(local))

    for cand in candidates:
        if host_compiler_works(cand):
            return cand

    raise SystemExit(
        "error: no usable host C++ compiler found; "
        "install clang++ or set CLANGXX=/path/to/host-clang++"
    )


def build_and_run_host(clangxx: str, host_bin: Path) -> tuple[dict[str, str], str]:
    sources = [
        str(REPO_ROOT / "avs/qemu/tests/16_pto_kernel_parity.cpp"),
        *[str(REPO_ROOT / s) for s in KERNEL_SOURCES],
    ]
    cmd = [
        clangxx,
        "-std=c++17",
        "-O2",
        "-DPTO_HOST_SIM=1",
        "-DPTO_QEMU_SMOKE=1",
        f"-I{REPO_ROOT / 'lib/pto/include'}",
        *sources,
        "-o",
        str(host_bin),
    ]
    p = run(cmd)
    if p.returncode != 0:
        sys.stderr.write(p.stderr)
        raise SystemExit("error: host parity build failed")

    r = run([str(host_bin)])
    if r.returncode != 0:
        sys.stderr.write(r.stdout)
        sys.stderr.write(r.stderr)
        raise SystemExit("error: host parity binary failed")

    text = (r.stdout or "") + "\n" + (r.stderr or "")
    return parse_digests(text), text


def run_qemu_suite(timeout_s: float) -> tuple[dict[str, str], str, list[str]]:
    cmd = [
        "python3",
        str(REPO_ROOT / "avs/qemu/run_tests.py"),
        "--suite",
        "pto_parity",
        "--timeout",
        str(timeout_s),
        "--verbose",
    ]
    compile_and_run_timeout = timeout_s + 120.0
    try:
        p = run(cmd, cwd=REPO_ROOT, timeout=compile_and_run_timeout)
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or "") + "\n" + (exc.stderr or "")
        if out:
            sys.stderr.write(out)
        raise SystemExit(
            "error: qemu pto_parity suite timed out "
            f"(timeout={compile_and_run_timeout:.1f}s)"
        )
    text = (p.stdout or "") + "\n" + (p.stderr or "")
    if p.returncode != 0:
        sys.stderr.write(text)
        raise SystemExit(f"error: qemu pto_parity suite failed (exit={p.returncode})")
    return parse_digests(text), text, cmd


def write_reports(host: dict[str, str], qemu: dict[str, str], qemu_cmd: list[str], host_log: str, qemu_log: str) -> tuple[Path, Path, bool]:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    json_path = OUT_DIR / "pto_kernel_parity_latest.json"
    md_path = OUT_DIR / "pto_kernel_parity_latest.md"

    rows = []
    ok = True
    for name in KERNEL_NAMES:
        hv = host.get(name)
        qv = qemu.get(name)
        match = hv is not None and qv is not None and hv == qv
        if not match:
            ok = False
        rows.append(
            {
                "kernel": name,
                "host_digest": hv,
                "qemu_digest": qv,
                "match": match,
            }
        )

    payload = {
        "generated_at_utc": dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "profile": "smoke",
        "expected_kernels": KERNEL_NAMES,
        "host_digest_count": len(host),
        "qemu_digest_count": len(qemu),
        "all_match": ok,
        "qemu_command": qemu_cmd,
        "results": rows,
    }
    json_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    lines = [
        "# PTO Kernel Parity (Host vs QEMU)",
        "",
        f"- Generated (UTC): `{payload['generated_at_utc']}`",
        "- Profile: `PTO_QEMU_SMOKE=1`",
        f"- All match: `{'YES' if ok else 'NO'}`",
        "",
        "| Kernel | Host Digest | QEMU Digest | Match |",
        "|---|---|---|---|",
    ]
    for r in rows:
        lines.append(
            f"| `{r['kernel']}` | `{r['host_digest'] or 'MISSING'}` | `{r['qemu_digest'] or 'MISSING'}` | `{'yes' if r['match'] else 'no'}` |"
        )
    lines += [
        "",
        "## Raw Logs",
        "",
        "### Host",
        "```text",
        host_log.strip(),
        "```",
        "",
        "### QEMU",
        "```text",
        qemu_log.strip(),
        "```",
        "",
    ]
    md_path.write_text("\n".join(lines), encoding="utf-8")

    return json_path, md_path, ok


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Run PTO kernel parity (host-sim vs QEMU).")
    parser.add_argument("--timeout", type=float, default=180.0, help="QEMU timeout seconds")
    args = parser.parse_args(argv)

    clangxx = pick_clangxx()
    host_bin = OUT_DIR / "pto_kernel_parity_host"

    host_digests, host_log = build_and_run_host(clangxx, host_bin)
    qemu_digests, qemu_log, qemu_cmd = run_qemu_suite(args.timeout)

    json_path, md_path, ok = write_reports(
        host_digests,
        qemu_digests,
        qemu_cmd,
        host_log,
        qemu_log,
    )

    print(f"wrote {json_path}")
    print(f"wrote {md_path}")
    if not ok:
        print("parity mismatch detected")
        return 1
    print("parity matched for all kernels")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
