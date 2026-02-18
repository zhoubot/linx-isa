#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$ROOT/../../../../" && pwd)"

TARGET="${TARGET:-linx64-unknown-linux-musl}"
MODE="${MODE:-phase-b}"
OUT_DIR="${OUT_DIR:-$ROOT/out-cpp}"
SYSROOT="${SYSROOT:-$REPO_ROOT/out/libc/musl/install/$MODE}"
CPP_OVERLAY="${CPP_OVERLAY:-$REPO_ROOT/out/cpp-runtime/musl-cxx17-noeh/install}"
LINK_MODE="${LINK_MODE:-both}" # static|shared|both

CLANGXX="${CLANGXX:-}"
if [[ -z "$CLANGXX" ]]; then
  for cand in \
    "$REPO_ROOT/compiler/llvm/build-linxisa-clang/bin/clang++" \
    "$HOME/llvm-project/build-linxisa-clang/bin/clang++"
  do
    if [[ -x "$cand" ]]; then
      CLANGXX="$cand"
      break
    fi
  done
fi
if [[ -z "$CLANGXX" || ! -x "$CLANGXX" ]]; then
  echo "error: clang++ not found; set CLANGXX=/path/to/clang++" >&2
  exit 2
fi

case "$LINK_MODE" in
  static|shared|both) ;;
  *)
    echo "error: LINK_MODE must be static|shared|both (got '$LINK_MODE')" >&2
    exit 2
    ;;
esac

if [[ ! -d "$SYSROOT/lib" ]]; then
  echo "error: sysroot not found: $SYSROOT" >&2
  exit 2
fi

vector_header=""
for cand in \
  "$SYSROOT/include/c++/v1/vector" \
  "$SYSROOT/usr/include/c++/v1/vector" \
  "$CPP_OVERLAY/include/c++/v1/vector"
do
  if [[ -f "$cand" ]]; then
    vector_header="$cand"
    break
  fi
done

if [[ -z "$vector_header" ]]; then
  echo "error: C++ headers not found under sysroot/overlay." >&2
  echo "hint: run 'bash tools/build_linx_llvm_cpp_runtimes.sh --mode $MODE' first." >&2
  exit 2
fi

SRC_DIR="$ROOT/cpp"
if [[ ! -d "$SRC_DIR" ]]; then
  echo "error: missing source dir: $SRC_DIR" >&2
  exit 2
fi

mkdir -p "$OUT_DIR/obj" "$OUT_DIR/bin"

COMMON_FLAGS=(
  -target "$TARGET"
  --sysroot "$SYSROOT"
  -std=c++17
  -O2
  -fno-exceptions
  -fno-rtti
  -fuse-ld=lld
  -nostdinc++
  -stdlib=libc++
  -rtlib=compiler-rt
  -fno-stack-protector
)

INCLUDE_FLAGS=()
if [[ -d "$CPP_OVERLAY/include/c++/v1" ]]; then
  INCLUDE_FLAGS+=("-isystem" "$CPP_OVERLAY/include/c++/v1")
elif [[ -d "$SYSROOT/usr/include/c++/v1" ]]; then
  INCLUDE_FLAGS+=("-isystem" "$SYSROOT/usr/include/c++/v1")
fi

LIB_FLAGS=("-L$SYSROOT/lib" "-L$SYSROOT/usr/lib")
if [[ -d "$CPP_OVERLAY/lib" ]]; then
  LIB_FLAGS+=("-L$CPP_OVERLAY/lib")
fi

find_first_lib() {
  local name="$1"
  shift
  local d
  for d in "$@"; do
    if [[ -f "$d/$name" ]]; then
      echo "$d/$name"
      return 0
    fi
  done
  return 1
}

LIB_SEARCH_DIRS=(
  "$SYSROOT/lib"
  "$SYSROOT/usr/lib"
  "$CPP_OVERLAY/lib"
  "$CPP_OVERLAY/usr/lib"
)

LIBCXX_A="$(find_first_lib libc++.a "${LIB_SEARCH_DIRS[@]}")" || {
  echo "error: missing libc++.a in sysroot/overlay" >&2
  exit 2
}
LIBCXXABI_A="$(find_first_lib libc++abi.a "${LIB_SEARCH_DIRS[@]}")" || {
  echo "error: missing libc++abi.a in sysroot/overlay" >&2
  exit 2
}
HAS_LIBUNWIND=0
for cand in \
  "$SYSROOT/lib/libunwind.a" \
  "$SYSROOT/usr/lib/libunwind.a" \
  "$CPP_OVERLAY/lib/libunwind.a" \
  "$CPP_OVERLAY/usr/lib/libunwind.a"
