#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

missing_legacy=0
for p in \
  spec/isa/golden/v0.1 \
  spec/isa/golden/v0.2 \
  spec/isa/spec/current/linxisa-v0.1.json \
  spec/isa/spec/current/linxisa-v0.2.json
 do
  if [[ -e "$p" ]]; then
    echo "error: legacy artifact still present: $p" >&2
    missing_legacy=1
  fi
done
if [[ "$missing_legacy" -ne 0 ]]; then
  exit 1
fi

# Runtime/public docs/tools must not depend on removed v0.1/v0.2 catalogs.
if rg -n \
  --glob '!**/.git/**' \
  --glob '!docs/migration/**' \
  --glob '!spec/isa/changelog/**' \
  --glob '!docs/architecture/isa-manual/src/chapters/98_changelog.adoc' \
  --glob '!tools/isa/check_no_legacy_v02.py' \
  --glob '!tools/ci/check_public_v03.sh' \
  --glob '!impl/compiler/llvm/tests/out-*/**' \
  'linxisa-v0\.[12]\.json' \
  README.md docs impl spec tools tests
then
  echo "error: found forbidden runtime/public references to removed v0.1/v0.2 catalogs" >&2
  exit 1
fi

echo "OK: public v0.3 guard passed"
