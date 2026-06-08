#include "save_manager.h"
#include "platform.h"
#include "kernel.h"
#include "fs.h"
#include <stddef.h>

static save_slot_info_t slots[SAVE_SLOT_MAX];
static int slot_count;
static uint8_t backup_buf[SAVE_BACKUP_SIZE];
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
    backup_size = sizeof(backup_buf);
    uint32_t i;
    for (i = 0; i < backup_size; i++) backup_buf[i] = (uint8_t)(i & 0xFF);
    if (plat_fs_write(backup_name, backup_buf, backup_size) != 0) {
        kprint("  saves: FAT12 write failed\n");
        return -1;
    }
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprintf(" Backup '%s' written (%u bytes, v%u).\n", backup_name, (unsigned)backup_size, (unsigned)backup_version);
    return 0;
}

int save_manager_restore(int slot, const char *from_name) {
    (void)slot;
    const char *src = from_name && from_name[0] ? from_name : backup_name;
    if (!src[0]) return -1;
    uint32_t got;
    if (plat_fs_read(src, backup_buf, SAVE_BACKUP_SIZE, &got) != 0) {
        kprint("  saves: restore read failed\n");
        return -1;
    }
    backup_size = got;
    copy_str(backup_name, src, SAVE_NAME_LEN);
    kprint("  ");
    kprint_color("saves", 0x0A);
    kprintf(" Restore from '%s' (%u bytes) -> slot %d.\n", src, (unsigned)got, slot);
    return 0;
}

int save_manager_clone(int slot_dest, int slot_src) {
    char src_name[SAVE_NAME_LEN];
    char dst_name[SAVE_NAME_LEN];
    src_name[0] = 'S'; src_name[1] = 'L'; src_name[2] = 'O'; src_name[3] = 'T';
    src_name[4] = '0' + (char)slot_src;
    src_name[5] = '.'; src_name[6] = 'S'; src_name[7] = 'A'; src_name[8] = 'V'; src_name[9] = '\0';
    dst_name[0] = 'S'; dst_name[1] = 'L'; dst_name[2] = 'O'; dst_name[3] = 'T';
    dst_name[4] = '0' + (char)slot_dest;
    dst_name[5] = '.'; dst_name[6] = 'S'; dst_name[7] = 'A'; dst_name[8] = 'V'; dst_name[9] = '\0';
    uint32_t sz;
    if (plat_fs_read(src_name, backup_buf, SAVE_BACKUP_SIZE, &sz) != 0) return -1;
    return plat_fs_write(dst_name, backup_buf, sz);
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
    if (version_index != 1 || !backup_name[0]) return -1;
    return save_manager_restore(slot, backup_name);
}
