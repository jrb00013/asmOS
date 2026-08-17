/* x86 platform HAL — storage (FAT12 file ops on top of ASM sector I/O). */

#include "platform.h"
#include "kernel.h"
#include <stdint.h>

extern void init_fat12(void);
extern int fat12_read_sector(uint32_t lba, void *buf);
extern int fat12_write_sector(uint32_t lba, const void *buf);

#define FAT_LBA            33
#define FAT_ROOT_LBA       51
#define FAT_ROOT_SECTORS   14
#define FAT_ROOT_ENTRIES   224
#define FAT_DATA_START     65
#define FAT_SECTORS        9
#define FAT_END            0xFF8
#define FAT_ATTR_DIR       0x10
/* 1.44MB floppy geometry (2880 sectors), matching scripts/fat_put.sh and
 * the boot-sector BPB baked into disk/os.img. Cluster numbers 2..FAT_MAX_CLUSTER
 * address the data region that starts at FAT_DATA_START. */
#define FAT_DISK_SECTORS   2880
#define FAT_MAX_CLUSTER    (FAT_DISK_SECTORS - FAT_DATA_START + 1)

/* Directory navigation: a subdirectory's data is a cluster chain of 32-byte
 * entries, same layout as the root directory. DIR_CACHE_SECTORS bounds how
 * many clusters of a subdirectory we can hold/traverse at once (8KB = up to
 * 256 entries), which comfortably covers real FAT12 floppy-sized volumes. */
#define DIR_CACHE_SECTORS  16
#define DIR_CACHE_ENTRIES  (DIR_CACHE_SECTORS * 512 / 32)
#define CWD_PATH_MAX        128

static uint8_t fat_cache[FAT_SECTORS * 512];
static uint8_t root_cache[FAT_ROOT_SECTORS * 512];
static int fs_ready;

/* Current directory state. cur_dir_cluster == 0 means "root directory".
 * When not in root, dir_cache holds the loaded cluster chain of the
 * current directory and dir_cache_clusters records how many clusters
 * (and therefore how many valid 512-byte sectors) it holds, so writes
 * flush back only the sectors that were actually loaded. */
static uint8_t dir_cache[DIR_CACHE_SECTORS * 512];
static uint16_t dir_cache_clusters;
static uint16_t cur_dir_cluster;   /* 0 = root */
static uint16_t cur_dir_parent;    /* cluster of ".." target, 0 = root */
static char cwd_path[CWD_PATH_MAX] = "/";

static int kstreq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

static unsigned int kstrlen(const char *s) {
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}

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
        if (fat12_read_sector(33 + s, fat_cache + s * 512) != 0)
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

/* Write a 12-bit FAT entry, preserving the neighboring nibble the way real
 * FAT12 packs two entries into three bytes. */
static void fat_set_next_cluster(uint16_t cluster, uint16_t value) {
    uint32_t off = cluster + (cluster >> 1);
    uint16_t packed = *(uint16_t *)(fat_cache + off);
    if (cluster & 1) {
        packed = (uint16_t)((packed & 0x000F) | ((value & 0x0FFF) << 4));
    } else {
        packed = (uint16_t)((packed & 0xF000) | (value & 0x0FFF));
    }
    *(uint16_t *)(fat_cache + off) = packed;
}

static int fat_flush_fat(void) {
    int s;
    for (s = 0; s < FAT_SECTORS; s++) {
        if (fat12_write_sector(FAT_LBA + s, fat_cache + s * 512) != 0) return -1;
        /* Mirror to FAT copy 2, immediately after copy 1. */
        if (fat12_write_sector(FAT_LBA + FAT_SECTORS + s, fat_cache + s * 512) != 0) return -1;
    }
    return 0;
}

/* Release an entire cluster chain back to the free pool. */
static void fat_free_chain(uint16_t cluster) {
    while (cluster >= 2 && cluster < FAT_END) {
        uint16_t next = fat_next_cluster(cluster);
        fat_set_next_cluster(cluster, 0);
        cluster = next;
    }
}

/* Allocate one free cluster and mark it end-of-chain. Returns 0 if the
 * volume is full. */
