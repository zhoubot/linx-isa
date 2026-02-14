#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

BASE_REF="${1:-}"
if [[ -z "$BASE_REF" ]]; then
  BASE_REF="origin/main"
fi

if git rev-parse --verify "$BASE_REF" >/dev/null 2>&1; then
  changed="$(git diff --name-only "$BASE_REF"...HEAD || true)"
else
  changed="$(git diff-tree --no-commit-id --name-only -r HEAD || true)"
fi

warned=0
while IFS= read -r p; do
  [[ -z "$p" ]] && continue
  case "$p" in
    isa/*|compiler/*|emulator/*|rtl/*|models/*|toolchain/*|avs/*)
      echo "::warning file=$p::Compatibility shim path touched. Use canonical spec/ or impl/ paths (shim removal scheduled for v0.3.1)."
      warned=1
      ;;
  esac
done <<< "$changed"

if [[ "$warned" -eq 1 ]]; then
  echo "WARN: shim path touches detected"
else
  echo "OK: no shim path touches"
fi
