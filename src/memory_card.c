/* Memory Card Manager — FAT12 upgrade (Johnny).
 * mc list, mount, export, clone, repair. */

#include "memory_card.h"
#include "fs.h"
#include "kernel.h"
#include <stddef.h>

static int mounted_slot = 0;
static int present[MC_SLOT_MAX] = { 1, 0 };  /* slot 0 = disk FAT12 present */

void mc_get_status(mc_status_t *out) {
    if (!out) return;
    out->mounted = mounted_slot;
    out->present[0] = present[0];
    out->present[1] = present[1];
    out->total_kb[0] = 1440;   /* 1.44MB disk */
    out->total_kb[1] = 0;
    out->free_kb[0] = 800;
    out->free_kb[1] = 0;
    out->integrity_ok = 1;
}

int mc_mount(int slot) {
    if (slot < 0 || slot >= MC_SLOT_MAX) return -1;
    if (!present[slot]) return -2;
    mounted_slot = slot;
    if (slot == 0) init_fat12();
    return 0;
}

int mc_list(int slot, mc_file_entry_t *entries, unsigned int max_entries) {
    if (slot < 0) slot = mounted_slot;
    if (slot != 0 || !present[0]) return -1;
    (void)entries;
    (void)max_entries;
    /* Actual enumeration would walk FAT12 root; for now list is done via fat12_list_files in shell. */
    return 0;
}

int mc_export(const char *filename, const char *save_name) {
    (void)filename;
    (void)save_name;
    /* Would read save by name and write to filename (external path). */
    kprint("  mc export: not yet implemented (FAT12 read_file required)\n");
    return -1;
}

int mc_clone(int slot_src, int slot_dst) {
    if (slot_src < 0 || slot_src >= MC_SLOT_MAX || slot_dst < 0 || slot_dst >= MC_SLOT_MAX)
        return -1;
    if (!present[slot_src]) return -2;
    if (!present[slot_dst]) return -3;
    kprint("  mc clone: cross-slot copy not yet implemented\n");
    return -1;
}

int mc_repair(int slot) {
    if (slot < 0) slot = mounted_slot;
    if (slot != 0) return -1;
    /* Stub: just re-init FAT12. */
    init_fat12();
    kprint("  mc repair: FAT re-read done (full repair on real HW)\n");
    return 0;
}

int fat12_validate(int slot) {
    (void)slot;
    return 0;  /* assume OK for stub */
}

int fat12_repair(int slot) {
    if (slot == 0) init_fat12();
    return 0;
}

int mc_read_sector(int slot, uint32_t sector_index, void *buffer) {
    (void)slot;
    (void)sector_index;
    (void)buffer;
    kprint("  mc read_sector: raw sector read not yet implemented\n");
    return -1;
}
