#!/usr/bin/env python3
"""
Fail when legacy aliases leak into canonical v0.3 artifacts.

The v0.3 staged profile requires:
  - canonical typed block starts (`BSTART.<type>`)
  - canonical vector mnemonic family (no L.* / l.*)
  - no legacy `.kill` tile-operand notation in canonical docs/spec/tests
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
    ".ll",
    ".mir",
}


def _local_targets(root: Path) -> List[Path]:
    return [
        root / "spec" / "isa" / "golden" / "v0.3",
        root / "spec" / "isa" / "spec" / "v0.3" / "linxisa-v0.3.json",
        root / "docs" / "architecture" / "isa-manual" / "src" / "chapters" / "02_programming_model.adoc",
        root / "docs" / "architecture" / "isa-manual" / "src" / "chapters" / "04_block_isa.adoc",
        root / "docs" / "architecture" / "isa-manual" / "src" / "chapters" / "08_memory_operations.adoc",
        root / "docs" / "architecture" / "isa-manual" / "src" / "generated",
        root / "docs" / "bringup" / "ALIGNMENT_MATRIX.md",
        root / "docs" / "bringup" / "MATURITY_PLAN.md",
        root / "docs" / "bringup" / "PROGRESS.md",
        root / "tests" / "qemu" / "tests" / "12_v03_vector_tile.c",
        root / "tests" / "qemu" / "run_tests.sh",
        root / "tools" / "regression" / "run.sh",
    ]


def _extra_targets(root: Path) -> List[Path]:
    targets: List[Path] = []

    p = root / "target" / "linx"
    if p.exists():
        targets.append(p)
    p = root / "hw" / "linx"
    if p.exists():
        targets.append(p)

    p = root / "arch" / "linx"
    if p.exists():
        targets.append(p)

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
    p = str(path)
    if "/spec/isa/golden/v0.3/reconcile/" in p or "/isa/golden/v0.3/reconcile/" in p:
        return True
    if path.name in {
        "linxisa-v0.3.txt",
        "linxisa-v0.3-example.asm",
        "Linx Update.txt",
        "Exception Model Update.txt",
        "v03-reject-legacy-alias.s",
    }:
        return True
    if "/llvm/lib/Target/LinxISA/AsmParser/" in p and path.name == "LinxISAAsmParser.cpp":
        return True
    if "/llvm/lib/Target/LinxISA/MCTargetDesc/" in p and path.name == "linxisa_opcodes.c":
        # Compatibility aliases are retained in generated opcode catalogs.
        return True
    if "/spec/isa/golden/v0.3/opcodes/" in p:
        # v0.3 keeps compatibility aliases in opcode sources; canonical-output
        # checks run on docs/spec/tests and generated outputs.
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
                line = text.count("\n", 0, m.start()) + 1
                failures.append(f"{scan_root.name}/{rel}:{line}: {label}: {m.group(0)!r}")
                if len([f for f in failures if f.startswith(f"{scan_root.name}/{rel}:")]) > 20:
                    break
    return failures


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=".", help="Repo root")
    ap.add_argument(
        "--extra-root",
        action="append",
        default=[],
        help="Additional repo roots to scan (qemu/linux/llvm-project)",
    )
    ap.add_argument(
        "--fail-missing-extra",
        action="store_true",
        help="Fail if an --extra-root path does not exist",
    )
    args = ap.parse_args()

    root = Path(args.root).resolve()

    checks: List[Tuple[str, Pattern[str], List[Path]]] = [
        (
            "legacy vector mnemonic family",
            re.compile(r"\b[Ll]\.[A-Za-z0-9_.]+\b"),
            [
                root / "tools" / "isa" / "reconcile_v03_raw.py",
                root / "tools" / "isa" / "normalize_v03_example_asm.py",
                root / "tools" / "isa" / "check_no_legacy_v03.py",
            ],
        ),
        (
            "legacy tile-kill annotation",
            re.compile(r"\.kill\b"),
            [],
        ),
        (
            "legacy trap-save SSR name",
            re.compile(r"\b(EBPC|ETPC|EBPCN)\b"),
            [],
        ),
        (
            "non-canonical PAR block-start spelling",
            re.compile(r"\bBSTART\.PAR\b"),
            [
                root / "tools" / "isa" / "reconcile_v03_raw.py",
                root / "tools" / "isa" / "normalize_v03_example_asm.py",
                root / "tools" / "isa" / "check_no_legacy_v03.py",
                root / "spec" / "isa" / "spec" / "v0.3" / "linxisa-v0.3.json",
                root / "spec" / "isa" / "spec" / "current" / "linxisa-v0.3.json",
            ],
        ),
    ]

    failures: List[str] = []
    missing_extra: List[str] = []

    failures.extend(_scan_root(root, _local_targets(root), checks))

    for raw in args.extra_root:
        extra = Path(raw).expanduser().resolve()
        if not extra.exists():
            missing_extra.append(str(extra))
            continue
        targets = _extra_targets(extra)
        if not targets:
            targets = [extra]
        failures.extend(_scan_root(extra, targets, checks))

    if missing_extra and args.fail_missing_extra:
        for p in missing_extra:
            print(f"missing extra root: {p}", file=sys.stderr)
        return 2

    if failures:
        for f in failures[:200]:
            print(f, file=sys.stderr)
        if len(failures) > 200:
            print(f"... {len(failures) - 200} more", file=sys.stderr)
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
