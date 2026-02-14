#!/usr/bin/env bash
set -euo pipefail

# Bring-up helper for building glibc for LinxISA (linx64) with the local LLVM
# toolchain. This script is intentionally incremental: it configures glibc and
# attempts to build early startup objects (csu) first.
#
# Expected local checkouts:
#   - ~/llvm-project (build dir: build-linxisa-clang)
#   - ~/glibc
#   - ~/sysroots/linx64-linux-gnu/src/linux-6.1 (Linux source)
#
# Outputs:
#   - ~/glibc/build-linx64-glibc1 (configure)
#   - /tmp/glibc_linx_build_csu*.log (build logs)

LLVM_BUILD="${LLVM_BUILD:-~/llvm-project/build-linxisa-clang}"
GLIBC_SRC="${GLIBC_SRC:-~/glibc}"
SYSROOT="${SYSROOT:-~/sysroots/linx64-linux-gnu}"
LINUX_SRC="${LINUX_SRC:-$SYSROOT/src/linux-6.1}"
LINUX_HDRS="${LINUX_HDRS:-$SYSROOT/linux-headers}"
BUILD_DIR="${BUILD_DIR:-$GLIBC_SRC/build-linx64-glibc1}"

need() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "error: missing required tool: $1" >&2
    exit 2
  }
}

need "$LLVM_BUILD/bin/clang"
need "$LLVM_BUILD/bin/ld.lld"
need /opt/homebrew/bin/gmake
need /opt/homebrew/bin/gsed
need /opt/homebrew/opt/bison/bin/bison

mkdir -p /tmp/linx-glibc-tools
cat > /tmp/linx-glibc-tools/gnumake <<'SH'
#!/bin/sh
exec /opt/homebrew/bin/gmake "$@"
SH
cat > /tmp/linx-glibc-tools/sed <<'SH'
#!/bin/sh
exec /opt/homebrew/bin/gsed "$@"
SH
cat > /tmp/linx-glibc-tools/bison <<'SH'
#!/bin/sh
exec /opt/homebrew/opt/bison/bin/bison "$@"
SH
cat > /tmp/linx-glibc-tools/yacc <<'SH'
#!/bin/sh
exec /opt/homebrew/opt/bison/bin/yacc "$@"
SH
chmod +x /tmp/linx-glibc-tools/gnumake /tmp/linx-glibc-tools/sed \
  /tmp/linx-glibc-tools/bison /tmp/linx-glibc-tools/yacc

export PATH="/tmp/linx-glibc-tools:$PATH"
export MAKEINFO=:

echo "[1/3] Installing Linux UAPI headers (ARCH=riscv stand-in) ..."
rm -rf "$LINUX_HDRS"
mkdir -p "$LINUX_HDRS"
make -C "$LINUX_SRC" ARCH=riscv headers_install INSTALL_HDR_PATH="$LINUX_HDRS" >/tmp/linx_linux_headers_install.log

echo "[2/3] Configuring glibc (linx64) ..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

"$GLIBC_SRC/configure" \
  --host=linx64-unknown-linux-gnu \
  --build=aarch64-apple-darwin \
  --prefix=/usr \
  --with-headers="$LINUX_HDRS/include" \
  --disable-werror \
  --disable-nscd \
  --disable-timezone-tools \
  CC="$LLVM_BUILD/bin/clang -target linx64-unknown-linux-gnu --sysroot=$SYSROOT" \
  CXX="$LLVM_BUILD/bin/clang++ -target linx64-unknown-linux-gnu --sysroot=$SYSROOT" \
  LD="$LLVM_BUILD/bin/ld.lld" \
  AR="$LLVM_BUILD/bin/llvm-ar" \
  RANLIB="$LLVM_BUILD/bin/llvm-ranlib" \
  NM="$LLVM_BUILD/bin/llvm-nm" \
  OBJCOPY="$LLVM_BUILD/bin/llvm-objcopy" \
  OBJDUMP="$LLVM_BUILD/bin/llvm-objdump" \
  READELF=/opt/homebrew/opt/binutils/bin/readelf \
  CFLAGS="-O2 -g" \
  CPPFLAGS="-U_FORTIFY_SOURCE" \
  LDFLAGS="-fuse-ld=lld"

echo "[3/3] Building early startup (csu/subdir_lib) ..."
set +e
gnumake -j8 csu/subdir_lib 2>&1 | tee /tmp/glibc_linx_build_csu.log
rc=${PIPESTATUS[0]}
set -e

if [ "$rc" -ne 0 ]; then
  echo "note: build failed (expected during bring-up). See /tmp/glibc_linx_build_csu.log" >&2
  exit "$rc"
fi

echo "done: $BUILD_DIR"
