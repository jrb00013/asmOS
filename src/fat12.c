/* Unified FAT12 implementation for boot stage2 and kernel HAL. */

#include "fat12.h"

void fat12_normalize_name(const char *src, char *dest83) {
    int i, j = 0;
    for (i = 0; i < 8; i++) dest83[j++] = ' ';
    for (i = 0; i < 3; i++) dest83[8 + i] = ' ';
    i = 0;
    while (src[i] && src[i] != '.' && j < 8) {
        char c = src[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        dest83[j++] = c;
    }
    if (src[i] == '.') i++;
    j = 8;
    while (src[i] && j < 11) {
        char c = src[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        dest83[j++] = c;
    }
}

void fat12_init(fat12_fs_t *fs, block_dev_t *dev) {
    if (!fs) return;
    fs->dev = dev;
    fs->ready = 0;
}

int fat12_read_sector(const fat12_fs_t *fs, uint32_t lba, void *buf) {
    if (!fs || !fs->dev) return -1;
    return block_dev_read(fs->dev, lba, buf);
}

int fat12_write_sector(fat12_fs_t *fs, uint32_t lba, const void *buf) {
    if (!fs || !fs->dev) return -1;
    int r = block_dev_write(fs->dev, lba, buf);
    if (r != 0) return r;
    /* Mirror FAT copy 2 when writing FAT copy 1 */
    if (lba >= FAT12_RESERVED_SECTORS &&
        lba < (uint32_t)(FAT12_RESERVED_SECTORS + FAT12_SECTORS_PER_FAT)) {
        uint32_t mirror = lba + FAT12_SECTORS_PER_FAT;
        block_dev_write(fs->dev, mirror, buf);
    }
    return 0;
}

int fat12_mount(fat12_fs_t *fs) {
    int s;
    if (!fs || !fs->dev) return -1;
    for (s = 0; s < FAT12_SECTORS_PER_FAT; s++) {
        if (fat12_read_sector(fs, FAT12_RESERVED_SECTORS + s,
                              fs->fat + s * BLOCK_SECTOR_SIZE) != 0)
            return -1;
    }
    for (s = 0; s < FAT12_ROOT_SECTORS; s++) {
        if (fat12_read_sector(fs, FAT12_RESERVED_SECTORS + FAT12_SECTORS_PER_FAT * 2 + s,
                              fs->root + s * BLOCK_SECTOR_SIZE) != 0)
            return -1;
    }
    fs->ready = 1;
    return 0;
}

uint16_t fat12_next_cluster(const fat12_fs_t *fs, uint16_t cluster) {
    uint32_t off = cluster + (cluster >> 1);
    uint16_t val = *(uint16_t *)(fs->fat + off);
    if (cluster & 1) val >>= 4;
    else val &= 0x0FFF;
    return val;
}

int fat12_read_cluster(const fat12_fs_t *fs, uint16_t cluster, void *buf) {
    if (cluster < 2) return -1;
    uint32_t lba = FAT12_DATA_START + (cluster - 2);
    return fat12_read_sector(fs, lba, buf);
}

int fat12_find_entry(const fat12_fs_t *fs, const char *name83, int *slot_out) {
    int i;
    for (i = 0; i < FAT12_ROOT_ENTRIES; i++) {
        uint8_t *e = fs->root + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) continue;
        if (e[11] & 0x10) continue;
        int j;
        for (j = 0; j < 11; j++)
            if (e[j] != (uint8_t)name83[j]) break;
        if (j == 11) {
            if (slot_out) *slot_out = i;
            return 0;
        }
    }
    return -1;
}

int fat12_read_file(const fat12_fs_t *fs, const char *name83, void *buf, uint32_t max, uint32_t *out_size) {
    int slot;
    if (!fs || !fs->ready || fat12_find_entry(fs, name83, &slot) != 0) return -1;
    uint8_t *e = fs->root + slot * 32;
    uint16_t cluster = *(uint16_t *)(e + 26);
    uint32_t size = *(uint32_t *)(e + 28);
    uint32_t written = 0;
    uint8_t *dst = (uint8_t *)buf;
    while (cluster < FAT12_CLUSTER_EOF && written < max) {
        if (fat12_read_cluster(fs, cluster, dst + written) != 0) return -1;
        written += BLOCK_SECTOR_SIZE;
        cluster = fat12_next_cluster(fs, cluster);
    }
    if (written > size) written = size;
    if (out_size) *out_size = written;
    return 0;
}
