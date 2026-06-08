#!/usr/bin/env python3
"""Install files into a FAT12 floppy image with proper cluster chains."""
import struct
import sys

SECTOR = 512
RESERVED = 33
FAT_START = 33
FAT_SECTORS = 9
FAT_COPIES = 2
ROOT_LBA = 51
ROOT_SECTORS = 14
ROOT_ENTRIES = 224
DATA_START = 65
CLUSTER_EOF = 0xFF8


def get_fat_entry(fat: bytearray, cluster: int) -> int:
    off = cluster + (cluster >> 1)
    if cluster & 1:
        return (fat[off] >> 4) | (fat[off + 1] << 4)
    return fat[off] | ((fat[off + 1] & 0x0F) << 8)


def set_fat_entry(fat: bytearray, cluster: int, value: int) -> None:
    off = cluster + (cluster >> 1)
    value &= 0xFFF
    if cluster & 1:
        fat[off] = (fat[off] & 0x0F) | ((value & 0x0F) << 4)
        fat[off + 1] = (value >> 4) & 0xFF
    else:
        fat[off] = value & 0xFF
        fat[off + 1] = (fat[off + 1] & 0xF0) | ((value >> 8) & 0x0F)


def load_fat(img: bytearray) -> bytearray:
    off = FAT_START * SECTOR
    return bytearray(img[off:off + FAT_SECTORS * SECTOR])


def store_fat(img: bytearray, fat: bytearray) -> None:
    blob = fat[: FAT_SECTORS * SECTOR]
    for copy in range(FAT_COPIES):
        off = (FAT_START + copy * FAT_SECTORS) * SECTOR
        img[off:off + len(blob)] = blob


def load_root(img: bytearray) -> bytearray:
    off = ROOT_LBA * SECTOR
    return bytearray(img[off:off + ROOT_SECTORS * SECTOR])


def store_root(img: bytearray, root: bytearray) -> None:
    off = ROOT_LBA * SECTOR
    img[off:off + len(root)] = root


def find_free_cluster(fat: bytearray, start: int = 2) -> int:
    c = start
    while c < 4096:
        if get_fat_entry(fat, c) == 0:
            return c
        c += 1
    raise RuntimeError("no free clusters")


def allocate_clusters(fat: bytearray, count: int, next_start: int) -> tuple[list[int], int]:
    clusters = []
    prev = None
    cur_search = next_start
    for _ in range(count):
        c = find_free_cluster(fat, cur_search)
        clusters.append(c)
        if prev is not None:
            set_fat_entry(fat, prev, c)
        prev = c
        cur_search = c + 1
    set_fat_entry(fat, clusters[-1], CLUSTER_EOF)
    return clusters, cur_search


def find_root_slot(root: bytearray):
    for i in range(ROOT_ENTRIES):
        o = i * 32
        if root[o] in (0x00, 0xE5):
            return i
    raise RuntimeError("root directory full")


def put_file(img: bytearray, fat: bytearray, root: bytearray, src_path: str, name83: str, next_cluster: int):
    name83 = name83.upper().ljust(11)[:11]
    data = open(src_path, "rb").read()
    n_clusters = max(1, (len(data) + SECTOR - 1) // SECTOR)
    clusters, next_cluster = allocate_clusters(fat, n_clusters, next_cluster)
    slot = find_root_slot(root)
    o = slot * 32
    root[o:o + 11] = name83.encode("ascii")
    root[o + 11] = 0x20
    root[o + 26:o + 28] = struct.pack("<H", clusters[0])
    root[o + 28:o + 32] = struct.pack("<I", len(data))
    for i, cluster in enumerate(clusters):
        off = DATA_START * SECTOR + (cluster - 2) * SECTOR
        chunk = data[i * SECTOR:(i + 1) * SECTOR]
        img[off:off + SECTOR] = chunk.ljust(SECTOR, b"\x00")
    print(f"fat_put: {name83.strip()} ({len(data)} bytes, {n_clusters} clusters, start={clusters[0]})")
    return next_cluster


def format_fat12(img: bytearray, media: int = 0xF0) -> None:
    """Initialize BPB layout FAT/root on a blank image (no mkfs required)."""
    fat = bytearray(FAT_SECTORS * SECTOR)
    set_fat_entry(fat, 0, 0xFF0 | media)
    set_fat_entry(fat, 1, 0xFFF)
    store_fat(img, fat)
    store_root(img, bytearray(ROOT_SECTORS * SECTOR))


def main():
    if len(sys.argv) < 4:
        print("usage: fat_put.py image file NAME83 [file NAME83 ...]", file=sys.stderr)
        sys.exit(1)
    img_path = sys.argv[1]
    with open(img_path, "r+b") as f:
        img = bytearray(f.read())
    root = load_root(img)
    if root[0] == 0 and root[32] == 0:
        format_fat12(img)
        root = load_root(img)
    fat = load_fat(img)
    next_c = 2
    i = 2
    while i < len(sys.argv):
        src = sys.argv[i]
        name = sys.argv[i + 1]
        next_c = put_file(img, fat, root, src, name, next_c)
        i += 2
    store_fat(img, fat)
    store_root(img, root)
    with open(img_path, "r+b") as f:
        f.write(img)


if __name__ == "__main__":
    main()
