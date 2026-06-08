#!/usr/bin/env bash
# Put files into FAT12 image without mtools. Usage: fat_put.sh image file [8.3_NAME]
set -euo pipefail
IMG="${1:?img}"
SRC="${2:?src}"
NAME="${3:-$(basename "$SRC" | tr '[:lower:]' '[:upper:]')}"
python3 - "$IMG" "$SRC" "$NAME" <<'PY'
import struct, sys
img, src, name = sys.argv[1:4]
name = name.upper().ljust(11)[:11]
data = open(src, "rb").read()
with open(img, "r+b") as f:
    f.seek(19 * 512)
    root = bytearray(f.read(14 * 512))
    slot = None
    for i in range(224):
        o = i * 32
        if root[o] in (0x00, 0xE5):
            slot = i
            break
    if slot is None:
        sys.exit("no root dir slot")
    o = slot * 32
    root[o:o+11] = name.encode("ascii")
    root[o+11] = 0x20
    root[o+26:o+28] = struct.pack("<H", 2)
    root[o+28:o+32] = struct.pack("<I", len(data))
    f.seek(19 * 512)
    f.write(root)
    cluster = 2
    off = 33 * 512 + (cluster - 2) * 512
    for i in range(0, len(data), 512):
        f.seek(off + i)
        f.write(data[i:i+512])
print(f"fat_put: {name.strip()} ({len(data)} bytes)")
PY
