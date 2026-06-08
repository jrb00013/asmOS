#include "storage.h"
#include "platform.h"
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
    storage_register(0, STORAGE_TYPE_HDD, "disk0:", 1440, 800, "ASMOS Disk");
    storage_register(1, STORAGE_TYPE_USB, "usb0:", 16384, 8192, "USB Mass");
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
    if (!src || !dst) return -1;
    static uint8_t buf[8192];
    uint32_t sz;
    if (plat_fs_read(src, buf, sizeof(buf), &sz) != 0) return -1;
    if (plat_fs_write(dst, buf, sz) != 0) return -1;
    kprintf("  storage: copied %s -> %s (%u bytes)\n", src, dst, (unsigned)sz);
    return 0;
}

int storage_snapshot_backup(const char *source_path, const char *backup_path) {
    return storage_copy(source_path, backup_path);
}