static uint16_t fat_alloc_cluster(void) {
    uint16_t c;
    for (c = 2; c <= FAT_MAX_CLUSTER; c++) {
        if (fat_next_cluster(c) == 0) {
            fat_set_next_cluster(c, 0xFFF);
            return c;
        }
    }
    return 0;
}

/* Return the entry table backing the *current* directory (root or a loaded
 * subdirectory chain) and how many 32-byte slots it holds. */
static uint8_t *cur_dir_buf(unsigned int *entries_out) {
    if (cur_dir_cluster == 0) {
        if (entries_out) *entries_out = FAT_ROOT_ENTRIES;
        return root_cache;
    }
    if (entries_out) *entries_out = (unsigned int)dir_cache_clusters * 512 / 32;
    return dir_cache;
}

/* Load a subdirectory's cluster chain into dir_cache. Bounded by
 * DIR_CACHE_SECTORS so a pathological/corrupt chain can't overrun the
 * buffer; a directory bigger than that is truncated (matches the fixed
 * root-directory size limit already in this driver). */
static int fat_load_dir_chain(uint16_t first_cluster) {
    uint16_t cluster = first_cluster;
    uint16_t count = 0;
    while (cluster >= 2 && cluster < FAT_END && count < DIR_CACHE_SECTORS) {
        if (fat12_read_sector(FAT_DATA_START + cluster - 2,
                               dir_cache + count * 512) != 0)
            return -1;
        count++;
        cluster = fat_next_cluster(cluster);
    }
    dir_cache_clusters = count;
    return 0;
}

/* Flush the currently loaded directory (root or subdirectory) back to disk. */
static int fat_flush_cur_dir(void) {
    if (cur_dir_cluster == 0) return fat_flush_root();
    uint16_t cluster = cur_dir_cluster;
    int s;
    for (s = 0; s < dir_cache_clusters && cluster >= 2 && cluster < FAT_END; s++) {
        if (fat12_write_sector(FAT_DATA_START + cluster - 2,
                                dir_cache + s * 512) != 0)
            return -1;
        cluster = fat_next_cluster(cluster);
    }
    return 0;
}

static int fat_find_entry(const char *name83, int *slot_out) {
    unsigned int n, i;
    uint8_t *dir = cur_dir_buf(&n);
    for (i = 0; i < n; i++) {
        uint8_t *e = dir + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) continue;
        if (e[11] & FAT_ATTR_DIR) continue;
        int j;
        for (j = 0; j < 11; j++)
            if (e[j] != (uint8_t)name83[j]) break;
        if (j == 11) {
            if (slot_out) *slot_out = (int)i;
            return (int)i;
        }
    }
    return -1;
}

static int fat_find_dir_entry(const char *name83, int *slot_out) {
    unsigned int n, i;
    uint8_t *dir = cur_dir_buf(&n);
    for (i = 0; i < n; i++) {
        uint8_t *e = dir + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) continue;
        if (!(e[11] & FAT_ATTR_DIR)) continue;
        int j;
        for (j = 0; j < 11; j++)
            if (e[j] != (uint8_t)name83[j]) break;
        if (j == 11) {
            if (slot_out) *slot_out = (int)i;
            return (int)i;
        }
    }
    return -1;
}

static int fat_find_free_entry(void) {
    unsigned int n, i;
    uint8_t *dir = cur_dir_buf(&n);
    for (i = 0; i < n; i++) {
        uint8_t *e = dir + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) return (int)i;
    }
    return -1;
}

int plat_fs_init(void) {
    init_fat12();
    fs_ready = (fat_load_tables() == 0);
    return fs_ready ? 0 : -1;
}

void fat12_list_files(void) {
    plat_file_info_t files[32];
    int n = plat_fs_list(files, 32);
    int i;
    if (n < 0) {
        kprint("  fs: list failed\n");
        return;
    }
    for (i = 0; i < n; i++)
        kprintf("  %-12s %6u bytes\n", files[i].name, (unsigned)files[i].size);
    kprintf("  %d file(s)\n", n);
}

