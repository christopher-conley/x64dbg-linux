#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Build
cmake -G Ninja -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target x64dbg-linux

# Create AppDir structure
APPDIR="$BUILD_DIR/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Copy binary
cp "$BUILD_DIR/x64dbg-linux" "$APPDIR/usr/bin/"

# Copy icon
cp "$REPO_ROOT/src/bug_black_outline.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/x64dbg.png"

# Create .desktop file
cat > "$APPDIR/usr/share/applications/x64dbg.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=x64dbg
Exec=x64dbg-linux
Icon=x64dbg
Categories=Development;Debugger;
Comment=Open-source debugger for Linux
EOF

# Build AppImage
export QMAKE=/usr/bin/qmake
export OUTPUT="$BUILD_DIR/x64dbg-linux-x86_64.AppImage"
linuxdeploy --appdir "$APPDIR" \
    --plugin qt \
    --output appimage \
    --desktop-file "$APPDIR/usr/share/applications/x64dbg.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/x64dbg.png" \
    --executable "$APPDIR/usr/bin/x64dbg-linux"

echo ""
echo "Built: $OUTPUT"
