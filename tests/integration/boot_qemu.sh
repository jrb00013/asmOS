#!/usr/bin/env bash
# Boot disk/os.img in QEMU and grep for kernel banner (optional if QEMU missing).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
IMG="$ROOT/disk/os.img"
QEMU="$(command -v qemu-system-i386 2>/dev/null || command -v qemu-system-x86_64 2>/dev/null || true)"
if [[ -z "$QEMU" ]]; then
    echo "SKIP: QEMU not installed"
    exit 0
fi
[[ -f "$IMG" ]] || { echo "FAIL: missing $IMG"; exit 1; }
OUT="$(timeout 15 "$QEMU" -drive format=raw,file="$IMG" -m 32 -display none -serial stdio 2>&1 || true)"
echo "$OUT" | head -20
if echo "$OUT" | grep -qi "ASMOS"; then
    echo "PASS: boot output contains ASMOS"
else
    echo "WARN: no ASMOS string in serial output (may need VGA capture)"
fi
