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

OUT="$(timeout 20 "$QEMU" \
    -drive file="$IMG",format=raw,if=ide,index=0,media=disk \
    -m 32 \
    -display none \
    -device isa-debugcon,iobase=0xe9 \
    -debugcon file:/tmp/asmos_boot.log \
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
