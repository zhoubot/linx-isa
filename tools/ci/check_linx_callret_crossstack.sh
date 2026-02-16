#!/usr/bin/env bash
set -euo pipefail

LINUX_ROOT="${1:-/Users/zhoubot/linux}"
VMLINUX=""
for cand in \
  "$LINUX_ROOT/build-linx-fixed/vmlinux" \
  "$LINUX_ROOT/build-linx/vmlinux" \
  "$LINUX_ROOT/vmlinux"; do
  if [[ -f "$cand" ]]; then
    VMLINUX="$cand"
    break
  fi
done

pick_first_exists() {
  for p in "$@"; do
    if [[ -f "$p" ]]; then
      echo "$p"
      return 0
    fi
  done
  return 1
}

SWITCH_O="$(pick_first_exists \
  "$LINUX_ROOT/build-linx-fixed/arch/linx/kernel/switch_to.o" \
  "$LINUX_ROOT/build-linx/arch/linx/kernel/switch_to.o" \
  "$LINUX_ROOT/arch/linx/kernel/switch_to.o" || true)"
ENTRY_O="$(pick_first_exists \
  "$LINUX_ROOT/build-linx-fixed/arch/linx/kernel/entry.o" \
  "$LINUX_ROOT/build-linx/arch/linx/kernel/entry.o" \
  "$LINUX_ROOT/arch/linx/kernel/entry.o" || true)"

if [[ -n "${LLVM_OBJDUMP:-}" ]]; then
  OBJDUMP="$LLVM_OBJDUMP"
elif [[ -x "/Users/zhoubot/llvm-project/build-linxisa-clang/bin/llvm-objdump" ]]; then
  OBJDUMP="/Users/zhoubot/llvm-project/build-linxisa-clang/bin/llvm-objdump"
else
  OBJDUMP="$(command -v llvm-objdump || true)"
fi

if [[ -z "$OBJDUMP" || ! -x "$OBJDUMP" ]]; then
  echo "error: llvm-objdump not found; set LLVM_OBJDUMP=/path/to/llvm-objdump" >&2
  exit 2
fi

if [[ -n "${LLVM_READELF:-}" ]]; then
  READELF="$LLVM_READELF"
elif [[ -x "/Users/zhoubot/llvm-project/build-linxisa-clang/bin/llvm-readelf" ]]; then
  READELF="/Users/zhoubot/llvm-project/build-linxisa-clang/bin/llvm-readelf"
else
  READELF="$(command -v llvm-readelf || true)"
fi

if [[ -z "$READELF" || ! -x "$READELF" ]]; then
  echo "error: llvm-readelf not found; set LLVM_READELF=/path/to/llvm-readelf" >&2
  exit 2
fi

if [[ -z "$SWITCH_O" || ! -f "$SWITCH_O" ]]; then
  echo "error: missing object for switch_to cross-check: $SWITCH_O" >&2
  exit 2
fi
if [[ -z "$ENTRY_O" || ! -f "$ENTRY_O" ]]; then
  echo "error: missing object for entry cross-check: $ENTRY_O" >&2
  exit 2
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

"$OBJDUMP" -d --triple=linx64 "$SWITCH_O" >"$tmpdir/switch_to.dis"
"$OBJDUMP" -d --triple=linx64 "$ENTRY_O" >"$tmpdir/entry.dis"

audit_objs=("$ENTRY_O")
SELECTED_BUILD_ROOT=""
case "$ENTRY_O" in
  "$LINUX_ROOT/build-linx-fixed/"*)
    SELECTED_BUILD_ROOT="$LINUX_ROOT/build-linx-fixed"
    ;;
  "$LINUX_ROOT/build-linx/"*)
    SELECTED_BUILD_ROOT="$LINUX_ROOT/build-linx"
    ;;
esac
if [[ -n "$SELECTED_BUILD_ROOT" && -f "$SELECTED_BUILD_ROOT/kernel/printk/printk.o" ]]; then
  audit_objs+=("$SELECTED_BUILD_ROOT/kernel/printk/printk.o")
fi
if [[ -n "$SELECTED_BUILD_ROOT" && -f "$SELECTED_BUILD_ROOT/kernel/panic.o" ]]; then
  audit_objs+=("$SELECTED_BUILD_ROOT/kernel/panic.o")
fi

for obj in "${audit_objs[@]}"; do
  dis="$tmpdir/$(basename "$obj").dis"
  rel="$tmpdir/$(basename "$obj").rel"
  "$OBJDUMP" -d --triple=linx64 "$obj" >"$dis"
  "$READELF" -r "$obj" >"$rel"
  if ! python3 - "$dis" "$rel" "$obj" <<'PY'
import re
import sys
import os
import bisect
from pathlib import Path

dis_path = Path(sys.argv[1])
rel_path = Path(sys.argv[2])
obj_path = sys.argv[3]
strict_relocs = os.environ.get("LINX_STRICT_CALLRET_RELOCS", "").lower() in {
    "1",
    "true",
    "yes",
}

