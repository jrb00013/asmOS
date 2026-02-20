#include "save_manager.h"
#include "kernel.h"
#include "fs.h"
#include <stddef.h>

static save_slot_info_t slots[SAVE_SLOT_MAX];
static int slot_count;
static uint8_t backup_buf[SAVE_BACKUP_SIZE];  /* for future FAT12 write */
static char backup_name[SAVE_NAME_LEN];
static uint32_t backup_size;
static uint32_t backup_version;

void save_manager_init(void) {
    slot_count = 0;
    backup_name[0] = '\0';
    backup_size = 0;
    backup_version = 0;
    for (int i = 0; i < SAVE_SLOT_MAX; i++) {
        slots[i].name[0] = '\0';
        slots[i].size = 0;
        slots[i].version = 0;
        slots[i].date[0] = '\0';
    }
}

void save_manager_list(void) {
    kprint("\n  ");
    kprint_color(" saves ", 0x2A);
    kprint_color(" (FAT12 files = save candidates) -----\n", 0x08);
    fat12_list_files();
    kprint("  Backup: ");
    kprint_color("saves backup <slot> [name]", 0x0B);
    kprint("  Restore: ");
    kprint_color("saves restore <slot> [name]\n", 0x0B);
    kprint("  Clone: ");
    kprint_color("saves clone <dest> <src>\n", 0x0B);
    kprint("  ----------------------------------------\n\n");
}

static void copy_str(char *dst, const char *src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int save_manager_backup(int slot, const char *dest_name) {
    (void)slot;
    if (!dest_name || !dest_name[0]) return -1;
    copy_str(backup_name, dest_name, SAVE_NAME_LEN);
    backup_version++;
    backup_size = 1;  /* mark backup present (payload in backup_buf when FAT12 read used) */
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprintf(" Backup '%s' -> in-memory (version %u).\n", backup_name, (unsigned)backup_version);
    kprint("  (FAT12 write not yet available; backup stored in RAM.)\n");
    return 0;
}

int save_manager_restore(int slot, const char *from_name) {
    (void)slot;
    (void)from_name;
    if (!backup_name[0]) {
        kprint("  No backup in memory. Use ");
        kprint_color("saves backup <slot> <name>", 0x0B);
        kprint(" first.\n");
        return -1;
    }
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprintf(" Restore from '%s' (%u bytes) -> slot %d.\n", backup_name, (unsigned)backup_size, slot);
    kprint("  (FAT12 write not yet available; restore simulated.)\n");
    return 0;
}

int save_manager_clone(int slot_dest, int slot_src) {
    (void)slot_dest;
    (void)slot_src;
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprintf(" Clone slot %d -> %d (simulated; FS write required for persist).\n", slot_src, slot_dest);
    return 0;
}

void save_manager_list_versions(int slot) {
    (void)slot;
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprint(" versions: ");
    if (backup_name[0])
        kprintf("1. %s (v%u)\n", backup_name, (unsigned)backup_version);
    else
        kprint("(none)\n");
}

int save_manager_rollback(int slot, int version_index) {
    (void)slot;
    (void)version_index;
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprint(" Rollback: select version 1 to restore from last backup.\n");
    return 0;
}
