#!/usr/bin/env python3
"""
Fail if legacy (v0.1) trap ABI terminology appears in v0.2 "current" artifacts.

This is a lightweight drift gate intended to catch:
  - EBPC/ETPC/EBPCN references (removed from v0.2 contract)
  - legacy TRAPNO E/BI descriptions
  - legacy syscall encoding names (e.g. E_SCALL=16)
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable, List, Pattern, Sequence, Tuple


ALLOWED_EXTS = {
    ".adoc",
    ".c",
    ".cc",
    ".cpp",
    ".h",
    ".hpp",
    ".json",
    ".md",
    ".opc",
    ".py",
    ".S",
    ".s",
    ".sh",
    ".decode",
    ".txt",
}

SKIP_BASENAMES = {
    "Linx Update.txt",
    "Exception Model Update.txt",
}


def _local_targets(root: Path) -> List[Path]:
    return [
        root / "isa" / "spec" / "current" / "linxisa-v0.3.json",
        root / "isa" / "golden" / "v0.2",
        root / "isa" / "generated" / "codecs",
        root / "docs" / "architecture" / "isa-manual" / "src",
        root / "docs" / "bringup",
        root / "tests",
        root / "tools",
    ]


def _extra_targets(root: Path) -> List[Path]:
    targets: List[Path] = []

    # Linux repo.
    p = root / "arch" / "linx"
    if p.exists():
        targets.append(p)

    # QEMU repo.
    p = root / "target" / "linx"
    if p.exists():
        targets.append(p)
    p = root / "hw" / "linx"
    if p.exists():
        targets.append(p)

    # llvm-project repo can be rooted at llvm-project or llvm/.
    llvm_root = root / "llvm" if (root / "llvm").is_dir() else root
    for rel in [
        ("lib", "Target", "LinxISA"),
        ("test", "MC", "LinxISA"),
        ("test", "CodeGen", "LinxISA"),
    ]:
        p = llvm_root.joinpath(*rel)
        if p.exists():
            targets.append(p)

    return targets


def _should_skip(path: Path) -> bool:
    if path.name in SKIP_BASENAMES:
        return True
    # Keep historical free-form notes out of gate coverage.
    if path.suffix == ".txt" and "LinxISA" in str(path):
        return True
    return False


def _iter_files(targets: Sequence[Path]) -> Iterable[Path]:
    for t in targets:
        if t.is_file():
            if t.suffix in ALLOWED_EXTS and not _should_skip(t):
                yield t
            continue
        if not t.exists():
            continue
        for p in t.rglob("*"):
            if not p.is_file():
                continue
            if p.suffix in ALLOWED_EXTS and not _should_skip(p):
                yield p


def _read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return path.read_text(encoding="latin-1", errors="replace")


def _scan_root(
    scan_root: Path,
    targets: Sequence[Path],
    checks: Sequence[Tuple[str, Pattern[str], List[Path]]],
) -> List[str]:
    failures: List[str] = []
    for path in sorted(set(_iter_files(targets))):
        text = _read_text(path)
        rel = path.relative_to(scan_root)
        for label, pat, allow in checks:
            if any(path.resolve() == a.resolve() for a in allow):
                continue
            for m in pat.finditer(text):
                start = m.start()
                line = text.count("\n", 0, start) + 1
                failures.append(f"{scan_root.name}/{rel}:{line}: {label}: {m.group(0)!r}")
                if len([f for f in failures if f.startswith(f"{scan_root.name}/{rel}:")]) > 20:
                    break
    return failures


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=".", help="Repo root (default: .)")
    ap.add_argument(
        "--extra-root",
        action="append",
        default=[],
        help="Additional repo roots to scan (e.g. ~/linux, ~/qemu, ~/llvm-project)",
    )
    ap.add_argument(
        "--fail-missing-extra",
        action="store_true",
        help="Fail if an --extra-root path does not exist",
    )
    args = ap.parse_args()

    root = Path(args.root).resolve()

    # (label, pattern, allow_paths)
    checks: List[Tuple[str, Pattern[str], List[Path]]] = [
        (
            "legacy trap-save SSR name",
            re.compile(r"\\b(EBPC|ETPC|EBPCN)\\b"),
            [
                # Allow mention only in the changelog chapter.
                root
                / "docs"
                / "architecture"
                / "isa-manual"
                / "src"
                / "chapters"
                / "98_changelog.adoc"
            ],
        ),
        (
            "legacy TRAPNO E/BI description",
            re.compile(r"(BI\\s*\\[62\\])|(E\\s*=\\s*sync)|(E\\s*=\\s*synchronous)", re.IGNORECASE),
            [],
        ),
        (
            "legacy syscall encoding name",
            re.compile(r"\\bE_SCALL\\b|SCALL\\s*=\\s*16", re.IGNORECASE),
            [],
        ),
        (
            "legacy ISA version tag",
            re.compile(r"\\bv0\\.1\\b", re.IGNORECASE),
            [
                # Historical/reasoning references that intentionally mention v0.1.
                root
                / "docs"
                / "architecture"
                / "isa-manual"
                / "src"
                / "chapters"
                / "98_changelog.adoc",
                root / "docs" / "bringup" / "contracts" / "linxisa_v0_2_profile_lock.md",
                root / "tools" / "isa" / "README.md",
                root / "tools" / "isa" / "check_no_legacy_v02.py",
            ],
        ),
    ]

    failures: List[str] = []
    missing_extra: List[str] = []

    local_targets = _local_targets(root)
    failures.extend(_scan_root(root, local_targets, checks))

    for raw in args.extra_root:
        extra = Path(raw).expanduser().resolve()
        if not extra.exists():
            missing_extra.append(str(extra))
            continue
        targets = _extra_targets(extra)
        if not targets:
            # Fallback: scan whole tree when layout is unknown.
            targets = [extra]
        failures.extend(_scan_root(extra, targets, checks))

    if missing_extra and args.fail_missing_extra:
        for p in missing_extra:
            print(f"missing extra root: {p}", file=sys.stderr)
        return 1

    if failures:
        for f in failures[:200]:
            print(f, file=sys.stderr)
        if len(failures) > 200:
            print(f"... {len(failures) - 200} more", file=sys.stderr)
        return 1

    if missing_extra:
        for p in missing_extra:
            print(f"note: skipped missing extra root: {p}")
    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
