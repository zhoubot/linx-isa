#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

MODE="phase-b"
TARGET="linx64-unknown-linux-musl"
LLVM_ROOT="${LLVM_ROOT:-$ROOT/compiler/llvm}"
OUT_ROOT="${OUT_ROOT:-$ROOT/out/cpp-runtime/musl-cxx17-noeh}"
MUSL_SYSROOT=""
CACHE_FILE=""
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 8)}"
MERGE_SYSROOT=1
ENABLE_LIBUNWIND=0

usage() {
  cat <<'EOF'
Usage: build_linx_llvm_cpp_runtimes.sh [options]

Options:
  --mode <phase-a|phase-b>     Musl lane mode used for sysroot selection (default: phase-b)
  --target <triple>            Runtime target triple (default: linx64-unknown-linux-musl)
  --llvm-root <path>           LLVM monorepo root (default: compiler/llvm in superproject)
  --out-root <path>            Runtime build/install root (default: out/cpp-runtime/musl-cxx17-noeh)
  --musl-sysroot <path>        Musl sysroot to merge runtime overlay into
  --cache-file <path>          Runtime CMake cache preset
  --jobs <N>                   Parallel build jobs
  --enable-libunwind           Also build/install libunwind (requires Linx libunwind arch support)
  --no-merge-sysroot           Build/install runtime overlay but do not copy into musl sysroot
  -h, --help                   Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      MODE="$2"
      shift 2
      ;;
    --target)
      TARGET="$2"
      shift 2
      ;;
    --llvm-root)
      LLVM_ROOT="$2"
      shift 2
      ;;
    --out-root)
      OUT_ROOT="$2"
      shift 2
      ;;
    --musl-sysroot)
      MUSL_SYSROOT="$2"
      shift 2
      ;;
    --cache-file)
      CACHE_FILE="$2"
      shift 2
      ;;
    --jobs)
      JOBS="$2"
      shift 2
      ;;
    --enable-libunwind)
      ENABLE_LIBUNWIND=1
      shift
      ;;
    --no-merge-sysroot)
      MERGE_SYSROOT=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "error: unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

case "$MODE" in
  phase-a|phase-b) ;;
  *)
    echo "error: --mode must be phase-a or phase-b (got '$MODE')" >&2
    exit 2
    ;;
esac

LLVM_ROOT="$(cd "$LLVM_ROOT" && pwd -P)"
OUT_ROOT="$(mkdir -p "$OUT_ROOT" && cd "$OUT_ROOT" && pwd -P)"

if [[ -z "$MUSL_SYSROOT" ]]; then
  MUSL_SYSROOT="$ROOT/out/libc/musl/install/$MODE"
fi
MUSL_SYSROOT="$(cd "$MUSL_SYSROOT" && pwd -P)"

if [[ -z "$CACHE_FILE" ]]; then
  CACHE_FILE="$LLVM_ROOT/runtimes/cmake/caches/LinxISA-musl-cxx17-noeh.cmake"
fi

if [[ ! -d "$LLVM_ROOT/llvm" ]]; then
  echo "error: invalid --llvm-root, missing '$LLVM_ROOT/llvm'" >&2
  exit 2
fi
if [[ ! -f "$CACHE_FILE" ]]; then
  echo "error: runtime cache file not found: $CACHE_FILE" >&2
  exit 2
fi
if [[ ! -d "$MUSL_SYSROOT/lib" ]]; then
  echo "error: musl sysroot missing lib dir: $MUSL_SYSROOT/lib" >&2
  echo "hint: run lib/musl/tools/linx/build_linx64_musl.sh first." >&2
  exit 2
fi
if [[ ! -d "$MUSL_SYSROOT/include" && ! -d "$MUSL_SYSROOT/usr/include" ]]; then
  echo "error: musl sysroot not ready: $MUSL_SYSROOT" >&2
  echo "hint: run lib/musl/tools/linx/build_linx64_musl.sh first." >&2
  exit 2
fi

CLANG="${CLANG:-}"
if [[ -z "$CLANG" ]]; then
  for cand in \
    "$ROOT/compiler/llvm/build-linxisa-clang/bin/clang" \
    "$HOME/llvm-project/build-linxisa-clang/bin/clang"
  do
    if [[ -x "$cand" ]]; then
      CLANG="$cand"
      break
    fi
  done
fi
if [[ -z "$CLANG" || ! -x "$CLANG" ]]; then
  echo "error: CLANG not found; set CLANG=/path/to/clang" >&2
  exit 2
fi

LLVM_BIN="$(cd "$(dirname "$CLANG")" && pwd -P)"
LLVM_HOST_BUILD_ROOT="${LLVM_HOST_BUILD_ROOT:-$(cd "$LLVM_BIN/.." && pwd -P)}"
CLANGXX="${CLANGXX:-$LLVM_BIN/clang++}"
LLD="${LLD:-$LLVM_BIN/ld.lld}"
AR="${AR:-$LLVM_BIN/llvm-ar}"
RANLIB="${RANLIB:-$LLVM_BIN/llvm-ranlib}"
NM="${NM:-$LLVM_BIN/llvm-nm}"
STRIP="${STRIP:-$LLVM_BIN/llvm-strip}"

