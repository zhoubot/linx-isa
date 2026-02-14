#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

LLVM_PROJECT="${LLVM_PROJECT:-$HOME/llvm-project}"
DEST_DIR="$LLVM_PROJECT/llvm/lib/Target/LinxISA/MCTargetDesc"

SRC_C="$REPO_ROOT/spec/isa/generated/codecs/linxisa_opcodes.c"
SRC_H="$REPO_ROOT/spec/isa/generated/codecs/linxisa_opcodes.h"

if [[ ! -d "$DEST_DIR" ]]; then
  echo "error: LLVM LinxISA backend dir not found: $DEST_DIR" >&2
  echo "hint: set LLVM_PROJECT=/path/to/llvm-project" >&2
  exit 1
fi
if [[ ! -f "$SRC_C" || ! -f "$SRC_H" ]]; then
  echo "error: generated opcode tables missing under spec/isa/generated/codecs" >&2
  echo "hint: run: python3 tools/isa/gen_c_codec.py --spec spec/isa/spec/current/linxisa-v0.3.json --out-dir spec/isa/generated/codecs" >&2
  exit 1
fi

mkdir -p "$DEST_DIR"
cp -f "$SRC_C" "$DEST_DIR/linxisa_opcodes.c"
cp -f "$SRC_H" "$DEST_DIR/linxisa_opcodes.h"

echo "ok: synced to $DEST_DIR"