do
  if [[ -f "$cand" ]]; then
    HAS_LIBUNWIND=1
    break
  fi
done
if [[ "$HAS_LIBUNWIND" == "1" ]]; then
  COMMON_FLAGS+=(-unwindlib=libunwind)
  UNWIND_NAME="libunwind"
else
  COMMON_FLAGS+=(-unwindlib=none)
  UNWIND_NAME="none"
fi

LIBUNWIND_A=""
if [[ "$HAS_LIBUNWIND" == "1" ]]; then
  LIBUNWIND_A="$(find_first_lib libunwind.a "${LIB_SEARCH_DIRS[@]}")" || {
    echo "error: unwindlib selected but libunwind.a is missing" >&2
    exit 2
  }
fi

CPP_LINK_LIBS=("$LIBCXX_A" "$LIBCXXABI_A")
if [[ -n "$LIBUNWIND_A" ]]; then
  CPP_LINK_LIBS+=("$LIBUNWIND_A")
fi

RUNTIME_LIB=""
for cand in \
  "$SYSROOT/lib/liblinx_builtin_rt.a" \
  "$SYSROOT/usr/lib/liblinx_builtin_rt.a" \
  "$REPO_ROOT/out/libc/musl/runtime/$MODE/liblinx_builtin_rt.a"
do
  if [[ -f "$cand" ]]; then
    RUNTIME_LIB="$cand"
    break
  fi
done
if [[ -z "$RUNTIME_LIB" ]]; then
  echo "error: missing liblinx_builtin_rt.a runtime archive" >&2
  exit 2
fi

echo "info: clangxx=$CLANGXX"
echo "info: target=$TARGET sysroot=$SYSROOT link_mode=$LINK_MODE"
echo "info: unwindlib=$UNWIND_NAME"

for src in "$SRC_DIR"/*.cpp; do
  base="$(basename "$src" .cpp)"
  obj="$OUT_DIR/obj/$base.o"
  case_id="${base%%_*}"
  gate_fn="cpp_gate_case${case_id}"
  harness_src="$OUT_DIR/obj/${base}_main.cpp"
  harness_obj="$OUT_DIR/obj/${base}_main.o"

  cat >"$harness_src" <<EOF
extern "C" int ${gate_fn}(void);
int main() { return ${gate_fn}(); }
EOF

  echo "[cc] $base"
  "$CLANGXX" "${COMMON_FLAGS[@]}" "${INCLUDE_FLAGS[@]}" -c "$src" -o "$obj"
  "$CLANGXX" "${COMMON_FLAGS[@]}" -c "$harness_src" -o "$harness_obj"

  if [[ "$LINK_MODE" == "static" || "$LINK_MODE" == "both" ]]; then
    out="$OUT_DIR/bin/${base}.static"
    echo "[ld:static] $base"
    STATIC_LINK_CMD=(
      "$CLANGXX"
      -target "$TARGET"
      --sysroot "$SYSROOT"
      -std=c++17
      -fno-exceptions
      -fno-rtti
      -fuse-ld=lld
      "-unwindlib=$UNWIND_NAME"
      -no-pie
      -nostdlib
      "$SYSROOT/lib/crt1.o"
      "$SYSROOT/lib/crti.o"
      "$obj"
      "$harness_obj"
      "$RUNTIME_LIB"
      "${CPP_LINK_LIBS[@]}"
      "$SYSROOT/lib/libc.a"
      "$SYSROOT/lib/crtn.o"
      -o "$out"
    )
    "${STATIC_LINK_CMD[@]}"
  fi

  if [[ "$LINK_MODE" == "shared" || "$LINK_MODE" == "both" ]]; then
    out="$OUT_DIR/bin/${base}.shared"
    echo "[ld:shared] $base"
    SHARED_LINK_CMD=(
      "$CLANGXX"
      -target "$TARGET"
      --sysroot "$SYSROOT"
      -std=c++17
      -fno-exceptions
      -fno-rtti
      -fuse-ld=lld
      "-unwindlib=$UNWIND_NAME"
      -no-pie
      -nostdlib
      "$SYSROOT/lib/crt1.o"
      "$SYSROOT/lib/crti.o"
      "$obj"
      "$harness_obj"
      "$RUNTIME_LIB"
      "${CPP_LINK_LIBS[@]}"
      "-L$SYSROOT/lib"
      "-L$SYSROOT/usr/lib"
      -lc
      "$SYSROOT/lib/crtn.o"
      -Wl,--dynamic-linker=/lib/ld-musl-linx64.so.1
      -o "$out"
    )
    "${SHARED_LINK_CMD[@]}"
  fi
done

echo "ok: C++ compile/link outputs in $OUT_DIR"
