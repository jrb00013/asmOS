/* External Storage — unified abstraction (Johnny). */

#include "storage.h"
#include "kernel.h"
#include <stddef.h>

static storage_device_t devices[STORAGE_DEVICE_MAX];
static int initialized = 0;

static void str_copy(char *dst, const char *src, unsigned int max) {
    unsigned int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void storage_init(void) {
    for (int i = 0; i < STORAGE_DEVICE_MAX; i++) {
        devices[i].in_use = 0;
        devices[i].type = STORAGE_TYPE_NONE;
        devices[i].mount_path[0] = '\0';
        devices[i].total_kb = 0;
        devices[i].free_kb = 0;
        devices[i].label[0] = '\0';
    }
    initialized = 1;
}

void storage_register(int index, storage_type_t type, const char *mount_path,
                      uint32_t total_kb, uint32_t free_kb, const char *label) {
    if (index < 0 || index >= STORAGE_DEVICE_MAX) return;
    devices[index].in_use = 1;
    devices[index].type = type;
    str_copy(devices[index].mount_path, mount_path ? mount_path : "", STORAGE_PATH_MAX);
    devices[index].total_kb = total_kb;
    devices[index].free_kb = free_kb;
    str_copy(devices[index].label, label ? label : "", 32);
}

void storage_list(storage_device_t *out, unsigned int max) {
    if (!initialized) storage_init();
    if (!out) return;
    unsigned int n = 0;
    for (int i = 0; i < STORAGE_DEVICE_MAX && n < max; i++) {
        if (!devices[i].in_use) continue;
        out[n] = devices[i];
        n++;
    }
}

int storage_copy(const char *src, const char *dst) {
    (void)src;
    (void)dst;
    kprint("  storage copy: cross-device transfer not yet implemented\n");
    return -1;
}

int storage_snapshot_backup(const char *source_path, const char *backup_path) {
    (void)source_path;
    (void)backup_path;
    kprint("  storage snapshot: backup engine not yet implemented\n");
    return -1;
}
