#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

/* External Storage — unified abstraction (Johnny).
 * USB mass storage, internal HDD (fat), network mount.
 * Cross-device transfer + snapshot backup. */

#define STORAGE_DEVICE_MAX   4
#define STORAGE_PATH_MAX     128

typedef enum {
    STORAGE_TYPE_NONE = 0,
    STORAGE_TYPE_USB,
    STORAGE_TYPE_HDD,
    STORAGE_TYPE_NETWORK,
} storage_type_t;

typedef struct {
    int in_use;
    storage_type_t type;
    char mount_path[STORAGE_PATH_MAX];  /* e.g. "mass:", "hdd0:", "net0:" */
    uint32_t total_kb;
    uint32_t free_kb;
    char label[32];
} storage_device_t;

/* Register/update devices (USB, HDD, network). Called by drivers. */
void storage_register(int index, storage_type_t type, const char *mount_path,
                      uint32_t total_kb, uint32_t free_kb, const char *label);

/* List all external storage devices. */
void storage_list(storage_device_t *devices, unsigned int max);

/* Copy file from src path to dst path (cross-device). */
int storage_copy(const char *src, const char *dst);

/* Snapshot backup: clone memory card or given path to external device. */
int storage_snapshot_backup(const char *source_path, const char *backup_path);

/* Init layer (detect USB, HDD, network mounts). */
void storage_init(void);

#endif /* STORAGE_H */