addr_re = re.compile(r"^\s*([0-9a-fA-F]+):")
sym_re = re.compile(r"^\s*([0-9a-fA-F]+)\s+<([^>]+)>:")
section_re = re.compile(r"^Disassembly of section (.+):$")
call_pat = re.compile(r"\b(?:HL\.)?BSTART(?:\.STD)?\s+CALL,")
call_tgt_pat = re.compile(r"\bCALL,\s*([^,\s]+)")
ra_pat = re.compile(r"\bra=([^,\s]+)")
rel_re = re.compile(r"^\s*([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+(R_LINX_[A-Z0-9_]+)\b")

call_reloc_types = {
    "R_LINX_B17_PCREL",
    "R_LINX_B17_PLT",
    "R_LINX_HL_BSTART30_PCREL",
}
setret_reloc_types = {
    "R_LINX_CSETRET5_PCREL",
    "R_LINX_SETRET20_PCREL",
    "R_LINX_HL_SETRET32_PCREL",
}

local_syms: set[str] = set()
sym_locs: dict[str, tuple[str, int]] = {}
marker_locs: set[tuple[str, int]] = set()
marker_index_by_section: dict[str, list[int]] = {}
insn_sections: list[str] = []
insn_offsets: list[int] = []
calls = []
current_section = ""
for ln in dis_path.read_text(encoding="utf-8", errors="replace").splitlines():
    sec = section_re.match(ln)
    if sec:
        current_section = sec.group(1)
        continue
    s = sym_re.match(ln)
    if s:
        addr = int(s.group(1), 16)
        sym = s.group(2)
        local_syms.add(sym)
        sym_locs[sym] = (current_section, addr)
    m = addr_re.match(ln)
    if not m:
        continue
    idx = len(insn_offsets)
    off = int(m.group(1), 16)
    insn_sections.append(current_section)
    insn_offsets.append(off)
    low = ln.lower()
    is_marker = ("bstart" in low) or ("fentry" in low) or ("fexit" in low) or ("fret." in low)
    if is_marker:
        marker_locs.add((current_section, off))
        marker_index_by_section.setdefault(current_section, []).append(idx)
    if "CALL" not in ln or "ICALL" in ln or "BSTART" not in ln:
        continue
    if not call_pat.search(ln):
        continue
    is_hl = "HL.BSTART" in ln
    call_tgt_m = call_tgt_pat.search(ln)
    ra_m = ra_pat.search(ln)
    call_tgt = call_tgt_m.group(1) if call_tgt_m else ""
    ra_tgt = ra_m.group(1) if ra_m else ""
    has_ra = bool(ra_tgt)
    calls.append(
        {
            "off": off,
            "is_hl": is_hl,
            "has_ra": has_ra,
            "idx": idx,
            "section": current_section,
            "call_tgt": call_tgt,
            "ra_tgt": ra_tgt,
            "line": ln.strip(),
        }
    )

def _resolve_target(tok: str, call_section: str) -> tuple[str, int] | None:
    if not tok:
        return None
    if tok.startswith("0x"):
        try:
            return (call_section, int(tok, 16))
        except ValueError:
            return None
    return sym_locs.get(tok)


def _next_marker_after(call_section: str, call_idx: int) -> tuple[str, int] | None:
    marker_indices = marker_index_by_section.get(call_section, [])
    pos = bisect.bisect_right(marker_indices, call_idx)
    if pos >= len(marker_indices):
        return None
    next_idx = marker_indices[pos]
    return (insn_sections[next_idx], insn_offsets[next_idx])


relocs: dict[int, set[str]] = {}
for ln in rel_path.read_text(encoding="utf-8", errors="replace").splitlines():
    m = rel_re.match(ln)
    if not m:
        continue
    off = int(m.group(1), 16)
    typ = m.group(2)
    relocs.setdefault(off, set()).add(typ)

missing_call = []
missing_setret = []
missing_ra = []
bad_call_target = []
resolved_call_without_reloc = 0
resolved_setret_without_reloc = 0
for c in calls:
    off = c["off"]
    is_hl = c["is_hl"]
    has_ra = c["has_ra"]
    idx = c["idx"]
    section = c["section"]
    line = c["line"]
    call_tgt = c["call_tgt"]
    ra_tgt = c["ra_tgt"]

    call_tgt_loc = _resolve_target(call_tgt, section)
    call_has_reloc = bool(relocs.get(off, set()) & call_reloc_types)
    call_has_resolved_target = call_tgt_loc is not None
    if not call_has_reloc:
        if not call_has_resolved_target:
            missing_call.append((off, line))
        else:
            resolved_call_without_reloc += 1

    if strict_relocs and call_tgt_loc is not None and call_tgt_loc not in marker_locs:
        bad_call_target.append((section, off, call_tgt_loc[1], line))

    if not has_ra:
        missing_ra.append((off, line))
        continue

    setret_off = off + (6 if is_hl else 4)
    setret_has_reloc = bool(relocs.get(setret_off, set()) & setret_reloc_types)
    ra_tgt_loc = _resolve_target(ra_tgt, section)
    setret_has_resolved_target = ra_tgt_loc is not None
    if not setret_has_reloc:
        if not setret_has_resolved_target:
            missing_setret.append((off, setret_off, line))
        else:
            resolved_setret_without_reloc += 1

