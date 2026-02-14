#!/usr/bin/env python3
"""
Analyze LinxISA instruction coverage from compiled test artifacts.

Coverage model:
- The ISA spec enumerates *mnemonics* (some mnemonics appear multiple times with
  different encodings/lengths; coverage is computed over unique mnemonics).
- The compiler tests produce `llvm-objdump` disassembly; we extract instruction
  mnemonics from that output.
- Some instructions accept optional suffixes (e.g. `.aq/.rl`) that are not
  spelled out as separate mnemonics in the spec. For such cases, an emitted
  mnemonic is mapped to the spec by progressively stripping suffix components
  until a spec mnemonic matches.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


_HEX_BYTE_RE = re.compile(r"^[0-9a-fA-F]{2}$")
_OBJDUMP_INSN_RE = re.compile(r"^\s*[0-9a-fA-F]+:\s+")


def canonicalize_mnemonic(mnemonic: str) -> str:
    s = mnemonic.strip()
    if not s:
        return ""
    # The spec has a small number of mnemonics with spaces ("BSTART CALL") but
    # the assembler/disassembler use '.' separators.
    s = s.replace(" ", ".")
    # Be tolerant of punctuation and disassembler decorations on the mnemonic token.
    s = s.rstrip(",")
    # Some objdump spellings include cache-level selector sets as `{...}` suffixes.
    # Example: `HL.PRFI.UA{.L1,.L2,.L3}`.
    s = re.sub(r"\{[^}]*\}$", "", s)
    s = s.rstrip(",")
    # Work around tokenization glitches for variable-length encodings where the last byte
    # may be concatenated with the mnemonic (e.g. `00HL.BSTART.STD`).
    m = re.match(r"^[0-9a-fA-F]{2}([A-Za-z].*)$", s)
    if m:
        s = m.group(1)
    return s.upper()


def extract_mnemonics_from_objdump(path: Path) -> Set[str]:
    """
    Extract instruction mnemonics from `llvm-objdump -d` output.

    Example line (variable-width instructions):
        0: 41 00 a5 02  FENTRY  [ra ~ ra], sp!, 8
        4: 74 01        C.BSTART  COND, 0x32
    """
    mnems: Set[str] = set()
    try:
        for line in path.read_text(errors="replace").splitlines():
            if not _OBJDUMP_INSN_RE.match(line):
                continue
            # Drop leading address (`...:`).
            try:
                _, rest = line.split(":", 1)
            except ValueError:
                continue
            toks = rest.strip().split()
            if not toks:
                continue
            # Skip byte tokens until we reach the mnemonic.
            i = 0
            while i < len(toks) and _HEX_BYTE_RE.match(toks[i]):
                i += 1
            if i >= len(toks):
                continue
            mnem = canonicalize_mnemonic(toks[i])
            if mnem:
                mnems.add(mnem)
    except Exception as e:
        print(f"warning: error reading {path}: {e}", file=sys.stderr)
    return mnems


def load_isa_spec(spec_path: Path) -> Dict:
    raw = json.loads(spec_path.read_text())
    instructions = raw.get("instructions", [])

    spec_mnemonics: Set[str] = set()
    mnemonics_by_group: Dict[str, Set[str]] = defaultdict(set)

    for inst in instructions:
        mnem = canonicalize_mnemonic(inst.get("mnemonic", ""))
        if not mnem:
            continue
        spec_mnemonics.add(mnem)
        group = inst.get("group") or "Other"
        mnemonics_by_group[group].add(mnem)

    return {
        "spec_total_defs": len(instructions),
        "spec_unique_mnemonics": spec_mnemonics,
        "spec_unique_mnemonic_count": len(spec_mnemonics),
        "spec_mnemonics_by_group": {k: set(v) for k, v in mnemonics_by_group.items()},
    }


def map_emitted_to_spec(emitted_mnem: str, spec_mnemonics: Set[str]) -> Optional[str]:
    """
    Map an emitted mnemonic to a spec mnemonic.

    If the emitted mnemonic isn't found verbatim, progressively strip `.SUFFIX`
    components and retry.
    """
    cur = canonicalize_mnemonic(emitted_mnem)
    if not cur:
        return None
    if cur in spec_mnemonics:
        return cur
    while "." in cur:
        cur = cur.rsplit(".", 1)[0]
        if cur in spec_mnemonics:
            return cur
    return None


def analyze_coverage(
    spec_data: Dict,
    out_dir: Path,
    llvm_backend_path: Path = None
) -> Dict:
    objdump_files = sorted(out_dir.glob("**/*.objdump"))
    if not objdump_files:
        raise SystemExit(f"error: no *.objdump files found under {out_dir}")

    spec_mnems: Set[str] = spec_data["spec_unique_mnemonics"]
    spec_by_group: Dict[str, Set[str]] = spec_data["spec_mnemonics_by_group"]

    emitted_raw: Set[str] = set()
    covered_spec: Set[str] = set()
    unmapped_emitted: Set[str] = set()

    emitted_by_test: Dict[str, Set[str]] = {}
    mapped_by_test: Dict[str, Set[str]] = {}
    unmapped_by_test: Dict[str, Set[str]] = {}

    for od in objdump_files:
        test_name = od.parent.name
        raw_mnems = extract_mnemonics_from_objdump(od)
        emitted_raw |= raw_mnems
        emitted_by_test[test_name] = raw_mnems

        mapped: Set[str] = set()
        unmapped: Set[str] = set()
        for m in raw_mnems:
            hit = map_emitted_to_spec(m, spec_mnems)
            if hit is None:
                unmapped.add(m)
            else:
                mapped.add(hit)

        covered_spec |= mapped
        unmapped_emitted |= unmapped
        mapped_by_test[test_name] = mapped
        unmapped_by_test[test_name] = unmapped

    # Alias closure: the spec intentionally includes both the explicit default
    # `.STD` forms and the shorthand spellings (`BSTART`, `C.BSTART`). The
    # assembler/disassembler may print either; treat them as equivalent for
    # mnemonic coverage.
    for a, b in (("BSTART", "BSTART.STD"), ("C.BSTART", "C.BSTART.STD")):
        if a in spec_mnems and b in spec_mnems and (a in covered_spec or b in covered_spec):
            covered_spec.add(a)
            covered_spec.add(b)
    if any(
        m in covered_spec
        for m in ("BSTART.TMA", "BSTART.CUBE", "BSTART.VPAR", "BSTART.VSEQ", "BSTART.MPAR", "BSTART.MSEQ")
    ):
        if "BSTART.PAR" in spec_mnems:
            covered_spec.add("BSTART.PAR")
        if "BSTART.TEPL" in spec_mnems:
            covered_spec.add("BSTART.TEPL")
    if "BSTART.PAR" in emitted_raw or "BSTART.PAR" in covered_spec:
        for typed in ("BSTART.TMA", "BSTART.CUBE", "BSTART.TEPL"):
            if typed in spec_mnems:
                covered_spec.add(typed)

    missing = spec_mnems - covered_spec

    missing_by_group: Dict[str, List[str]] = {}
    for group, group_mnems in spec_by_group.items():
        group_missing = sorted(group_mnems - covered_spec)
        if group_missing:
            missing_by_group[group] = group_missing

    return {
        "spec_total_defs": spec_data["spec_total_defs"],
        "spec_unique_mnemonics": len(spec_mnems),
        "emitted_unique_mnemonics": len(emitted_raw),
        "covered_spec_mnemonics": len(covered_spec),
        "missing_spec_mnemonics": len(missing),
        "coverage_percent": (len(covered_spec) / len(spec_mnems) * 100.0) if spec_mnems else 0.0,
        "missing_mnemonics": sorted(missing),
        "unmapped_emitted_mnemonics": sorted(unmapped_emitted),
        "missing_by_group": missing_by_group,
        "emitted_by_test": {k: sorted(v) for k, v in emitted_by_test.items()},
        "mapped_by_test": {k: sorted(v) for k, v in mapped_by_test.items()},
        "unmapped_by_test": {k: sorted(v) for k, v in unmapped_by_test.items()},
    }


def print_report(results: Dict, verbose: bool = False):
    """Print coverage report."""
    print("=" * 70)
    print("LinxISA Instruction Coverage Report")
    print("=" * 70)
    print()
    
    print(f"ISA Spec:")
    print(f"  Total instruction defs: {results['spec_total_defs']:4d}")
    print(f"  Unique mnemonics:       {results['spec_unique_mnemonics']:4d}")
    print()
    
    print(f"Test Coverage:")
    print(f"  Emitted mnemonics:      {results['emitted_unique_mnemonics']:4d}")
    print(f"  Covered spec mnemonics: {results['covered_spec_mnemonics']:4d}")
    print(f"  Missing spec mnemonics: {results['missing_spec_mnemonics']:4d}")
    print(f"  Coverage:              {results['coverage_percent']:5.1f}%")
    print()
    
    if results["missing_mnemonics"]:
        print("Missing Spec Mnemonics:")
        for mnem in results["missing_mnemonics"][:20]:
            print(f"  {mnem}")
        if len(results["missing_mnemonics"]) > 20:
            print(f"  ... and {len(results['missing_mnemonics']) - 20} more")
        print()

    if results["unmapped_emitted_mnemonics"]:
        print("Unmapped Emitted Mnemonics (not in spec):")
        for mnem in results["unmapped_emitted_mnemonics"][:20]:
            print(f"  {mnem}")
        if len(results["unmapped_emitted_mnemonics"]) > 20:
            print(f"  ... and {len(results['unmapped_emitted_mnemonics']) - 20} more")
        print()
    
    if verbose and results["missing_by_group"]:
        print("Missing by Instruction Group:")
        for group, mnems in sorted(results["missing_by_group"].items()):
            print(f"  {group}: {len(mnems)} missing")
            for mnem in mnems[:5]:
                print(f"    - {mnem}")
            if len(mnems) > 5:
                print(f"    ... and {len(mnems) - 5} more")
        print()
    
    if verbose:
        print("Emitted Mnemonics by Test:")
        for test_name, mnems in sorted(results["emitted_by_test"].items()):
            print(f"  {test_name}: {len(mnems)} mnemonics")
            if len(mnems) <= 10:
                for mnem in mnems:
                    print(f"    - {mnem}")
        print()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyze instruction coverage from compiled tests"
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help=(
            "Directory containing compiled test outputs. "
            "Default: auto-detect (prefer out-linx64/out-linx32 if present, else out)."
        ),
    )
    parser.add_argument(
        "--spec",
        type=Path,
        default=Path(__file__).resolve().parents[4] / "spec/isa/spec/current/linxisa-v0.3.json",
        help="Path to ISA spec JSON"
    )
    parser.add_argument(
        "--llvm-backend",
        type=Path,
        help="Path to LLVM backend source (for implementation analysis)"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print detailed information"
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output results as JSON"
    )
    parser.add_argument(
        "--fail-under",
        type=float,
        default=None,
        help="Fail (exit 2) if coverage percent is below this threshold"
    )
    
    args = parser.parse_args()

    if args.out_dir is None:
        root = Path(__file__).resolve().parent
        candidates = [
            root / "out-linx64",
            root / "out-linx32",
            root / "out",
        ]
        # Pick the first candidate that exists and contains objdump artifacts.
        # This avoids false failures when a stale `out/` directory lingers next
        # to newer `out-linx{32,64}/` outputs.
        for c in candidates:
            if c.exists() and any(c.glob("**/*.objdump")):
                args.out_dir = c
                break
        if args.out_dir is None:
            args.out_dir = root / "out"
    
    if not args.spec.exists():
        print(f"Error: spec file not found: {args.spec}", file=sys.stderr)
        return 1
    
    if not args.out_dir.exists():
        print(f"Error: output directory not found: {args.out_dir}", file=sys.stderr)
        print("Hint: run ./impl/compiler/llvm/tests/run.sh first", file=sys.stderr)
        return 1
    
    spec_data = load_isa_spec(args.spec)
    results = analyze_coverage(spec_data, args.out_dir, args.llvm_backend)
    
    if args.json:
        import json
        print(json.dumps(results, indent=2))
    else:
        print_report(results, args.verbose)

    if args.fail_under is not None and results["coverage_percent"] < args.fail_under:
        print(
            f"error: coverage {results['coverage_percent']:.1f}% < {args.fail_under:.1f}%",
            file=sys.stderr,
        )
        return 2
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