int plat_fs_list(plat_file_info_t *out, unsigned int max) {
    unsigned int n = 0;
    if (!fs_ready && plat_fs_init() != 0) return -1;
    unsigned int total, i;
    uint8_t *dir = cur_dir_buf(&total);
    for (i = 0; i < total && n < max; i++) {
        uint8_t *e = dir + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) continue;
        /* Skip the "." and ".." pseudo-entries in a subdirectory listing;
         * they are navigation bookkeeping, not real files. */
        if ((e[11] & FAT_ATTR_DIR) && (e[0] == '.')) continue;
        int j, k = 0;
        for (j = 0; j < 8 && e[j] != ' '; j++) out[n].name[k++] = (char)e[j];
        if (e[8] != ' ') {
            out[n].name[k++] = '.';
            for (j = 8; j < 11 && e[j] != ' '; j++) out[n].name[k++] = (char)e[j];
        }
        if (e[11] & FAT_ATTR_DIR) out[n].name[k++] = '/';
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
    uint8_t *e = cur_dir_buf(0) + slot * 32;
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
    uint8_t *dir = cur_dir_buf(0);
    if (slot < 0) {
        slot = fat_find_free_entry();
        if (slot < 0) return -1;
        uint8_t *e = dir + slot * 32;
        int i;
        for (i = 0; i < 11; i++) e[i] = (uint8_t)name83[i];
        e[11] = 0x20;
        *(uint16_t *)(e + 26) = 0;
        *(uint32_t *)(e + 28) = 0;
    }
    uint8_t *e = dir + slot * 32;

    /* Release whatever clusters this file previously held before
     * allocating a fresh chain, so overwriting a file doesn't leak the
     * clusters its old contents used. */
    uint16_t old_cluster = *(uint16_t *)(e + 26);
    if (old_cluster >= 2) fat_free_chain(old_cluster);

    *(uint32_t *)(e + 28) = size;

    const uint8_t *src = (const uint8_t *)data;
    uint32_t remaining = size;
    uint8_t sector[512];
    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;
    while (remaining > 0) {
        uint16_t cluster = fat_alloc_cluster();
        if (cluster == 0) {
            /* Disk full: unwind what we already allocated for this write
             * so a failed write doesn't leak clusters either. */
            if (first_cluster) fat_free_chain(first_cluster);
            *(uint16_t *)(e + 26) = 0;
            *(uint32_t *)(e + 28) = 0;
            return -1;
        }
        if (!first_cluster) first_cluster = cluster;
        if (prev_cluster) fat_set_next_cluster(prev_cluster, cluster);
        uint32_t n = remaining > 512 ? 512 : remaining;
        uint32_t j;
        for (j = 0; j < 512; j++) sector[j] = (j < n) ? src[j] : 0;
        if (fat12_write_sector(FAT_DATA_START + cluster - 2, sector) != 0) return -1;
        src += n;
        remaining -= n;
        prev_cluster = cluster;
    }
    *(uint16_t *)(e + 26) = first_cluster;
    if (fat_flush_fat() != 0) return -1;
    return fat_flush_cur_dir();
}

int plat_fs_delete(const char *name) {
    char name83[11];
    if (!fs_ready && plat_fs_init() != 0) return -1;
    fat_normalize(name, name83);
    int slot = fat_find_entry(name83, 0);
    if (slot < 0) return -1;
    uint8_t *dir = cur_dir_buf(0);
    uint8_t *e = dir + slot * 32;
    uint16_t cluster = *(uint16_t *)(e + 26);
    if (cluster >= 2) fat_free_chain(cluster);
    e[0] = 0xE5;
    if (fat_flush_fat() != 0) return -1;
    return fat_flush_cur_dir();
}

/* --- Directory navigation --------------------------------------------- */

int plat_fs_cwd(char *buf, unsigned int max) {
    unsigned int i;
    if (!buf || max == 0) return -1;
    for (i = 0; i + 1 < max && cwd_path[i]; i++) buf[i] = cwd_path[i];
    buf[i] = '\0';
    return 0;
}

