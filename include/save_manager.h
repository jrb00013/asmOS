#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <stdint.h>

#define SAVE_SLOT_MAX      16
#define SAVE_VERSION_MAX  4
#define SAVE_NAME_LEN     32
#define SAVE_BACKUP_SIZE  (4 * 1024)

typedef struct {
    char name[SAVE_NAME_LEN];
    uint32_t size;
    uint32_t version;
    char date[16];
} save_slot_info_t;

void save_manager_init(void);
void save_manager_list(void);
int save_manager_backup(int slot, const char *dest_name);
int save_manager_restore(int slot, const char *from_name);
int save_manager_clone(int slot_dest, int slot_src);
void save_manager_list_versions(int slot);
int save_manager_rollback(int slot, int version_index);

#endif
