#!/bin/bash
# Release build script for x64dbg-linux
# Creates optimized release build with all features enabled

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

BUILD_TYPE="${1:-Release}"
BUILD_DIR="$REPO_ROOT/src/cross/build-release"

echo "=== x64dbg-linux Release Build ==="
echo "Build type: $BUILD_TYPE"
echo "Repository root: $REPO_ROOT"
echo ""

# Clean previous build
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cd "$REPO_ROOT/src/cross"

# Configure with Release optimizations
echo "Configuring CMake..."
cmake -G Ninja \
    -B "$BUILD_DIR" \
    -S "$REPO_ROOT/src/cross" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=x86-64-v2" \
    -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG -march=x86-64-v2" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,--strip-all" \
    -DBUILD_SHARED_LIBS=OFF

# Build with parallel jobs
echo "Building..."
cmake --build "$BUILD_DIR" --target x64dbg-linux -j$(nproc)

# Strip the binary
echo "Stripping binary..."
strip "$BUILD_DIR/x64dbg-linux" || true

# Show build info
echo ""
echo "=== Build Complete ==="
echo "Binary: $BUILD_DIR/x64dbg-linux"
echo "Size: $(du -h $BUILD_DIR/x64dbg-linux | cut -f1)"
echo ""

# Verify dependencies
echo "Dependencies:"
ldd "$BUILD_DIR/x64dbg-linux" | grep -E "Qt|libelf|libdw" || true