int plat_fs_mkdir(const char *name) {
    char name83[11];
    if (!fs_ready && plat_fs_init() != 0) return -1;
    if (kstreq(name, ".") || kstreq(name, "..") || kstreq(name, "/")) return -1;
    fat_normalize(name, name83);
    if (fat_find_dir_entry(name83, 0) >= 0 || fat_find_entry(name83, 0) >= 0)
        return -1; /* already exists */

    int slot = fat_find_free_entry();
    if (slot < 0) return -1;

    uint16_t new_cluster = fat_alloc_cluster();
    if (new_cluster == 0) return -1;

    /* Initialize the new directory's own cluster: "." points at itself,
     * ".." points at the current directory (0 = root, matching the FAT12
     * convention that a ".." entry pointing at the root uses cluster 0). */
    uint8_t sector[512];
    unsigned int i;
    for (i = 0; i < 512; i++) sector[i] = 0;
    uint8_t *dot = sector;
    for (i = 0; i < 11; i++) dot[i] = ' ';
    dot[0] = '.';
    dot[11] = 0x20 | FAT_ATTR_DIR;
    *(uint16_t *)(dot + 26) = new_cluster;
    *(uint32_t *)(dot + 28) = 0;

    uint8_t *dotdot = sector + 32;
    for (i = 0; i < 11; i++) dotdot[i] = ' ';
    dotdot[0] = '.'; dotdot[1] = '.';
    dotdot[11] = 0x20 | FAT_ATTR_DIR;
    *(uint16_t *)(dotdot + 26) = cur_dir_cluster; /* 0 == root, by convention */
    *(uint32_t *)(dotdot + 28) = 0;

    if (fat12_write_sector(FAT_DATA_START + new_cluster - 2, sector) != 0) {
        fat_free_chain(new_cluster);
        return -1;
    }

    uint8_t *dir = cur_dir_buf(0);
    uint8_t *e = dir + slot * 32;
    for (i = 0; i < 11; i++) e[i] = (uint8_t)name83[i];
    e[11] = FAT_ATTR_DIR;
    *(uint16_t *)(e + 26) = new_cluster;
    *(uint32_t *)(e + 28) = 0;

    if (fat_flush_fat() != 0) return -1;
    return fat_flush_cur_dir();
}

int plat_fs_chdir(const char *name) {
    if (!fs_ready && plat_fs_init() != 0) return -1;

    if (kstreq(name, ".") || kstreq(name, "")) return 0;

    if (kstreq(name, "/")) {
        cur_dir_cluster = 0;
        cur_dir_parent = 0;
        cwd_path[0] = '/';
        cwd_path[1] = '\0';
        return 0;
    }

    if (kstreq(name, "..")) {
        if (cur_dir_cluster == 0) return 0; /* already at root */
        uint16_t parent = cur_dir_parent;
        cur_dir_cluster = parent;
        if (parent != 0) {
            if (fat_load_dir_chain(parent) != 0) return -1;
            /* Re-derive parent's own ".." target for further ascension. */
            cur_dir_parent = *(uint16_t *)(dir_cache + 32 + 26);
        } else {
            cur_dir_parent = 0;
        }
        /* Trim the last path component from cwd_path. */
        unsigned int len = kstrlen(cwd_path);
        while (len > 1 && cwd_path[len - 1] != '/') len--;
        if (len > 1) len--; /* drop the trailing '/' unless it's root */
        cwd_path[len] = '\0';
        if (len == 0) { cwd_path[0] = '/'; cwd_path[1] = '\0'; }
        return 0;
    }

    char name83[11];
    fat_normalize(name, name83);
    int slot = fat_find_dir_entry(name83, 0);
    if (slot < 0) return -1;
    uint8_t *dir = cur_dir_buf(0);
    uint16_t target = *(uint16_t *)(dir + slot * 32 + 26);
    uint16_t prev_dir = cur_dir_cluster;

    if (fat_load_dir_chain(target) != 0) return -1;
    cur_dir_parent = prev_dir;
    cur_dir_cluster = target;

    unsigned int len = kstrlen(cwd_path);
    if (len > 1 && cwd_path[len - 1] != '/') cwd_path[len++] = '/';
    unsigned int i;
    for (i = 0; name[i] && len + 1 < CWD_PATH_MAX; i++) cwd_path[len++] = name[i];
    cwd_path[len] = '\0';
    return 0;
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