for exe in "$CLANGXX" "$LLD" "$AR" "$RANLIB" "$NM" "$STRIP"; do
  if [[ ! -x "$exe" ]]; then
    echo "error: missing executable tool: $exe" >&2
    exit 2
  fi
done

LLVM_CONFIG_DIR="$LLVM_HOST_BUILD_ROOT/lib/cmake/llvm"
CLANG_CONFIG_DIR="$LLVM_HOST_BUILD_ROOT/lib/cmake/clang"
if [[ ! -f "$LLVM_CONFIG_DIR/LLVMConfig.cmake" ]]; then
  echo "error: missing LLVMConfig.cmake in host build: $LLVM_CONFIG_DIR/LLVMConfig.cmake" >&2
  exit 2
fi
if [[ ! -f "$CLANG_CONFIG_DIR/ClangConfig.cmake" ]]; then
  echo "error: missing ClangConfig.cmake in host build: $CLANG_CONFIG_DIR/ClangConfig.cmake" >&2
  exit 2
fi

BUILD_DIR="$OUT_ROOT/build/$MODE"
INSTALL_DIR="$OUT_ROOT/install"
LOG_DIR="$OUT_ROOT/logs"
SUMMARY="$OUT_ROOT/summary_${MODE}.json"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR" "$INSTALL_DIR" "$LOG_DIR"

configure_log="$LOG_DIR/configure_${MODE}.log"
build_log="$LOG_DIR/build_${MODE}.log"
install_log="$LOG_DIR/install_${MODE}.log"

RUNTIME_LIST="libcxxabi;libcxx"
LIBCXXABI_USE_LLVM_UNWINDER=OFF
if [[ "$ENABLE_LIBUNWIND" == "1" ]]; then
  RUNTIME_LIST="libunwind;libcxxabi;libcxx"
  LIBCXXABI_USE_LLVM_UNWINDER=ON
fi

CMAKE_COMMON=(
  -G Ninja
  -C "$CACHE_FILE"
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
  -DLLVM_DIR="$LLVM_CONFIG_DIR"
  -DClang_DIR="$CLANG_CONFIG_DIR"
  -DLLVM_BINARY_DIR="$LLVM_HOST_BUILD_ROOT"
  -DLLVM_PATH="$LLVM_ROOT/llvm"
  -DLLVM_ENABLE_RUNTIMES="$RUNTIME_LIST"
  -DLLVM_INCLUDE_TESTS=OFF
  -DLLVM_INCLUDE_DOCS=OFF
  -DLLVM_INCLUDE_BENCHMARKS=OFF
  -DHAVE_LIBRT=FALSE
  -DCMAKE_SYSTEM_NAME=Linux
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
  -DCMAKE_C_COMPILER="$CLANG"
  -DCMAKE_CXX_COMPILER="$CLANGXX"
  -DCMAKE_ASM_COMPILER="$CLANG"
  -DCMAKE_C_COMPILER_TARGET="$TARGET"
  -DCMAKE_CXX_COMPILER_TARGET="$TARGET"
  -DCMAKE_ASM_COMPILER_TARGET="$TARGET"
  -DCMAKE_AR="$AR"
  -DCMAKE_RANLIB="$RANLIB"
  -DCMAKE_NM="$NM"
  -DCMAKE_STRIP="$STRIP"
  -DCMAKE_SYSROOT="$MUSL_SYSROOT"
  "-DCMAKE_C_FLAGS=--sysroot=$MUSL_SYSROOT -fuse-ld=lld"
  "-DCMAKE_CXX_FLAGS=--sysroot=$MUSL_SYSROOT -fuse-ld=lld -std=c++17 -fno-exceptions"
  "-DCMAKE_EXE_LINKER_FLAGS=--sysroot=$MUSL_SYSROOT -fuse-ld=lld"
  "-DCMAKE_SHARED_LINKER_FLAGS=--sysroot=$MUSL_SYSROOT -fuse-ld=lld"
  "-DCMAKE_MODULE_LINKER_FLAGS=--sysroot=$MUSL_SYSROOT -fuse-ld=lld"
  -DLIBCXX_ENABLE_EXCEPTIONS=OFF
  -DLIBCXX_ENABLE_RTTI=ON
  -DLIBCXX_HAS_MUSL_LIBC=ON
  -DLIBCXX_ENABLE_LOCALIZATION=OFF
  -DLIBCXX_ENABLE_WIDE_CHARACTERS=OFF
  -DLIBCXX_ENABLE_UNICODE=OFF
  -DLIBCXX_ENABLE_FILESYSTEM=OFF
  -DLIBCXX_ENABLE_THREADS=OFF
  -DLIBCXX_ENABLE_MONOTONIC_CLOCK=OFF
  -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF
  -DLIBCXXABI_ENABLE_THREADS=OFF
  "-DLIBCXXABI_USE_LLVM_UNWINDER=$LIBCXXABI_USE_LLVM_UNWINDER"
)

