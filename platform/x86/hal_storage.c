/* x86 platform HAL — storage (FAT12 file ops on top of ASM sector I/O). */

#include "platform.h"
#include "kernel.h"
#include <stdint.h>

extern void init_fat12(void);
extern int fat12_read_sector(uint32_t lba, void *buf);
extern int fat12_write_sector(uint32_t lba, const void *buf);

#define FAT_ROOT_LBA       19
#define FAT_ROOT_SECTORS   14
#define FAT_ROOT_ENTRIES   224
#define FAT_DATA_START     33
#define FAT_SECTORS        9
#define FAT_END            0xFF8

static uint8_t fat_cache[FAT_SECTORS * 512];
static uint8_t root_cache[FAT_ROOT_SECTORS * 512];
static int fs_ready;

static void fat_normalize(const char *src, char *dest83) {
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

static int fat_load_tables(void) {
    int s;
    for (s = 0; s < FAT_SECTORS; s++) {
        if (fat12_read_sector(1 + s, fat_cache + s * 512) != 0)
            return -1;
    }
    for (s = 0; s < FAT_ROOT_SECTORS; s++) {
        if (fat12_read_sector(FAT_ROOT_LBA + s, root_cache + s * 512) != 0)
            return -1;
    }
    return 0;
}

static int fat_flush_root(void) {
    int s;
    for (s = 0; s < FAT_ROOT_SECTORS; s++) {
        if (fat12_write_sector(FAT_ROOT_LBA + s, root_cache + s * 512) != 0)
            return -1;
    }
    return 0;
}

static uint16_t fat_next_cluster(uint16_t cluster) {
    uint32_t off = cluster + (cluster >> 1);
    uint16_t val = *(uint16_t *)(fat_cache + off);
    if (cluster & 1) val >>= 4;
    else val &= 0x0FFF;
    return val;
}

static int fat_find_entry(const char *name83, int *slot_out) {
    int i;
    for (i = 0; i < FAT_ROOT_ENTRIES; i++) {
        uint8_t *e = root_cache + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) continue;
        if (e[11] & 0x10) continue;
        if (slot_out) *slot_out = i;
        int j;
        for (j = 0; j < 11; j++)
            if (e[j] != (uint8_t)name83[j]) break;
        if (j == 11) return i;
    }
    return -1;
}

static int fat_find_free_entry(void) {
    int i;
    for (i = 0; i < FAT_ROOT_ENTRIES; i++) {
        uint8_t *e = root_cache + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) return i;
    }
    return -1;
}

int plat_fs_init(void) {
    init_fat12();
    fs_ready = (fat_load_tables() == 0);
    return fs_ready ? 0 : -1;
}

int plat_fs_list(plat_file_info_t *out, unsigned int max) {
    unsigned int n = 0;
    if (!fs_ready && plat_fs_init() != 0) return -1;
    int i;
    for (i = 0; i < FAT_ROOT_ENTRIES && n < max; i++) {
        uint8_t *e = root_cache + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) continue;
        if (e[11] & 0x10) continue;
        int j, k = 0;
        for (j = 0; j < 8 && e[j] != ' '; j++) out[n].name[k++] = (char)e[j];
        if (e[8] != ' ') {
            out[n].name[k++] = '.';
            for (j = 8; j < 11 && e[j] != ' '; j++) out[n].name[k++] = (char)e[j];
        }
        out[n].name[k] = '\0';
        out[n].size = *(uint32_t *)(e + 28);
        out[n].cluster = *(uint16_t *)(e + 26);
        n++;
    }
    return (int)n;
}

int plat_fs_read(const char *name, void *buf, uint32_t buf_size, uint32_t *out_size) {
    char name83[11];
    int slot;
    if (!fs_ready && plat_fs_init() != 0) return -1;
    fat_normalize(name, name83);
    slot = fat_find_entry(name83, 0);
    if (slot < 0) return -1;
    uint8_t *e = root_cache + slot * 32;
    uint32_t fsize = *(uint32_t *)(e + 28);
    uint16_t cluster = *(uint16_t *)(e + 26);
    uint32_t copied = 0;
    uint8_t sector[512];
    while (cluster < FAT_END && copied < fsize && copied < buf_size) {
        if (fat12_read_sector(FAT_DATA_START + cluster - 2, sector) != 0) return -1;
        uint32_t n = 512;
        if (copied + n > fsize) n = fsize - copied;
        if (copied + n > buf_size) n = buf_size - copied;
        uint32_t j;
        for (j = 0; j < n; j++) ((uint8_t *)buf)[copied + j] = sector[j];
        copied += n;
        cluster = fat_next_cluster(cluster);
    }
    if (out_size) *out_size = copied;
    return 0;
}

int plat_fs_write(const char *name, const void *data, uint32_t size) {
    char name83[11];
    if (!fs_ready && plat_fs_init() != 0) return -1;
    fat_normalize(name, name83);
    int slot = fat_find_entry(name83, 0);
    if (slot < 0) {
        slot = fat_find_free_entry();
        if (slot < 0) return -1;
        uint8_t *e = root_cache + slot * 32;
        int i;
        for (i = 0; i < 11; i++) e[i] = (uint8_t)name83[i];
        e[11] = 0x20;
    }
    uint8_t *e = root_cache + slot * 32;
    *(uint32_t *)(e + 28) = size;
    uint16_t cluster = 2;
    *(uint16_t *)(e + 26) = cluster;
    const uint8_t *src = (const uint8_t *)data;
    uint32_t remaining = size;
    uint8_t sector[512];
    while (remaining > 0 && cluster < FAT_END) {
        uint32_t n = remaining > 512 ? 512 : remaining;
        uint32_t j;
        for (j = 0; j < 512; j++) sector[j] = (j < n) ? src[j] : 0;
        if (fat12_write_sector(FAT_DATA_START + cluster - 2, sector) != 0) return -1;
        src += n;
        remaining -= n;
        cluster++;
    }
    return fat_flush_root();
}

int plat_fs_delete(const char *name) {
    char name83[11];
    if (!fs_ready && plat_fs_init() != 0) return -1;
    fat_normalize(name, name83);
    int slot = fat_find_entry(name83, 0);
    if (slot < 0) return -1;
    root_cache[slot * 32] = 0xE5;
    return fat_flush_root();
}

int plat_fs_validate(void) {
    uint8_t boot[512];
    if (fat12_read_sector(0, boot) != 0) return -1;
    return (boot[510] == 0x55 && boot[511] == 0xAA) ? 0 : -1;
}

int plat_fs_repair(void) {
    if (plat_fs_init() != 0) return -1;
    return 0;
}

int plat_fs_read_sector(uint32_t lba, void *buf) {
    return fat12_read_sector(lba, buf);
}

int plat_fs_write_sector(uint32_t lba, const void *buf) {
    return fat12_write_sector(lba, buf);
}

/* C wrappers for legacy fs.h API */
void init_fat12_c(void) { plat_fs_init(); }

int fat12_read_file(const char *filename, void *buffer) {
    uint32_t sz;
    if (plat_fs_read(filename, buffer, 65536, &sz) != 0) return -1;
    return (int)sz;
}

int fat12_write_file(const char *filename, const void *buffer, uint32_t size) {
    return plat_fs_write(filename, buffer, size);
}

int fat12_delete_file(const char *filename) {
    return plat_fs_delete(filename);
}

int fat12_validate_fs(void) {
    return plat_fs_validate();
}
