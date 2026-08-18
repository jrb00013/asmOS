#!/usr/bin/env bash
# Boot disk/os.img in QEMU; verify isa-debugcon boot markers.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
IMG="$ROOT/disk/os.img"
QEMU="$(command -v qemu-system-i386 2>/dev/null || command -v qemu-system-x86_64 2>/dev/null || true)"
if [[ -z "$QEMU" ]]; then
    echo "SKIP: QEMU not installed (apt install qemu-system-x86)"
    exit 0
fi
[[ -f "$IMG" ]] || { echo "FAIL: missing $IMG"; exit 1; }

# Two things here differ from what you'd naively write, both found by
# actually running this against a real QEMU (8.2) instead of assuming:
#
# 1. "-device isa-debugcon,iobase=0xe9 -debugcon file:PATH" errors out
#    immediately on current QEMU ("Can't create debugcon device, empty
#    char device") — the old shorthand no longer auto-creates a backend.
#    Create the chardev explicitly and attach the device to it instead.
#
# 2. "-drive ...,if=ide" makes QEMU/SeaBIOS treat this as an IDE hard
#    disk, which auto-detects its own CHS geometry rather than trusting
#    this image's own FAT12 BPB (18 sectors/track, 2 heads). That
#    geometry mismatch made every INT13h CHS read in boot/bootsect.asm
#    and boot/loader.asm land on the wrong physical sectors — reads
#    reported success but returned unrelated (usually all-zero) data.
#    This image is a 1.44MB floppy-formatted FAT12 volume, not a
#    partitioned hard disk, so booting it with "-fda" (BIOS floppy path,
#    whose CHS geometry is unambiguous and matches this BPB) is the
#    correct way to run it, not "-drive if=ide".
OUT="$(timeout 20 "$QEMU" \
    -fda "$IMG" \
    -m 32 \
    -display none \
    -chardev file,id=asmosdbg,path=/tmp/asmos_boot.log \
    -device isa-debugcon,iobase=0xe9,chardev=asmosdbg \
    -no-reboot \
    2>/dev/null || true)"

LOG=/tmp/asmos_boot.log
if [[ -f "$LOG" ]]; then
    cat "$LOG"
    if grep -q "DEBUG:KERNEL_LOADED" "$LOG" && grep -q "DEBUG:KERNEL_START" "$LOG"; then
        echo "PASS: QEMU boot reached kernel entry"
        exit 0
    fi
    if grep -q "DEBUG:LOADER_START" "$LOG"; then
        echo "WARN: loader ran but kernel did not start (check PM jump)"
        exit 1
    fi
fi
echo "FAIL: no debugcon boot markers in $LOG"
exit 1
