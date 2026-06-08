#!/bin/bash
# Assemble FreeMCBoot memory-card deploy package.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/deploy/fmcb"
mkdir -p "$OUT/BOOT" "$OUT/asmos"

echo "Building FMCB package at $OUT"

if [ -f "$ROOT/build/asmos.elf" ]; then
    cp "$ROOT/build/asmos.elf" "$OUT/BOOT/ASMOS.ELF"
    echo "Copied PS2 native ELF"
elif [ -f "$ROOT/build/kernel.elf" ]; then
    cp "$ROOT/build/kernel.elf" "$OUT/BOOT/ASMOS.ELF"
    echo "Note: using x86 kernel.elf placeholder — build with PS2SDK for native ELF"
else
    echo "Run 'make all' first"
    exit 1
fi

cat > "$OUT/README_FMCB.md" <<'EOF'
# ASMOS FreeMCBoot Package

Copy contents to memory card root:

- `BOOT/ASMOS.ELF` — launch from FMCB menu or configure auto-boot in SYS-CONF
- `asmos/` — config, saves, profiles

Install via USB adapter or ps2client:

    scripts/install_fmcb.sh <ps2_ip>
EOF

echo "FMCB package ready: $OUT"
