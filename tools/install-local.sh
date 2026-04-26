#!/bin/bash
set -e

# mmsim Local User Install Script
# Installs to ~/.local/bin, ~/.local/lib/mmsim, and ~/.local/share/mmsim

DEST_BIN="$HOME/.local/bin"
DEST_LIB="$HOME/.local/lib/mmsim/plugins"
DEST_SHARE="$HOME/.local/share/mmsim"
DEST_MAN1="$HOME/.local/share/man/man1"
DEST_MAN7="$HOME/.local/share/man/man7"

echo "Installing mmsim to local user directories..."

# Create directories
mkdir -p "$DEST_BIN"
mkdir -p "$DEST_LIB"
mkdir -p "$DEST_SHARE"
mkdir -p "$DEST_MAN1"
mkdir -p "$DEST_MAN7"

# Copy binaries from bin/
if [ -d "bin" ]; then
    echo "Copying binaries to $DEST_BIN..."
    cp bin/mmemu-cli "$DEST_BIN/" 2>/dev/null || true
    cp bin/mmemu-gui "$DEST_BIN/" 2>/dev/null || true
    cp bin/mmemu-mcp "$DEST_BIN/" 2>/dev/null || true
else
    echo "Warning: bin/ directory not found. Did you run 'make'?"
fi

# Copy plugins from lib/
if [ -d "lib" ]; then
    echo "Copying plugins to $DEST_LIB..."
    cp lib/mmemu-plugin-*.so "$DEST_LIB/" 2>/dev/null || true
else
    echo "Warning: lib/ directory not found."
fi

# Copy machines and roms to share/
echo "Copying machines and roms to $DEST_SHARE..."
cp -a machines "$DEST_SHARE/"
cp -a roms "$DEST_SHARE/"

# Generate and install man pages
if command -v pandoc >/dev/null 2>&1; then
    echo "Generating and installing man pages..."
    make man
    cp man/*.1 "$DEST_MAN1/" 2>/dev/null || true
    cp man/*.7 "$DEST_MAN7/" 2>/dev/null || true
else
    echo "Warning: pandoc not found. Skipping man page generation."
fi

echo "--------------------------------------------------"
echo "Local install complete."
echo "Ensure $DEST_BIN is in your PATH."
echo "Plugins will be loaded from $DEST_LIB"
echo "Resources will be loaded from $DEST_SHARE"
echo "Man pages installed to $HOME/.local/share/man"
