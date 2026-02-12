#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="$ROOT/workloads/benchmarks/third_party"
SOURCES_MD="$OUT_DIR/SOURCES.md"

mkdir -p "$OUT_DIR"

sha256_file() {
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{print $1}'
    return 0
  fi
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
    return 0
  fi
  echo "unknown"
}

fetch_github_tarball() {
  local owner="$1"
  local repo="$2"
  local ref="$3"
  local dest="$4"

  if is_populated_dir "$dest"; then
    echo "== skip ${owner}/${repo}@${ref} (already present: $dest)"
    {
      echo "- ${owner}/${repo}"
      echo "  - ref: ${ref}"
      echo "  - note: already present; not re-downloaded"
      echo "  - extracted_to: workloads/benchmarks/third_party/$(basename "$dest")"
    } >>"$SOURCES_MD"
    return 0
  fi

  local url="https://codeload.github.com/${owner}/${repo}/tar.gz/${ref}"
  local tmp
  tmp="$(mktemp -t "${repo}.XXXXXX.tar.gz")"

  echo "== fetch ${owner}/${repo}@${ref}"
  echo "   url: $url"

  # Retry a few times; GitHub connectivity can be flaky in some environments.
  if ! curl -fL --retry 3 --retry-delay 2 --connect-timeout 20 --max-time 600 -o "$tmp" "$url"; then
    rm -f "$tmp"
    return 1
  fi

  local sum
  sum="$(sha256_file "$tmp")"

  rm -rf "$dest"
  mkdir -p "$dest"
  if ! tar -xzf "$tmp" -C "$dest" --strip-components=1; then
    rm -rf "$dest"
    rm -f "$tmp"
    return 1
  fi
  rm -f "$tmp"

  {
    echo "- ${owner}/${repo}"
    echo "  - ref: ${ref}"
    echo "  - url: ${url}"
    echo "  - sha256(tarball): ${sum}"
    echo "  - extracted_to: workloads/benchmarks/third_party/$(basename "$dest")"
  } >>"$SOURCES_MD"
}

fetch_url_tarball() {
  local name="$1"
  local url="$2"
  local dest="$3"

  if is_populated_dir "$dest"; then
    echo "== skip ${name} (already present: $dest)"
    {
      echo "- ${name}"
      echo "  - url: ${url}"
      echo "  - note: already present; not re-downloaded"
      echo "  - extracted_to: workloads/benchmarks/third_party/$(basename "$dest")"
    } >>"$SOURCES_MD"
    return 0
  fi

  local tmp
  tmp="$(mktemp -t "${name}.XXXXXX.tar.gz")"

  echo "== fetch ${name}"
  echo "   url: $url"

  if ! curl -fL --retry 3 --retry-delay 2 --connect-timeout 20 --max-time 600 -o "$tmp" "$url"; then
    rm -f "$tmp"
    return 1
  fi

  local sum
  sum="$(sha256_file "$tmp")"

  rm -rf "$dest"
  mkdir -p "$dest"
  if ! tar -xzf "$tmp" -C "$dest" --strip-components=1; then
    if ! tar -xzf "$tmp" -C "$dest"; then
      rm -rf "$dest"
      rm -f "$tmp"
      return 1
    fi
  fi
  rm -f "$tmp"

  {
    echo "- ${name}"
    echo "  - url: ${url}"
    echo "  - sha256(tarball): ${sum}"
    echo "  - extracted_to: workloads/benchmarks/third_party/$(basename "$dest")"
  } >>"$SOURCES_MD"
}

init_sources_md() {
  cat >"$SOURCES_MD" <<'EOF'
# Third-party Benchmark Sources

This folder is populated by `workloads/benchmarks/fetch_third_party.sh`.

This repo already vendors CoreMark and Dhrystone under `workloads/benchmarks/`.

Notes:

- SPEC CPU is not downloaded here (paid/licensed). Track it separately.
EOF
}

init_sources_md

# Skip downloading if a destination directory already exists and is non-empty.
is_populated_dir() {
  local d="$1"
  [[ -d "$d" ]] && [[ -n "$(find "$d" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]
}

# Use env overrides to pin exact refs.
EMBENCH_REF="${EMBENCH_REF:-main}"
POLYBENCH_REF="${POLYBENCH_REF:-main}"
LLVM_TEST_SUITE_REF="${LLVM_TEST_SUITE_REF:-main}"
GBENCH_REF="${GBENCH_REF:-main}"
TSVC_REF="${TSVC_REF:-master}"

# Try main then master where needed.
try_fetch_github() {
  local owner="$1"
  local repo="$2"
  local preferred_ref="$3"
  local dest="$4"
  if fetch_github_tarball "$owner" "$repo" "$preferred_ref" "$dest"; then
    return 0
  fi
  if [[ "$preferred_ref" != "master" ]]; then
    fetch_github_tarball "$owner" "$repo" "master" "$dest"
    return 0
  fi
  return 1
}

try_fetch_github embench embench-iot "$EMBENCH_REF" "$OUT_DIR/embench-iot"
try_fetch_github ferrandi PolyBenchC "$POLYBENCH_REF" "$OUT_DIR/PolyBenchC"
try_fetch_github llvm llvm-test-suite "$LLVM_TEST_SUITE_REF" "$OUT_DIR/llvm-test-suite"
try_fetch_github google benchmark "$GBENCH_REF" "$OUT_DIR/google-benchmark"
try_fetch_github UoB-HPC TSVC_2 "$TSVC_REF" "$OUT_DIR/TSVC_2"

# MiBench: prefer the maintained mirror; do not rely on the legacy university host.
MIBENCH_REF="${MIBENCH_REF:-master}"
try_fetch_github embecosm mibench "$MIBENCH_REF" "$OUT_DIR/mibench"

echo
echo "ok: fetched third-party suites into: $OUT_DIR"
echo "ok: recorded sources in: $SOURCES_MD"
