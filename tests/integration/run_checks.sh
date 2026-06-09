#!/bin/bash
# Integration checks: build + FAT chain + symbols + optional QEMU boot.
set -e
cd "$(dirname "$0")/../.."

echo "=== ASMOS integration checks ==="

make clean >/dev/null 2>&1 || true
make all

bash tests/integration/check_fat_chain.sh
bash tests/integration/check_wiring.sh

for sym in _kernel_start fat12_read_file plat_fs_write plat_net_init net_init subsys_init_all; do
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

bash tests/integration/boot_qemu.sh

echo "=== All integration checks passed ==="
