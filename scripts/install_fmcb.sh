#!/bin/bash
# Install FMCB package to PS2 via ps2client (requires network + ps2link).
set -e
PS2_IP="${1:-}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG="$ROOT/deploy/fmcb"

if [ -z "$PS2_IP" ]; then
    echo "Usage: $0 <ps2_ip>"
    exit 1
fi

if [ ! -d "$PKG/BOOT" ]; then
    bash "$ROOT/scripts/build_fmcb_package.sh"
fi

if ! command -v ps2client >/dev/null 2>&1; then
    echo "ps2client not found. Copy $PKG to MC manually."
    exit 1
fi

export PS2IP="$PS2_IP"
ps2client -h "$PS2_IP" exec "host:$PKG/BOOT/ASMOS.ELF" || true
echo "Install attempted via ps2client. Verify on PS2 hardware."
