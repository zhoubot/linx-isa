#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
QEMU_SRC="${QEMU_SRC:-$HOME/qemu}"

OUT_DIR="${OUT_DIR:-$REPO_ROOT/workloads/generated/plugins}"
OUT_SO="$OUT_DIR/liblinx_insn_hist.so"

if [[ ! -d "$QEMU_SRC/include/qemu" ]]; then
  echo "error: QEMU source tree not found at $QEMU_SRC" >&2
  echo "hint: set QEMU_SRC=/path/to/qemu checkout" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"

GLIB_CFLAGS="$(pkg-config --cflags glib-2.0)"
GLIB_LIBS="$(pkg-config --libs glib-2.0)"

EXTRA_LDFLAGS=()
if [[ "$(uname -s)" == "Darwin" ]]; then
  # Allow unresolved qemu_plugin_* symbols; they resolve when QEMU loads the plugin.
  EXTRA_LDFLAGS+=("-Wl,-undefined,dynamic_lookup")
fi

cc -O2 -fPIC -shared \
  $GLIB_CFLAGS \
  -I"$QEMU_SRC/include/qemu" \
  -I"$QEMU_SRC/include" \
  -I"$REPO_ROOT/spec/isa/generated/codecs" \
  -o "$OUT_SO" \
  "$REPO_ROOT/tools/qemu_plugins/linx_insn_hist.c" \
  "$REPO_ROOT/spec/isa/generated/codecs/linxisa_opcodes.c" \
  $GLIB_LIBS \
  "${EXTRA_LDFLAGS[@]}"

echo "ok: built $OUT_SO"
