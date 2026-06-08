#ifndef FAT12_H
#define FAT12_H

#include <stdint.h>
#include "block_dev.h"

#define FAT12_ROOT_ENTRIES   224
#define FAT12_ROOT_SECTORS   14
#define FAT12_SECTORS_PER_FAT 9
#define FAT12_RESERVED_SECTORS 1
#define FAT12_DATA_START     (FAT12_RESERVED_SECTORS + FAT12_SECTORS_PER_FAT * 2 + FAT12_ROOT_SECTORS)
#define FAT12_CLUSTER_EOF    0xFF8
#define FAT12_NAME_LEN       11

typedef struct {
    block_dev_t *dev;
    uint8_t fat[FAT12_SECTORS_PER_FAT * BLOCK_SECTOR_SIZE];
    uint8_t root[FAT12_ROOT_SECTORS * BLOCK_SECTOR_SIZE];
    int ready;
} fat12_fs_t;

void fat12_init(fat12_fs_t *fs, block_dev_t *dev);
int  fat12_mount(fat12_fs_t *fs);
uint16_t fat12_next_cluster(const fat12_fs_t *fs, uint16_t cluster);
int  fat12_read_cluster(const fat12_fs_t *fs, uint16_t cluster, void *buf);
int  fat12_find_entry(const fat12_fs_t *fs, const char *name83, int *slot_out);
int  fat12_read_file(const fat12_fs_t *fs, const char *name83, void *buf, uint32_t max, uint32_t *out_size);
int  fat12_write_sector(fat12_fs_t *fs, uint32_t lba, const void *buf);
int  fat12_read_sector(const fat12_fs_t *fs, uint32_t lba, void *buf);
void fat12_normalize_name(const char *src, char *dest83);

#endif /* FAT12_H */
