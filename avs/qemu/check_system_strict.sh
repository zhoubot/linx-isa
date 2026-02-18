#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_LOG="${OUT_LOG:-$SCRIPT_DIR/out/system_strict.log}"
RETRIES="${RETRIES:-3}"
STRICT_TIMER_IRQ_DISABLE="${STRICT_TIMER_IRQ_DISABLE:-0}"
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

if ! [[ "$RETRIES" =~ ^[0-9]+$ ]] || [[ "$RETRIES" -lt 1 ]]; then
  echo "error: RETRIES must be a positive integer (got: $RETRIES)" >&2
  exit 2
fi

LAST_RC=1
LAST_LOG=""
for attempt in $(seq 1 "$RETRIES"); do
  ATTEMPT_LOG="$OUT_LOG.attempt$attempt"
  set +e
  LINX_DISABLE_TIMER_IRQ="$STRICT_TIMER_IRQ_DISABLE" "${CMD[@]}" >"$ATTEMPT_LOG" 2>&1
  LAST_RC=$?
  set -e
  LAST_LOG="$ATTEMPT_LOG"

  if [[ "$LAST_RC" -eq 0 ]]; then
    cat "$ATTEMPT_LOG"

    if grep -q "LINX_INSN_COUNT=" "$ATTEMPT_LOG"; then
      echo "error: unexpected LINX_INSN_COUNT debug output in strict system run" >&2
      exit 1
    fi

    if grep -q "Linx: TRACE" "$ATTEMPT_LOG"; then
      echo "error: unexpected Linx trace debug output in strict system run" >&2
      exit 1
    fi

    mv "$ATTEMPT_LOG" "$OUT_LOG"
    exit 0
  fi

  if [[ "$attempt" -lt "$RETRIES" ]]; then
    echo "warn: strict system attempt $attempt/$RETRIES failed; retrying..." >&2
    tail -n 20 "$ATTEMPT_LOG" >&2 || true
    sleep 1
  fi
done

if [[ -n "$LAST_LOG" && -f "$LAST_LOG" ]]; then
  mv "$LAST_LOG" "$OUT_LOG"
  cat "$OUT_LOG"
fi
exit "$LAST_RC"