echo "[1/4] configure runtimes"
{
  printf '+ cmake -S %q -B %q' "$LLVM_ROOT/runtimes" "$BUILD_DIR"
  printf ' %q' "${CMAKE_COMMON[@]}"
  echo
} >"$configure_log"
cmake -S "$LLVM_ROOT/runtimes" -B "$BUILD_DIR" "${CMAKE_COMMON[@]}" >>"$configure_log" 2>&1

echo "[2/4] build runtimes (target=$TARGET)"
{
  echo "+ cmake --build $BUILD_DIR --parallel $JOBS"
} >"$build_log"
cmake --build "$BUILD_DIR" --parallel "$JOBS" >>"$build_log" 2>&1

echo "[3/4] install runtimes"
{
  echo "+ cmake --build $BUILD_DIR --target install --parallel $JOBS"
} >"$install_log"
cmake --build "$BUILD_DIR" --target install --parallel "$JOBS" >>"$install_log" 2>&1

lib_candidates=(
  "libc++.a"
  "libc++abi.a"
)
if [[ "$ENABLE_LIBUNWIND" == "1" ]]; then
  lib_candidates+=("libunwind.a")
fi

declare -a copied_libs=()
cpp_include_dir=""
if [[ "$MERGE_SYSROOT" == "1" ]]; then
  echo "[4/4] merge runtime overlay into musl sysroot"
  while IFS= read -r path; do
    cpp_include_dir="$path"
    break
  done < <(find "$INSTALL_DIR" -type d -path "*/include/c++/v1" | sort)

  if [[ -n "$cpp_include_dir" ]]; then
    mkdir -p "$MUSL_SYSROOT/include/c++"
    rm -rf "$MUSL_SYSROOT/include/c++/v1"
    cp -R "$cpp_include_dir" "$MUSL_SYSROOT/include/c++/v1"
  fi

  for lib in "${lib_candidates[@]}"; do
    src=""
    while IFS= read -r cand; do
      src="$cand"
      break
    done < <(find "$INSTALL_DIR" -type f -name "$lib" | sort)
    if [[ -z "$src" ]]; then
      echo "error: missing runtime library after install: $lib" >&2
      exit 2
    fi
    install -m 644 "$src" "$MUSL_SYSROOT/lib/$lib"
    install -m 644 "$src" "$MUSL_SYSROOT/usr/lib/$lib"
    copied_libs+=("$MUSL_SYSROOT/lib/$lib")
  done

  # Reuse musl bring-up builtins as the compiler-rt builtins compatibility
  # archive expected by clang++ when -rtlib=compiler-rt is selected.
  target_arch="${TARGET%%-*}"
  builtins_name="libclang_rt.builtins-${target_arch}.a"
  builtins_src="$ROOT/out/libc/musl/runtime/$MODE/liblinx_builtin_rt.a"
  if [[ -f "$builtins_src" ]]; then
    install -m 644 "$builtins_src" "$MUSL_SYSROOT/lib/$builtins_name"
    install -m 644 "$builtins_src" "$MUSL_SYSROOT/usr/lib/$builtins_name"
    copied_libs+=("$MUSL_SYSROOT/lib/$builtins_name")
  fi
fi

python3 - <<PY
import json
from pathlib import Path

summary = {
    "schema_version": "linx-cpp-runtimes-v1",
    "mode": "${MODE}",
    "target": "${TARGET}",
    "runtime_list": "${RUNTIME_LIST}".split(";"),
    "libunwind_enabled": ${ENABLE_LIBUNWIND},
    "paths": {
        "llvm_root": "${LLVM_ROOT}",
        "llvm_host_build_root": "${LLVM_HOST_BUILD_ROOT}",
        "musl_sysroot": "${MUSL_SYSROOT}",
        "out_root": "${OUT_ROOT}",
        "build_dir": "${BUILD_DIR}",
        "install_dir": "${INSTALL_DIR}",
        "cache_file": "${CACHE_FILE}",
        "clang": "${CLANG}",
        "clangxx": "${CLANGXX}",
        "lld": "${LLD}",
    },
    "logs": {
        "configure": "${configure_log}",
        "build": "${build_log}",
        "install": "${install_log}",
    },
    "merge_sysroot": ${MERGE_SYSROOT},
    "copied_runtime_libs": [x for x in """${copied_libs[*]}""".split() if x],
}

summary_path = Path("${SUMMARY}")
summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\\n", encoding="utf-8")
print(f"ok: wrote {summary_path}")
PY

echo "ok: Linx C++ runtimes ready (mode=$MODE target=$TARGET)"
