#!/bin/bash
# Integration checks: build + verify v2.0 symbols and no stub strings in shell.
set -e
cd "$(dirname "$0")/../.."

echo "=== ASMOS integration checks ==="

make clean >/dev/null 2>&1 || true
make all

for sym in fat12_read_file plat_fs_write plat_net_init net_init; do
    if ! nm build/kernel.elf 2>/dev/null | grep -q " $sym"; then
        echo "FAIL: missing symbol $sym"
        exit 1
    fi
    echo "PASS: symbol $sym"
done

if grep -q "not yet implemented" src/shell.c; then
    echo "FAIL: shell still contains stub messages"
    exit 1
fi
echo "PASS: no shell stub messages"

echo "=== All integration checks passed ==="
