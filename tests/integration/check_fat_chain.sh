#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
IMG="$ROOT/disk/os.img"
python3 - "$IMG" <<'PY'
import struct, sys
RESERVED, FAT_START, FAT_SECTORS, ROOT_LBA, ROOT_SECTORS, DATA_START = 33, 33, 9, 51, 14, 65
img = open(sys.argv[1], "rb").read()
fat = img[FAT_START*512:FAT_START*512+FAT_SECTORS*512]
root = img[ROOT_LBA*512:ROOT_LBA*512+ROOT_SECTORS*512]
name = b"KERNEL  BIN"
slot = None
for i in range(224):
    o = i*32
    if root[o:o+11] == name:
        slot = o
        break
if slot is None:
    sys.exit("KERNEL  BIN not in root")
start = struct.unpack_from("<H", root, slot+26)[0]
size = struct.unpack_from("<I", root, slot+28)[0]
print(f"KERNEL  BIN start_cluster={start} size={size}")

def get_fat(c):
    off = c + (c >> 1)
    if off + 1 >= len(fat):
        return 0xFFF
    if c & 1:
        return (fat[off] >> 4) | (fat[off+1] << 4)
    return fat[off] | ((fat[off+1] & 0x0F) << 8)

chain = []
c = start
for _ in range(500):
    chain.append(c)
    n = get_fat(c)
    if n >= 0xFF8:
        break
    c = n
expected = (size + 511) // 512
print(f"chain_len={len(chain)} expected_clusters={expected}")
if len(chain) < expected:
    sys.exit(f"FAIL: FAT chain too short ({len(chain)} < {expected})")
print("PASS: FAT chain OK")
PY