if missing_ra or missing_call or missing_setret or bad_call_target:
    print(f"error: call/ret contract audit failed in {obj_path}", file=sys.stderr)
    for off, line in missing_ra[:20]:
        print(f"  missing fused ra=... @0x{off:x}: {line}", file=sys.stderr)
    for off, line in missing_call[:20]:
        print(f"  missing call reloc @0x{off:x}: {line}", file=sys.stderr)
    for call_off, setret_off, line in missing_setret[:20]:
        print(
            f"  missing setret target evidence @0x{setret_off:x} for call @0x{call_off:x}: {line}",
            file=sys.stderr,
        )
    for section, call_off, call_tgt_addr, line in bad_call_target[:20]:
        print(
            f"  call target is not a block-start marker ({section}) @0x{call_off:x} -> 0x{call_tgt_addr:x}: {line}",
            file=sys.stderr,
        )
    if (
        len(missing_ra) > 20
        or len(missing_call) > 20
        or len(missing_setret) > 20
        or len(bad_call_target) > 20
    ):
        print("  ... truncated", file=sys.stderr)
    sys.exit(1)

if not strict_relocs and (resolved_call_without_reloc or resolved_setret_without_reloc):
    print(
        "note: "
        f"{obj_path}: accepted resolved local call/setret targets without relocations "
        f"(call={resolved_call_without_reloc}, setret={resolved_setret_without_reloc})",
        file=sys.stderr,
    )
PY
  then
    exit 1
  fi
done

if ! rg -q 'C\.BSTART(\.STD)?[[:space:]]+IND' "$tmpdir/switch_to.dis"; then
  echo "error: switch_to return path is missing C.BSTART IND marker" >&2
  exit 1
fi
if ! rg -q 'setc\.tgt[[:space:]]+ra' "$tmpdir/switch_to.dis"; then
  echo "error: switch_to return path is missing setc.tgt ra" >&2
  exit 1
fi

if ! rg -q 'BSTART.*CALL' "$tmpdir/entry.dis"; then
  echo "error: entry.o has no BSTART CALL sites to audit" >&2
  exit 1
fi

if rg 'BSTART.*CALL' "$tmpdir/entry.dis" | rg -vq 'ra='; then
  echo "error: found BSTART CALL without fused return target (ra=...)" >&2
  exit 1
fi

if [[ -n "$VMLINUX" && "${LINX_AUDIT_VMLINUX:-0}" == "1" ]]; then
  "$OBJDUMP" -d --triple=linx64 "$VMLINUX" >"$tmpdir/vmlinux.dis"
  if ! python3 - "$tmpdir/vmlinux.dis" <<'PY'
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
lines = path.read_text(encoding="utf-8", errors="replace").splitlines()

addr_re = re.compile(r"^\s*([0-9a-fA-F]+):\s+")
call_re = re.compile(r"\bCALL,\s*0x([0-9a-fA-F]+)\b")

marker_addrs: set[int] = set()
calls: list[tuple[int, int, str]] = []

for ln in lines:
    m = addr_re.match(ln)
    if not m:
        continue
    addr = int(m.group(1), 16)
    low = ln.lower()

    if ("bstart" in low) or ("fentry" in low) or ("fexit" in low) or ("fret." in low):
        marker_addrs.add(addr)

    if "bstart" in low and "call" in low and "icall" not in low:
        if "ra=" not in low:
            calls.append((addr, -1, ln.strip()))
            continue
        t = call_re.search(ln)
        if t:
            calls.append((addr, int(t.group(1), 16), ln.strip()))

bad: list[tuple[int, int, str]] = []
missing_ra: list[tuple[int, str]] = []
for src, tgt, ln in calls:
    if tgt < 0:
        missing_ra.append((src, ln))
        continue
    if tgt not in marker_addrs:
        bad.append((src, tgt, ln))

if missing_ra or bad:
    if missing_ra:
        print("error: CALL headers missing fused ra=... in vmlinux disassembly:", file=sys.stderr)
        for src, ln in missing_ra[:20]:
            print(f"  src=0x{src:x} :: {ln}", file=sys.stderr)
        if len(missing_ra) > 20:
            print(f"  ... and {len(missing_ra) - 20} more", file=sys.stderr)
    if bad:
        print("error: CALL targets that are not block-start markers:", file=sys.stderr)
        for src, tgt, ln in bad[:20]:
            print(f"  src=0x{src:x} tgt=0x{tgt:x} :: {ln}", file=sys.stderr)
        if len(bad) > 20:
            print(f"  ... and {len(bad) - 20} more", file=sys.stderr)
    sys.exit(1)
PY
  then
    exit 1
  fi
elif [[ -n "$VMLINUX" ]]; then
  echo "note: skipping whole-vmlinux call-target audit (set LINX_AUDIT_VMLINUX=1 to enable)" >&2
fi

echo "PASS: Linx Linux call/ret cross-stack audit passed"
