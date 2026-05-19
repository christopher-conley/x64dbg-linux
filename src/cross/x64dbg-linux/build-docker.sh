#!/bin/bash
# Build script for x64dbg-linux using Docker
# This script must be run from the x64dbg-linux directory

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# 获取仓库根目录（从 x64dbg-linux 向上 3 层到 x64dbg 根目录）
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "Repository root: $REPO_ROOT"
echo "Building x64dbg-linux..."

# Check if Docker image already exists
if docker image inspect x64dbg-linux-build >/dev/null 2>&1; then
    echo "Docker image 'x64dbg-linux-build' already exists, skipping build."
else
    echo "Building Docker image 'x64dbg-linux-build'..."
    docker build -f "$SCRIPT_DIR/Dockerfile" -t x64dbg-linux-build "$SCRIPT_DIR"
fi

# Run the build with the entire repository mounted
docker run --rm \
    -v "$REPO_ROOT:/build" \
    -w /build/src/cross/x64dbg-linux \
    x64dbg-linux-build \
    bash -c "
        rm -rf build && \
        mkdir -p build && \
        cd build && \
        cmake /build/src/cross/x64dbg-linux -G Ninja -DCMAKE_BUILD_TYPE=Release && \
        cmake --build .
    "

echo "Build complete!"
