#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
QEMU_DIR="${QEMU_DIR:-$HOME/qemu}"

PATCH_DIR="$REPO_ROOT/impl/emulator/qemu/patches"

if [[ ! -d "$QEMU_DIR/.git" ]]; then
  echo "error: QEMU repo not found: $QEMU_DIR" >&2
  echo "hint: set QEMU_DIR=/path/to/qemu checkout" >&2
  exit 1
fi
if [[ ! -d "$PATCH_DIR" ]]; then
  echo "error: patch dir not found: $PATCH_DIR" >&2
  exit 1
fi

cd "$QEMU_DIR"
for p in "$PATCH_DIR"/*.patch; do
  echo "+ git apply $p"
  git apply "$p"
done

echo "ok: applied patches to $QEMU_DIR"

