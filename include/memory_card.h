#ifndef MEMORY_CARD_H
#define MEMORY_CARD_H

#include <stdint.h>

/* Memory Card Manager — FAT12 upgrade (Johnny).
 * mc list, mc mount, mc export, mc clone, mc repair.
 * PS2: FAT12 on mc0:/mc1:; this layer abstracts slot + disk FAT12. */

#define MC_SLOT_MAX       2
#define MC_FILENAME_MAX   32
#define MC_SAVE_PREVIEW_LEN 64

typedef struct {
    char name[MC_FILENAME_MAX];
    uint32_t size;
    uint16_t cluster;
    uint8_t slot;           /* 0 or 1 */
    char preview[MC_SAVE_PREVIEW_LEN];  /* save metadata / icon title */
} mc_file_entry_t;

typedef struct {
    int mounted;            /* which slot is active: 0, 1, or -1 */
    int present[MC_SLOT_MAX];
    uint32_t free_kb[MC_SLOT_MAX];
    uint32_t total_kb[MC_SLOT_MAX];
    int integrity_ok;       /* last check result */
} mc_status_t;

/* List files on current or given slot. slot < 0 = current mount. */
int mc_list(int slot, mc_file_entry_t *entries, unsigned int max_entries);

/* Mount slot 0 or 1. Returns 0 on success. */
int mc_mount(int slot);

/* Export save to path (e.g. save.bin). path is on current FS or external. */
int mc_export(const char *filename, const char *save_name);

/* Clone slot_src to slot_dst (e.g. slot1 -> slot2). */
int mc_clone(int slot_src, int slot_dst);

/* Repair FAT12 on mounted card: validate + fix. */
int mc_repair(int slot);

/* Get status (mounted slot, present, free/total, last integrity). */
void mc_get_status(mc_status_t *out);

/* FAT12: validate filesystem; return 0 if OK, <0 on error. */
int fat12_validate(int slot);

/* FAT12: attempt repair (rebuild FAT chain, clear bad clusters). */
int fat12_repair(int slot);

/* Raw sector view: read sector index into buffer (512 bytes). */
int mc_read_sector(int slot, uint32_t sector_index, void *buffer);

#endif /* MEMORY_CARD_H */
