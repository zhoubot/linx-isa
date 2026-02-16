#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPSTREAM_DIR="$ROOT_DIR/upstream/TSVC_2"
REPO_URL="https://github.com/UoB-HPC/TSVC_2.git"
LOCAL_MIRROR="$ROOT_DIR/../third_party/TSVC_2"
PINNED_COMMIT="${TSVC_COMMIT:-badf9adb2974867ac0937718d85a44dec6dec95a}"

mkdir -p "$ROOT_DIR/upstream"

if [[ ! -d "$UPSTREAM_DIR/.git" ]]; then
  if ! git clone "$REPO_URL" "$UPSTREAM_DIR"; then
    if [[ -d "$LOCAL_MIRROR/.git" ]]; then
      echo "warn: upstream clone failed; using local mirror: $LOCAL_MIRROR" >&2
      git clone "$LOCAL_MIRROR" "$UPSTREAM_DIR"
    else
      exit 1
    fi
  fi
fi

if ! git -C "$UPSTREAM_DIR" rev-parse --verify --quiet "${PINNED_COMMIT}^{commit}" >/dev/null; then
  if ! git -C "$UPSTREAM_DIR" fetch --tags origin; then
    if [[ -d "$LOCAL_MIRROR/.git" ]]; then
      echo "warn: upstream fetch failed; fetching from local mirror: $LOCAL_MIRROR" >&2
      git -C "$UPSTREAM_DIR" fetch --tags "$LOCAL_MIRROR"
    else
      exit 1
    fi
  fi
  if ! git -C "$UPSTREAM_DIR" rev-parse --verify --quiet "${PINNED_COMMIT}^{commit}" >/dev/null; then
    if [[ -d "$LOCAL_MIRROR/.git" ]]; then
      git -C "$UPSTREAM_DIR" fetch --tags "$LOCAL_MIRROR"
    fi
  fi
fi

if ! git -C "$UPSTREAM_DIR" rev-parse --verify --quiet "${PINNED_COMMIT}^{commit}" >/dev/null; then
  echo "error: pinned commit not found: $PINNED_COMMIT" >&2
  if [[ -d "$LOCAL_MIRROR/.git" ]]; then
    echo "hint: local mirror checked: $LOCAL_MIRROR" >&2
  else
    echo "hint: run workloads/fetch_third_party.sh to populate mirror" >&2
  fi
  exit 1
fi

if ! git -C "$UPSTREAM_DIR" checkout --detach "$PINNED_COMMIT"; then
  if [[ -d "$LOCAL_MIRROR/.git" ]]; then
    git -C "$UPSTREAM_DIR" fetch --tags "$LOCAL_MIRROR"
    git -C "$UPSTREAM_DIR" checkout --detach "$PINNED_COMMIT"
  else
    exit 1
  fi
fi

RESOLVED_COMMIT="$(git -C "$UPSTREAM_DIR" rev-parse HEAD)"
if [[ "$RESOLVED_COMMIT" != "$PINNED_COMMIT" ]]; then
  echo "error: expected commit $PINNED_COMMIT, got $RESOLVED_COMMIT" >&2
  exit 1
fi

cat > "$ROOT_DIR/SOURCES.md" <<EOF
# TSVC Sources

- Upstream repository: \`$REPO_URL\`
- Pinned commit: \`$PINNED_COMMIT\`
- Resolved commit: \`$RESOLVED_COMMIT\`
- Local checkout target: \`workloads/tsvc/upstream/TSVC_2\`
EOF

echo "ok: TSVC pinned checkout ready at $UPSTREAM_DIR"
