/* Stage2 bootloader: protected-mode FAT12 kernel loader (alternate path).
 * Not linked by the x86 Makefile — live boot uses NASM stage1 + fatload16. */

#include <stdint.h>
#include "boot_meta.h"
#include "block_dev.h"
#include "fat12.h"

static uint8_t boot_drive_storage;
static uint32_t crc32_byte(uint32_t crc, uint8_t b) {
    crc ^= b;
    for (int i = 0; i < 8; i++)
        crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1)));
    return crc;
}

static uint32_t crc32_buf(const void *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < len; i++)
        crc = crc32_byte(crc, p[i]);
    return ~crc;
}

typedef void (*kernel_entry_t)(void);

void _stage2_start(void) __attribute__((section(".text")));

void _stage2_start(void) {
    block_dev_t dev;
    fat12_fs_t fs;
    block_dev_init(&dev, boot_drive_storage);
    fat12_init(&fs, &dev);
    if (fat12_mount(&fs) != 0)
        for (;;);

    asmos_boot_meta_t meta;
    if (fat12_read_file(&fs, ASMOS_META_FILENAME, &meta, sizeof(meta), 0) != 0)
        for (;;);
    if (meta.magic[0] != 'A')
        for (;;);

    uint8_t *load = (uint8_t *)(uintptr_t)meta.kernel_load_addr;
    uint32_t loaded = 0;
    char kname[] = ASMOS_KERNEL_FILENAME;
    int slot;
    if (fat12_find_entry(&fs, kname, &slot) != 0)
        for (;;);
    uint8_t *e = fs.root + slot * 32;
    uint16_t cluster = *(uint16_t *)(e + 26);
    uint32_t size = *(uint32_t *)(e + 28);
    while (cluster < FAT12_CLUSTER_EOF && loaded < size) {
        fat12_read_cluster(&fs, cluster, load + loaded);
        loaded += BLOCK_SECTOR_SIZE;
        cluster = fat12_next_cluster(&fs, cluster);
    }
    if (loaded > size) loaded = size;

    uint32_t crc = crc32_buf(load, loaded);
    if (meta.kernel_crc32 && crc != meta.kernel_crc32)
        for (;;);

    uint32_t *mk = (uint32_t *)load;
    if (mk[0] != ASMOS_KERNEL_MAGIC)
        for (;;);

    kernel_entry_t entry = (kernel_entry_t)(uintptr_t)meta.kernel_entry;
    entry();
    for (;;);
}

/* Stage1 sets this before jumping to protected mode. */
uint8_t boot_drive_storage __attribute__((section(".data"))) = 0x00;
