#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_LOG="${OUT_LOG:-$SCRIPT_DIR/out/system_strict.log}"
mkdir -p "$(dirname "$OUT_LOG")"

CMD=(
  python3 "$SCRIPT_DIR/run_tests.py"
  --suite system
  --timeout "${TIMEOUT:-15}"
  --verbose
  --require-test-id 0x1100
  --require-test-id 0x1101
  --require-test-id 0x1102
  --require-test-id 0x1103
  --require-test-id 0x1104
  --require-test-id 0x1105
  --require-test-id 0x1106
  --require-test-id 0x1107
  --require-test-id 0x1108
  --require-test-id 0x1109
  --require-test-id 0x110A
  --require-test-id 0x110B
  --require-test-id 0x110C
  --require-test-id 0x110D
  --require-test-id 0x110E
)

"${CMD[@]}" >"$OUT_LOG" 2>&1
cat "$OUT_LOG"

if grep -q "LINX_INSN_COUNT=" "$OUT_LOG"; then
  echo "error: unexpected LINX_INSN_COUNT debug output in strict system run" >&2
  exit 1
fi

if grep -q "Linx: TRACE" "$OUT_LOG"; then
  echo "error: unexpected Linx trace debug output in strict system run" >&2
  exit 1
fi
