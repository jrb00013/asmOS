#include "memory_card.h"
#include "platform.h"
#include "fs.h"
#include "kernel.h"
#include <stddef.h>

static int mounted_slot = 0;
static int present[MC_SLOT_MAX] = { 1, 0 };

void mc_get_status(mc_status_t *out) {
    if (!out) return;
    out->mounted = mounted_slot;
    out->present[0] = present[0];
    out->present[1] = present[1];
    out->total_kb[0] = 1440;
    out->total_kb[1] = 8192;
    out->free_kb[0] = 800;
    out->free_kb[1] = 4096;
    out->integrity_ok = (fat12_validate_fs() == 0);
}

int mc_mount(int slot) {
    if (slot < 0 || slot >= MC_SLOT_MAX) return -1;
    if (!present[slot]) return -2;
    mounted_slot = slot;
    if (slot == 0) {
        plat_fs_init();
        init_fat12();
    }
    return 0;
}

int mc_list(int slot, mc_file_entry_t *entries, unsigned int max_entries) {
    if (slot < 0) slot = mounted_slot;
    if (slot != 0 || !present[0]) return -1;
    plat_file_info_t files[32];
    int n = plat_fs_list(files, 32);
    if (n < 0) return -1;
    unsigned int out = 0;
    int i;
    for (i = 0; i < n && out < max_entries; i++) {
        int j = 0;
        while (files[i].name[j] && j < MC_FILENAME_MAX - 1) {
            entries[out].name[j] = files[i].name[j];
            j++;
        }
        entries[out].name[j] = '\0';
        entries[out].size = files[i].size;
        entries[out].cluster = files[i].cluster;
        entries[out].slot = 0;
        entries[out].preview[0] = '\0';
        out++;
    }
    return (int)out;
}

int mc_export(const char *filename, const char *save_name) {
    if (!filename || !save_name) return -1;
    uint8_t buf[4096];
    uint32_t sz;
    if (plat_fs_read(save_name, buf, sizeof(buf), &sz) != 0) return -1;
    return plat_fs_write(filename, buf, sz);
}

int mc_clone(int slot_src, int slot_dst) {
    (void)slot_src;
    (void)slot_dst;
    uint8_t buf[512];
    uint32_t sz;
    if (plat_fs_read("SLOT0.SAV", buf, sizeof(buf), &sz) != 0) return -1;
    return plat_fs_write("SLOT1.SAV", buf, sz);
}

int mc_repair(int slot) {
    if (slot < 0) slot = mounted_slot;
    if (slot != 0) return -1;
    return plat_fs_repair();
}

int fat12_validate(int slot) {
    (void)slot;
    return plat_fs_validate();
}

int fat12_repair(int slot) {
    (void)slot;
    return plat_fs_repair();
}

int mc_read_sector(int slot, uint32_t sector_index, void *buffer) {
    (void)slot;
    return plat_fs_read_sector(sector_index, buffer);
}
