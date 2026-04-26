#!/bin/bash
set -e

# mmsim System Install Script
# Installs to /usr/local/bin, /usr/local/lib/mmsim, and /usr/local/share/mmsim

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (e.g. sudo ./install-system.sh)"
  exit 1
fi

DEST_BIN="/usr/local/bin"
DEST_LIB="/usr/local/lib/mmsim/plugins"
DEST_SHARE="/usr/local/share/mmsim"
DEST_MAN1="/usr/local/share/man/man1"
DEST_MAN7="/usr/local/share/man/man7"

echo "Installing mmsim to system directories..."

# Create directories
mkdir -p "$DEST_BIN"
mkdir -p "$DEST_LIB"
mkdir -p "$DEST_SHARE"
mkdir -p "$DEST_MAN1"
mkdir -p "$DEST_MAN7"

# Copy binaries from bin/
if [ -d "bin" ]; then
    echo "Copying binaries to $DEST_BIN..."
    cp bin/mmemu-cli "$DEST_BIN/"
    cp bin/mmemu-gui "$DEST_BIN/"
    cp bin/mmemu-mcp "$DEST_BIN/"
    chmod 755 "$DEST_BIN"/mmemu-*
else
    echo "Error: bin/ directory not found. Please run 'make' first."
    exit 1
fi

# Copy plugins from lib/
if [ -d "lib" ]; then
    echo "Copying plugins to $DEST_LIB..."
    cp lib/mmemu-plugin-*.so "$DEST_LIB/"
    chmod 644 "$DEST_LIB"/*.so
else
    echo "Warning: lib/ directory not found."
fi

# Copy machines and roms to share/
echo "Copying machines and roms to $DEST_SHARE..."
cp -a machines "$DEST_SHARE/"
cp -a roms "$DEST_SHARE/"
find "$DEST_SHARE" -type d -exec chmod 755 {} +
find "$DEST_SHARE" -type f -exec chmod 644 {} +

# Generate and install man pages
if command -v pandoc >/dev/null 2>&1; then
    echo "Generating and installing man pages..."
    make man
    cp man/*.1 "$DEST_MAN1/"
    cp man/*.7 "$DEST_MAN7/"
    chmod 644 "$DEST_MAN1"/mmemu*.1 2>/dev/null || true
    chmod 644 "$DEST_MAN7"/mmemu*.7 2>/dev/null || true
else
    echo "Warning: pandoc not found. Skipping man page generation."
fi

echo "--------------------------------------------------"
echo "System install complete."
echo "Binaries: $DEST_BIN"
echo "Plugins:  $DEST_LIB"
echo "Data:     $DEST_SHARE"
echo "Man:      /usr/local/share/man"
