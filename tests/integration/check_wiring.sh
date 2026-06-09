#!/bin/bash
# Verify arch_x86 NASM layer is linked into kernel.elf
set -e
cd "$(dirname "$0")/../.."
ELF="${1:-build/kernel.elf}"
test -f "$ELF" || { echo "FAIL: missing $ELF"; exit 1; }

syms=(
    _kernel_start
    disk_read_sector
    disk_write_sector
    detect_ps2_memory
    sys_read_line
    sys_timer_init
    sys_sound_init
    fat12_read_sector
    fat12_list_files
    plat_fs_init
    plat_net_init
    task_yield_asm
    vga_putchar
    system_reboot
)

for sym in "${syms[@]}"; do
    if ! nm "$ELF" 2>/dev/null | grep -q " T $sym"; then
        echo "FAIL: missing wired symbol $sym"
        exit 1
    fi
    echo "PASS: wired $sym"
done

echo "PASS: arch_x86 wiring complete"
