#!/bin/bash
# Assemble FreeMCBoot memory-card deploy package.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/deploy/fmcb"
mkdir -p "$OUT/BOOT" "$OUT/asmos"

echo "Building FMCB package at $OUT"

if [ ! -f "$ROOT/build/asmos.elf" ]; then
    echo "PS2 native ELF missing. Run: make ps2-native  (or ./setup.sh first)"
    exit 1
fi

cp "$ROOT/build/asmos.elf" "$OUT/BOOT/ASMOS.ELF"
echo "Copied PS2 native ELF ($(stat -c%s "$OUT/BOOT/ASMOS.ELF" 2>/dev/null || wc -c < "$OUT/BOOT/ASMOS.ELF") bytes)"

cat > "$OUT/README_FMCB.md" <<'EOF'
# ASMOS FreeMCBoot Package

Copy contents to memory card root:

- `BOOT/ASMOS.ELF` — launch from FMCB menu or configure auto-boot in SYS-CONF
- `asmos/` — config, saves, profiles

Install via USB adapter or ps2client:

    scripts/install_fmcb.sh <ps2_ip>
EOF

echo "FMCB package ready: $OUT"
